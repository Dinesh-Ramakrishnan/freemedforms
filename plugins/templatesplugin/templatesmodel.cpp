/***************************************************************************
 *   FreeMedicalForms                                                      *
 *   Copyright (C) 2008-2009 by Eric MAEKER                                *
 *   eric.maeker@free.fr                                                   *
 *   All rights reserved.                                                  *
 *                                                                         *
 *   This program is a free and open source software.                      *
 *   It is released under the terms of the new BSD License.                *
 *                                                                         *
 *   Redistribution and use in source and binary forms, with or without    *
 *   modification, are permitted provided that the following conditions    *
 *   are met:                                                              *
 *   - Redistributions of source code must retain the above copyright      *
 *   notice, this list of conditions and the following disclaimer.         *
 *   - Redistributions in binary form must reproduce the above copyright   *
 *   notice, this list of conditions and the following disclaimer in the   *
 *   documentation and/or other materials provided with the distribution.  *
 *   - Neither the name of the FreeMedForms' organization nor the names of *
 *   its contributors may be used to endorse or promote products derived   *
 *   from this software without specific prior written permission.         *
 *                                                                         *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   *
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     *
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS     *
 *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE        *
 *   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,  *
 *   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,  *
 *   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;      *
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER      *
 *   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT    *
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN     *
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
 *   POSSIBILITY OF SUCH DAMAGE.                                           *
 ***************************************************************************/
/***************************************************************************
 *   Main Developper : Eric MAEKER, <eric.maeker@free.fr>                  *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/
/**
  \class Templates::TemplatesModel
  Manages the templates database in/out and link to views.
  Datas are statically stored so that you can instanciate how many requiered models
  as you want without consuming RAM.
*/

#include "templatesmodel.h"
#include "itemplates.h"
#include "constants.h"

#include <coreplugin/icore.h>
#include <coreplugin/isettings.h>
#include <coreplugin/constants.h>

#include <translationutils/constanttranslations.h>
#include <utils/log.h>
#include <utils/serializer.h>

#include <QList>
#include <QColor>
#include <QSqlTableModel>
#include <QDir>
#include <QMimeData>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>

#include <QDebug>

using namespace Templates;
using namespace Trans::ConstantTranslations;

enum { base64MimeDatas = true };

static inline Core::ISettings *settings() {return Core::ICore::instance()->settings();}

namespace Templates {
namespace Internal {

    static const char *const DATABASE_NAME = "templates";
    static const char *const DATABASE_FILENAME = "templates.db";
    static const char *const DATABASE_ACTUAL_VERSION = "0.3.0";


class TreeItem : public Templates::ITemplate
{
public:
    TreeItem(const QHash<int, QVariant> &datas, TreeItem *parent = 0) :
            ITemplate(datas),
            m_Parent(parent),
            m_IsTemplate(false),
            m_IsModified(false)
    {
        setHasTemplate(datas.value(Constants::Data_IsTemplate).toBool());
    }
    ~TreeItem() { qDeleteAll(m_Children); }

    // Genealogy management
    TreeItem *child(int number) { return m_Children.value(number); }
    int childCount() const { return m_Children.count(); }
    int columnCount() const { return Constants::Data_Max_Param; }
    TreeItem *parent() { return m_Parent; }
    void setParent(TreeItem *parent) { m_Parent = parent; }
    bool addChildren(TreeItem *child)
    {
        if (!m_Children.contains(child))
            m_Children.append(child);
        return true;
    }
    bool insertChild(const int row, TreeItem *child)
    {
        if (row > m_Children.count())
            return false;
        m_Children.insert(row, child);
        return true;
    }
    int childNumber() const
    {
        if (m_Parent)
            return m_Parent->m_Children.indexOf(const_cast<TreeItem*>(this));
        return 0;
    }
    void sortChildren()
    {
        qSort(m_Children.begin(), m_Children.end(), TreeItem::lessThan);
    }

    // For category only tree
    int childCategoryCount() const
    {
        int n = 0;
        foreach(TreeItem *c, this->m_Children) {
            if (!c->isTemplate())
                ++n;
        }
        return n;
    }
    TreeItem *categoryChild(int number)
    {
        QList<TreeItem *> cat;
        foreach(TreeItem *c, this->m_Children) {
            if (!c->isTemplate())
                cat << c;
        }
        return cat.value(number);
    }
    int categoryChildNumber() const
    {
        if (m_Parent) {
            QList<TreeItem *> cat;
            foreach(TreeItem *c, m_Parent->m_Children) {
                if (!c->isTemplate())
                    cat << c;
            }
            return cat.indexOf(const_cast<TreeItem*>(this));
        }
        return 0;
    }

    // For tree management
    void setHasTemplate(bool isTemplate) {m_IsTemplate = isTemplate; setData(Constants::Data_IsTemplate, isTemplate); }
    bool isTemplate() const {return m_IsTemplate;}

    // For database management
    void setModified(bool state)
    {
        m_IsModified = state;
        if (!state)
            m_DirtyRows.clear();
    }
    bool isModified() const {return m_IsModified;}
    void setNewlyCreated(bool state) {setData(Constants::Data_IsNewlyCreated, state); }
    bool isNewlyCreated() const {return data(Constants::Data_IsNewlyCreated).toBool();}

    bool removeChild(TreeItem *child)
    {
        if (m_Children.contains(child)) {
            m_Children.removeAll(child);
            return true;
        }
        return false;
    }

    // For data management
    bool setData(int column, const QVariant &value)
    {
//        qWarning()<< data(column) << value << (data(column)==value);
        if (data(column)==value)
            return true;
        ITemplate::setData(column, value);
        if (column==Constants::Data_IsTemplate) {
            m_IsTemplate=value.toBool();
        }
        m_IsModified = true;
        if (!m_DirtyRows.contains(column))
            m_DirtyRows.append(column);
        return true;
    }

    QVector<int> dirtyRows() const
    {
        return m_DirtyRows;
    }

    // For sort functions
    static bool lessThan(TreeItem *item1, TreeItem *item2)
    {
        // category goes first
        // then sort by name
        bool sameType = (((item1->isTemplate()) && (item2->isTemplate())) || ((!item1->isTemplate()) && (!item2->isTemplate())));
        if (sameType)
            return item1->data(Constants::Data_Label).toString() < item2->data(Constants::Data_Label).toString();
        return item2->isTemplate();
    }

private:
    TreeItem *m_Parent;
    QList<TreeItem*> m_Children;
    QVector<int> m_DirtyRows;
    bool m_IsTemplate, m_IsModified;
};

class TemplatesModelPrivate
{
public:
    TemplatesModelPrivate(Templates::TemplatesModel *parent) :
            q(parent), m_RootItem(0),
            m_ShowOnlyCategories(false)
    {
        if (!m_ModelDatasRetreived) {
            QSqlDatabase DB;
            DB = QSqlDatabase::addDatabase("QSQLITE" , Internal::DATABASE_NAME);
            if (!databasePath().exists()) {
                if (!QDir().mkpath(databasePath().absolutePath())) {
                    tkTr(Trans::Constants::_1_ISNOT_AVAILABLE_CANNOTBE_CREATED).arg(databasePath().absolutePath());
                }
            }
            DB.setDatabaseName(QDir::cleanPath(databasePath().absolutePath() + QDir::separator() + Internal::DATABASE_FILENAME));
            if (!DB.open()) {
                Utils::Log::addError(q, tkTr(Trans::Constants::UNABLE_TO_OPEN_DATABASE_1_ERROR_2)
                                     .arg(Internal::DATABASE_NAME)
                                     .arg(DB.lastError().text()));
            }
            // Test if database already created or need to be created
            if (DB.tables(QSql::Tables).count() == 0) {
                createDatabase();
            }
            QHash<int, QVariant> datas;
            datas.insert(Constants::Data_Label, "ROOT");
            datas.insert(Constants::Data_ParentId, -1);

            if (!m_Tree) {
                m_Tree = new TreeItem(datas,0);
            }
        }
        m_RootItem = m_Tree;
        m_Handles.insert(this);
    }

    ~TemplatesModelPrivate()
    {
        m_Handles.remove(this);
        if (m_Handles.count()==0) {
            if (m_Tree) {
                delete m_Tree;
                m_Tree = 0;
                m_RootItem = 0;
            }
            m_ModelDatasRetreived = false;
        }
    }

    void allInstancesReset() const
    {
        foreach(TemplatesModelPrivate *pr, m_Handles) {
            pr->q->reset();
        }
    }

    void allInstancesEmitDataChangedFrom(const QModelIndex &item)
    {
        /** \todo improve this */
//        bool isTemplate = q->isTemplate(item);
        foreach(TemplatesModelPrivate *pr, m_Handles) {
            if (pr->q->isCategoryOnly() == q->isCategoryOnly()) {
                Q_EMIT(pr->q->dataChanged(pr->q->index(item.row(), 0, item.parent()),
                                          pr->q->index(item.row(), Constants::Data_Max_Param, item.parent())));
                }
            }
    }

    void allInstancesBeginInsertRows(const QModelIndex &parent, int first, int last)
    {
        /** \todo improve this */
        foreach(TemplatesModelPrivate *pr, m_Handles) {
            if (pr->q->isCategoryOnly() == q->isCategoryOnly()) {
                pr->q->beginInsertRows(parent,first,last);
                }
            }
    }

    void allInstancesEndInsertRows()
    {
        /** \todo improve this */
        foreach(TemplatesModelPrivate *pr, m_Handles) {
            if (pr->q->isCategoryOnly() == q->isCategoryOnly()) {
                pr->q->endInsertRows();
                }
            }
    }

    void allInstancesBeginRemoveRows(const QModelIndex &parent, int first, int last)
    {
        /** \todo improve this */
        foreach(TemplatesModelPrivate *pr, m_Handles) {
            if (pr->q->isCategoryOnly() == q->isCategoryOnly()) {
                pr->q->beginRemoveRows(parent,first,last);
                }
            }
    }

    void allInstancesEndRemoveRows()
    {
        /** \todo improve this */
        foreach(TemplatesModelPrivate *pr, m_Handles) {
            if (pr->q->isCategoryOnly() == q->isCategoryOnly()) {
                pr->q->endRemoveRows();
                }
            }
    }

    QDir databasePath() const
    {
        return QDir(settings()->path(Core::ISettings::ReadWriteDatabasesPath) + QDir::separator() + DATABASE_NAME);
    }

    bool createDatabase() const
    {
        Utils::Log::addMessage(q, tkTr(Trans::Constants::TRYING_TO_CREATE_1_PLACE_2)
                               .arg(DATABASE_NAME).arg(databasePath().absolutePath()));
        QSqlDatabase DB = QSqlDatabase::database(DATABASE_NAME);
        if (!DB.open()) {
            Utils::Log::addError(q, tkTr(Trans::Constants::UNABLE_TO_OPEN_DATABASE_1_ERROR_2)
                                 .arg(DATABASE_NAME)
                                 .arg(DB.lastError().text()));
            return false;
        }
        QStringList req;
        req <<  "CREATE TABLE IF NOT EXISTS `TEMPLATES` ("
                "`TEMPLATE_ID`              INTEGER        PRIMARY KEY AUTOINCREMENT,"
                "`TEMPLATE_UUID`            varchar(40)    NULL,"
                "`USER_UUID`                int(11)        NULL,"
                "`ID_CATEGORY`              int(11)        -1,"
                "`LABEL`                    varchar(300)   NULL,"
                "`SUMMARY`                  varchar(500)   NULL,"
                "`CONTENT`                  blob           NULL,"
                "`CONTENT_MIMETYPES`        varchar(300)   NULL,"
                "`DATE_CREATION`            date           NULL,"
                "`DATE_MODIFICATION`        date           NULL,"
                "`THEMED_ICON_FILENAME`     varchar(50)    NULL,"
                "`TRANSMISSION_DATE`        date           NULL"
                ");";
        req <<  "CREATE TABLE IF NOT EXISTS `CATEGORIES` ("
                "`CATEGORY_ID`              INTEGER        PRIMARY KEY AUTOINCREMENT,"
                "`CATEGORY_UUID`            varchar(40)    NULL,"
                "`USER_UUID`                int(11)        NULL,"
                "`PARENT_CATEGORY`          int(11)        -1,"
                "`LABEL`                    varchar(300)   NULL,"
                "`SUMMARY`                  varchar(500)   NULL,"
                "`DATE_CREATION`            date           NULL,"
                "`DATE_MODIFICATION`        date           NULL,"
                "`THEMED_ICON_FILENAME`     varchar(50)    NULL,"
                "`TRANSMISSION_DATE`        date           NULL"
                ");";
        req <<  "CREATE TABLE IF NOT EXISTS `VERSION` ("
                "`ACTUAL`                  varchar(10)"
                ");";
        req <<  QString("INSERT INTO `VERSION` (`ACTUAL`) VALUES('%1');").arg(DATABASE_ACTUAL_VERSION);
        bool toReturn = true;
        foreach(const QString &r, req) {
            QSqlQuery query(r,DB);
            if (query.isActive()) {
                query.next();
            } else {
                Utils::Log::addQueryError(q,query);
                toReturn = false;
            }
        }
        return toReturn;
    }

    void setupModelData()
    {
        if (m_ModelDatasRetreived)
            return;

        Utils::Log::addMessage(q, "Getting Templates Categroies");
        QSqlDatabase DB = QSqlDatabase::database(DATABASE_NAME);
        if (!DB.open()) {
            Utils::Log::addError(q, tkTr(Trans::Constants::UNABLE_TO_OPEN_DATABASE_1_ERROR_2)
                                 .arg(DATABASE_NAME)
                                 .arg(DB.lastError().text()));
            return;
        }

        // get categories
        m_IdToCategory.clear();
        QString req = "SELECT `CATEGORY_ID`, `CATEGORY_UUID`, `USER_UUID`, "
                      "`PARENT_CATEGORY`, `LABEL`, `SUMMARY`,`DATE_CREATION`, `DATE_MODIFICATION` "
                      "FROM `CATEGORIES`";
        QSqlQuery query(req,DB);
        if (query.isActive()) {
            QHash<int, QVariant> datas;
            while (query.next()) {
                datas.insert(Constants::Data_Id, query.value(0));
                datas.insert(Constants::Data_Uuid, query.value(1));
                datas.insert(Constants::Data_UserUuid, query.value(2));
                datas.insert(Constants::Data_ParentId, query.value(3));
                datas.insert(Constants::Data_Label, query.value(4));
                datas.insert(Constants::Data_Summary, query.value(5));
                datas.insert(Constants::Data_CreationDate, query.value(6));
                datas.insert(Constants::Data_ModifDate, query.value(7));
                m_IdToCategory.insert(datas.value(Constants::Data_Id).toInt(), new TreeItem(datas,0));
                datas.clear();
            }
        } else {
            Utils::Log::addQueryError(q, query);
        }
        query.finish();
        foreach(TreeItem *item, m_IdToCategory.values()) {
            // need to be reparented
            item->setParent(m_IdToCategory.value(item->data(Constants::Data_ParentId).toInt(), m_RootItem));
            // add item to the children of its parent
            item->parent()->addChildren(item);
            item->setModified(false);
        }

        // get templates
        Utils::Log::addMessage(q, "Getting Templates");
        QList<TreeItem *> templates;
        req = "SELECT `TEMPLATE_ID`, `TEMPLATE_UUID`, `USER_UUID`, `ID_CATEGORY`, `LABEL`, "
                      "`SUMMARY`, `CONTENT`, `CONTENT_MIMETYPES`, `DATE_CREATION`, `DATE_MODIFICATION`, "
                      "`THEMED_ICON_FILENAME`  FROM `TEMPLATES`";
        query.exec(req);
        if (query.isActive()) {
            QHash<int, QVariant> datas;
            while (query.next()) {
                datas.insert(Constants::Data_Id, query.value(0));
                datas.insert(Constants::Data_Uuid, query.value(1));
                datas.insert(Constants::Data_UserUuid, query.value(2));
                datas.insert(Constants::Data_ParentId, query.value(3));
                datas.insert(Constants::Data_Label, query.value(4));
                datas.insert(Constants::Data_Summary, query.value(5));
                datas.insert(Constants::Data_Content, query.value(6));
                datas.insert(Constants::Data_ContentMimeTypes, query.value(7));
                datas.insert(Constants::Data_CreationDate, query.value(8));
                datas.insert(Constants::Data_ModifDate, query.value(9));
                datas.insert(Constants::Data_ThemedIcon, query.value(10));
                TreeItem *it = new TreeItem(datas,0);
                it->setHasTemplate(true);
                templates.insert(datas.value(Constants::Data_Id).toInt(), it);
                datas.clear();
            }
        } else {
            Utils::Log::addQueryError(q, query);
        }
        query.finish();
        // add templates to categories
        foreach(TreeItem *item, templates) {
            // need to be reparented
            item->setParent(m_IdToCategory.value(item->data(Constants::Data_ParentId).toInt(),m_RootItem));
            // add item to the children of its parent
            item->parent()->addChildren(item);
            item->setModified(false);
        }
        sortItems();
        m_ModelDatasRetreived = true;
    }

    void saveModelDatas(const QModelIndex &start = QModelIndex())
    {
        qWarning() << "saveModelDatas :" << start.data().toString();
        QSqlDatabase DB = QSqlDatabase::database(DATABASE_NAME);
        if (!DB.open()) {
            Utils::Log::addError(q, tkTr(Trans::Constants::UNABLE_TO_OPEN_DATABASE_1_ERROR_2)
                                 .arg(DATABASE_NAME)
                                 .arg(DB.lastError().text()));
            return;
        }
        QModelIndex idx = start;
        for(int i = 0; i< q->rowCount(start); ++i) {
            idx = q->index(i, 0, start);
            TreeItem *t = getItem(idx);
            qWarning() << "   saving" << t->label();
            QSqlQuery query(DB);
            QString req;
            if (t->isNewlyCreated()) {
                if (t->isTemplate()) {
                    query.prepare("INSERT INTO `TEMPLATES` ("
                                  "`TEMPLATE_UUID`,"
                                  "`USER_UUID`,"
                                  "`ID_CATEGORY`,"
                                  "`LABEL`,"
                                  "`SUMMARY`,"
                                  "`CONTENT`,"
                                  "`CONTENT_MIMETYPES`,"
                                  "`DATE_CREATION`,"
                                  "`DATE_MODIFICATION`,"
                                  "`THEMED_ICON_FILENAME`,"
                                  "`TRANSMISSION_DATE`"
                                  ") "
                                  "VALUES (?,?,?,?,?,?,?,?,?,?,?);");
                    query.bindValue(0, t->uuid());
                    query.bindValue(1, t->ownerUuid());
                    query.bindValue(2, t->parentId());
                    query.bindValue(3, t->label());
                    query.bindValue(4, t->summary());
                    query.bindValue(5, t->content());
                    query.bindValue(6, t->contentMimeTypes().join(";"));
                    query.bindValue(7, t->data(Constants::Data_CreationDate).toDate().toString());
                    query.bindValue(8, t->data(Constants::Data_ModifDate).toDate().toString());
                    query.bindValue(9, t->data(Constants::Data_ThemedIcon).toString());
                    query.bindValue(10, QVariant());
                } else {
                    query.prepare("INSERT INTO `CATEGORIES` ("
                                  "`CATEGORY_UUID`,"
                                  "`USER_UUID`,"
                                  "`PARENT_CATEGORY`,"
                                  "`LABEL`,"
                                  "`SUMMARY`,"
                                  "`DATE_CREATION`,"
                                  "`DATE_MODIFICATION`,"
                                  "`THEMED_ICON_FILENAME`,"
                                  "`TRANSMISSION_DATE`"
                                  ") "
                                  "VALUES (?,?,?,?,?,?,?,?,?);");
                    query.bindValue(0, t->uuid());
                    query.bindValue(1, t->ownerUuid());
                    query.bindValue(2, t->parentId());
                    query.bindValue(3, t->label());
                    query.bindValue(4, t->summary());
                    query.bindValue(5, t->data(Constants::Data_CreationDate).toDate().toString());
                    query.bindValue(6, t->data(Constants::Data_ModifDate).toDate().toString());
                    query.bindValue(7, t->data(Constants::Data_ThemedIcon).toString());
                    query.bindValue(8, QVariant());
                }
                query.exec();
                if (!query.isActive()) {
                    Utils::Log::addQueryError(q, query);
                } else {
                    t->setNewlyCreated(false);
                    t->setModified(false);
                }
            } else if (t->isModified()) {
                if (t->isTemplate()) {
                    req = QString("UPDATE `TEMPLATES` SET "
                          "`TEMPLATE_UUID`= '%1' ,"
                          "`USER_UUID`= '%2',"
                          "`ID_CATEGORY`= %3 ,"
                          "`LABEL`= '%4' ,"
                          "`CONTENT_MIMETYPES`= '%5',"
                          "`DATE_CREATION`= '%6',"
                          "`DATE_MODIFICATION`= '%7',"
                          "`THEMED_ICON_FILENAME`= '%8',"
                          "`TRANSMISSION_DATE`= '%9' "
                          "WHERE (`TEMPLATE_ID`= %10 )")
                            .arg(t->uuid())
                            .arg(t->ownerUuid())
                            .arg(t->parentId())
                            .arg(t->label())
                            .arg(t->contentMimeTypes().join(";"))
                            .arg(t->data(Constants::Data_CreationDate).toDate().toString())
                            .arg(t->data(Constants::Data_ModifDate).toDate().toString())
                            .arg(t->data(Constants::Data_ThemedIcon).toString())
                            .arg(t->data(Constants::Data_TransmissionDate).toDate().toString())
                            .arg(t->id());
                    query.exec(req);
                    if (!query.isActive()) {
                        Utils::Log::addQueryError(q, query);
                    }
                    req.clear();
                    query.finish();
                    query.prepare(QString("UPDATE `TEMPLATES` SET `SUMMARY` = ? WHERE `TEMPLATE_ID`=%1").arg(t->id()));
                    query.bindValue(0, t->summary());
                    query.exec();
                    if (!query.isActive()) {
                        Utils::Log::addQueryError(q, query);
                    }
                    query.finish();
                    query.prepare(QString("UPDATE `TEMPLATES` SET `CONTENT` = ? WHERE `TEMPLATE_ID`=%1").arg(t->id()));
                    query.bindValue(0, t->content());
                    query.exec();
                    if (!query.isActive()) {
                        Utils::Log::addQueryError(q, query);
                    }
                } else {
                    req = QString("UPDATE `CATEGORIES` SET "
                          "`CATEGORY_UUID`= '%1' ,"
                          "`USER_UUID`= '%2',"
                          "`PARENT_CATEGORY`= %3 ,"
                          "`LABEL`= '%4' ,"
                          "`DATE_CREATION`= '%5',"
                          "`DATE_MODIFICATION`= '%6',"
                          "`THEMED_ICON_FILENAME`= '%7',"
                          "`TRANSMISSION_DATE`= '%8' "
                          "WHERE (`CATEGORY_ID`= %9 )")
                            .arg(t->uuid())
                            .arg(t->ownerUuid())
                            .arg(t->parentId())
                            .arg(t->label())
                            .arg(t->data(Constants::Data_CreationDate).toDate().toString())
                            .arg(t->data(Constants::Data_ModifDate).toDate().toString())
                            .arg(t->data(Constants::Data_ThemedIcon).toString())
                            .arg(t->data(Constants::Data_TransmissionDate).toDate().toString())
                            .arg(t->id());
                    query.exec(req);
                    if (!query.isActive()) {
                        Utils::Log::addQueryError(q, query);
                    }
                    req.clear();
                    query.finish();
                    query.prepare(QString("UPDATE `CATEGORIES` SET `SUMMARY` = ? WHERE `CATEGORY_ID`=%1").arg(t->id()));
                    query.bindValue(0, t->summary());
                    query.exec();
                    if (!query.isActive()) {
                        Utils::Log::addQueryError(q, query);
                    }
                }
            }
        }
        // save all its children
        for(int i = 0; i < q->rowCount(start); ++i) {
            saveModelDatas(q->index(i, 0, start));
        }
    }

    void sortItems(TreeItem *root = 0)
    {
        if (!root)
            root = m_RootItem;
        int n = root->childCount();
        root->sortChildren();
        for(int i = 0; i < n; ++i) {
            sortItems(root->child(i));
        }
    }

    TreeItem *getItem(const QModelIndex &index) const
    {
        if (index.isValid()) {
            TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
            if (item) return item;
        }
        return m_RootItem;
    }

    QModelIndex findIndex(int id, bool findTemplate = true, const QModelIndex &start = QModelIndex())
    {
//        qWarning() << "findIndex from" << start.data().toString();
        QModelIndex idx = start;
        for(int i = 0; i< q->rowCount(start); ++i) {
            idx = q->index(i, 0, start);
            const TreeItem *t = getItem(idx);
//            qWarning() << "   testing" << t->label();
            if ((t->id() == id) && (t->isTemplate()==findTemplate))
                return idx;
        }
        // test all its children
        for(int i = 0; i < q->rowCount(start); ++i) {
            idx = findIndex(id, findTemplate, q->index(i, 0, start));
            if (idx.isValid())
                return idx;
        }
        return QModelIndex();
    }

    // For debugging
    void warnTree(Internal::TreeItem *root, int indent = 0)
    {
        QString space;
        space.fill(' ',indent);
        qWarning() << space << root->data(Constants::Data_Label);
        for(int i=0; i<root->childCount(); ++i) {
            warnTree(root->child(i), indent + 2);
        }
    }

private:
    Templates::TemplatesModel *q;

public:
    TreeItem *m_RootItem;
    bool m_ShowOnlyCategories;
    static TreeItem *m_Tree;
    static bool m_ModelDatasRetreived;
    static QSet<TemplatesModelPrivate *> m_Handles;
    static QHash<int, TreeItem *> m_IdToCategory;
};

TreeItem *TemplatesModelPrivate::m_Tree = 0;
bool TemplatesModelPrivate::m_ModelDatasRetreived = false;
QSet<TemplatesModelPrivate *> TemplatesModelPrivate::m_Handles;
QHash<int, TreeItem *> TemplatesModelPrivate::m_IdToCategory;

}  // End Internal
}  // End Templates


TemplatesModel::TemplatesModel(QObject *parent) :
        QAbstractItemModel(parent),
        d(new Internal::TemplatesModelPrivate(this))
{
    setObjectName("TemplatesModel");
    d->setupModelData();
    setSupportedDragActions(Qt::CopyAction | Qt::MoveAction);
}

TemplatesModel::~TemplatesModel()
{
    if (d) {
        delete d;
        d = 0;
    }
}

bool TemplatesModel::setCurrentUser(const QString &uuid)
{
    return true;
}

QModelIndex TemplatesModel::index(int row, int column, const QModelIndex &parent) const
 {
     if (parent.isValid() && parent.column() != 0)
         return QModelIndex();

     Internal::TreeItem *parentItem = d->getItem(parent);
     Internal::TreeItem *childItem = 0;
     if (d->m_ShowOnlyCategories) {
         childItem = parentItem->categoryChild(row);
     } else {
         childItem = parentItem->child(row);
     }

     if (childItem) {
         return createIndex(row, column, childItem);
     }
     return QModelIndex();
 }

QModelIndex TemplatesModel::parent(const QModelIndex &index) const
 {
     if (!index.isValid())
         return QModelIndex();

     Internal::TreeItem *childItem = d->getItem(index);
     Internal::TreeItem *parentItem = childItem->parent();

     if (parentItem == d->m_RootItem)
         return QModelIndex();

     if (d->m_ShowOnlyCategories) {
         return createIndex(parentItem->categoryChildNumber(), 0, parentItem);
     }
     return createIndex(parentItem->childNumber(), 0, parentItem);
 }

bool TemplatesModel::reparentIndex(const QModelIndex &item, const QModelIndex &parent)
{
    if (!item.isValid())
        return false;

    Internal::TreeItem *treeItem = d->getItem(item);
    Internal::TreeItem *treeItemParent = d->getItem(item.parent());
    Internal::TreeItem *treeParent = d->getItem(parent);

//    qWarning() << "reparentIndex" << treeItem->label() << treeItem->id()
//               << "to" << treeParent->label() << treeParent->id() << (treeItemParent == treeParent);

    if (treeItemParent == treeParent)
        return true;
    if (treeItem == treeParent)
        return true;

    int row = rowCount(parent);
    // add the item
    insertRow(row, parent);
    for(int i=0; i<Constants::Data_Max_Param; ++i) {
        setData(index(row, i, parent), index(item.row(), i, item.parent()).data());
    }
    setData(index(row, Constants::Data_ParentId, parent), treeParent->id());

    // append its children
    row = 0;
    while (hasIndex(0, 0, item)) {
//        qWarning() << "reparentIndex row" << row << index(row, Constants::Data_Label, item).data().toString();
        reparentIndex(index(0, 0, item), index(0, 0, parent));
        ++row;
    }
    removeRow(item.row(), item.parent());
    return true;
}

int TemplatesModel::rowCount(const QModelIndex &parent) const
{
    Internal::TreeItem *item = d->getItem(parent);
    if (item) {
        if (d->m_ShowOnlyCategories) {
            return item->childCategoryCount();
        } else {
            return item->childCount();
        }
    }
    return 0;
}

int TemplatesModel::columnCount(const QModelIndex &parent) const
{
    return Constants::Data_Max_Param;
}

bool TemplatesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    Internal::TreeItem *it = d->getItem(index);
    if ((role==Qt::EditRole) || (role==Qt::DisplayRole)) {
        it->setData(index.column(), value);
    }
    // emit from all instances
    d->allInstancesEmitDataChangedFrom(index);
    return true;
}

QVariant TemplatesModel::data(const QModelIndex &item, int role) const
{
    if (!item.isValid())
        return QVariant();

    const Internal::TreeItem *it = d->getItem(item);

    switch (role)
    {
    case Qt::EditRole :
    case Qt::DisplayRole :
        {
            return it->data(item.column());
        }
    case Qt::ToolTipRole :
        {
            return it->data(Constants::Data_Summary);
        }
    case Qt::ForegroundRole :
        {
            if (it->isTemplate()) {
                return QColor(settings()->value(Constants::S_FOREGROUND_TEMPLATES, "#000").toString());
            } else {
                return QColor(settings()->value(Constants::S_FOREGROUND_CATEGORIES, "darkblue").toString());
            }
        }
    case Qt::BackgroundRole :
        {
            QColor c;
            if (it->isTemplate()) {
                c = QColor(settings()->value(Constants::S_BACKGROUND_TEMPLATES, "white").toString());
            } else {
                c = QColor(settings()->value(Constants::S_BACKGROUND_CATEGORIES, "white").toString());
            }
            if (it->isNewlyCreated()) {
                c = QColor(Qt::blue);
            } else
                if (it->isModified()) {
                c = QColor(Qt::red);
            }
            if (c.name()=="#ffffff")
                return QVariant();
            c.setAlpha(125);
            return c;
        }
    }
    return QVariant();
}

Qt::ItemFlags TemplatesModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

Qt::DropActions TemplatesModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

bool TemplatesModel::insertTemplate(const Templates::ITemplate *t)
{
    // find parent
    Internal::TreeItem *parent = d->m_IdToCategory.value(t->parentId(), 0);
    if (!parent)
        return false;
    // insertRow in parentIndex
    QModelIndex parentIndex = d->findIndex(parent->id());
    // setDatas of newly created row
//    Internal::TreeItem *parentItem = d->getItem(item)->parent();
//    /** \todo manage user */
//    Internal::TreeItem *newItem = new Internal::TreeItem(t.datas(),parentItem);
//    /** \todo reset all instances of the model */
//    d->resetAllInstances();
    return true;
}

bool TemplatesModel::insertRows(int row, int count, const QModelIndex &parent)
{
//    qWarning() << "insertRows" << row << count << parent.data();
    Internal::TreeItem *parentItem = 0;
    if (!parent.isValid())
        parentItem = d->m_RootItem;
    else
        parentItem = d->getItem(parent);
//    if (!parent.isValid())
//        return false;
//    Internal::TreeItem *parentItem = d->getItem(parent);
    QHash<int, QVariant> datas;
    datas.insert(Constants::Data_Label, tr("New"));
    datas.insert(Constants::Data_ParentId, parentItem->data(Constants::Data_Id));
    datas.insert(Constants::Data_CreationDate, QDateTime::currentDateTime());
    datas.insert(Constants::Data_IsTemplate, false);
    d->allInstancesBeginInsertRows(parent, row, row+count-1);
    for(int i=0; i<count; ++i) {
        Internal::TreeItem *item = new Internal::TreeItem(datas, parentItem);
        if (!parentItem->insertChild(row+i, item))
            return false;
    }
    d->allInstancesEndInsertRows();
    return true;
}

bool TemplatesModel::removeRows(int row, int count, const QModelIndex &parent)
{
//    qWarning() << "removeRows" << row << count;
    Internal::TreeItem *parentItem = 0;
    if (!parent.isValid())
        parentItem = d->m_RootItem;
    else
        parentItem = d->getItem(parent);
    d->allInstancesBeginRemoveRows(parent, row, row+count-1);
    for(int i=0; i<count; ++i) {
        Internal::TreeItem *item = parentItem->child(row+i);
        parentItem->removeChild(item);
        delete item;
        item = 0;
    }
    d->allInstancesEndRemoveRows();
    return true;
}

/**
  \brief Creates the MimeData from the selection to drag
  You can retreive the corresponding indexes using getIndexesFromMimeData()
*/
QMimeData *TemplatesModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QString tmp, cat;
    QModelIndexList fullList;
    foreach (const QModelIndex &index, indexes) {
        Internal::TreeItem *it = d->getItem(index);
        if (it->isTemplate())
            tmp += QString::number(it->id()) + " ";
        else
            cat += QString::number(it->id()) + " ";
    }
    tmp.chop(1);
    cat.chop(1);
    if (!tmp.isEmpty()) {
        tmp.prepend("T(");
        tmp.append(")");
    }
    if (!cat.isEmpty()) {
        cat.prepend("C(");
        cat.append(")");
    }
    tmp += cat;
    mimeData->setData(mimeTypes().at(0), tmp.toUtf8());
    return mimeData;
}

bool TemplatesModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
//    qWarning() << "dropMimeData" << row << action;
    if (action == Qt::IgnoreAction)
        return true;

    if  (action == Qt::CopyAction)
        return true;

    if (!data->hasFormat(mimeTypes().at(0)))
        return false;
    
    QModelIndex parentIndex = parent;
    while (isTemplate(parentIndex)) {
        parentIndex = parentIndex.parent();
    }
    int beginRow = 0;
    if (row != -1)
        beginRow = row;

    QList<QPersistentModelIndex> list = getIndexesFromMimeData(data);
    foreach(const QPersistentModelIndex &id, list) {
        reparentIndex(id, parentIndex);
    }

//    d->saveModelDatas();

    return true;
}

QModelIndex TemplatesModel::getTemplateId(const int id)
{
//    QModelIndex idx = d->findIndex(id, true);
//    qWarning() << "TemplatesModel::getTemplateId" << idx.data().toString();
    return d->findIndex(id, true);;
}

QList<QPersistentModelIndex> TemplatesModel::getIndexesFromMimeData(const QMimeData *mime)
{
//    qWarning() << "TemplatesModel::getIndexesFromMimeData" << mime->data(mimeTypes().at(0));
    Q_ASSERT(mime);
    QList<QPersistentModelIndex> list;
    if (!mime)
        return list;
    // mimeData looks like "T(1,2)C(3)" T for templates C for categories
    QRegExp rx("(\\d+)+");
    QString s = mime->data(mimeTypes().at(0));
    int catBegin = 0;
    // Manage templates
    int pos = 0;
    if (s.contains("T(")) {
        catBegin = s.indexOf(")")+1;
        s = s.mid(0, catBegin);
        while ((pos = rx.indexIn(s, pos)) != -1) {
            list << QPersistentModelIndex(d->findIndex(rx.cap(1).toInt(), true));
            pos += rx.matchedLength();
        }
    }
    qWarning() << "Templates" << s;

    // Manage categories
    s = mime->data(mimeTypes().at(0));
    s = s.mid(catBegin);
    qWarning() << "cat" << s;
    pos = 0;
    while ((pos = rx.indexIn(s, pos)) != -1) {
        list << QPersistentModelIndex(d->findIndex(rx.cap(1).toInt(), false));
        pos += rx.matchedLength();
    }
    foreach(QPersistentModelIndex id, list) {
        qWarning() << id.data().toString();
    }

    return list;
}

bool TemplatesModel::isTemplate(const QModelIndex &index) const
{
    if (!index.isValid())
        return false;

    const Internal::TreeItem *it = d->getItem(index);
    return it->isTemplate();
}

void TemplatesModel::categoriesOnly() const
{
    d->m_ShowOnlyCategories = true;
}

bool TemplatesModel::isCategoryOnly() const
{
    return d->m_ShowOnlyCategories;
}

bool TemplatesModel::submit()
{
    d->saveModelDatas();
    return true;
}
