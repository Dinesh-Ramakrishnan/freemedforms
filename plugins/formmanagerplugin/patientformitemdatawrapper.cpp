/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2012 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program (COPYING.FREEMEDFORMS file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *   Main developers: Eric MAEKER, <eric.maeker@gmail.com>                 *
 *   Contributors:                                                         *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
/*!
 * \class Form::PatientFormItemDataWrapper
 * Get the patient data (see Core::IPatient::PatientDataRepresentation) from the form items.
 * If a patient data representation is available from the forms, you can get it from here.
 * This 'wrapper' always uses the \b last \b available value in the \b saved episodes.
 * \sa Patients::Internal::PatientModelWrapper, Core::IPatient
 */

#include "patientformitemdatawrapper.h"
#include "formcore.h"
#include "iformitem.h"
#include "formmanager.h"
#include "episodemodel.h"
#include "iformitemdata.h"
#include "episodemanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/ipatient.h>

#include <utils/log.h>
#include <translationutils/constants.h>

#include <QDebug>

using namespace Form;
using namespace Internal;
using namespace Trans::ConstantTranslations;

static inline Form::FormCore &formCore() {return Form::FormCore::instance();}
static inline Form::FormManager &formManager() {return Form::FormCore::instance().formManager();}
static inline Form::EpisodeManager &episodeManager() {return Form::FormCore::instance().episodeManager();}
static inline Core::IPatient *patient()  { return Core::ICore::instance()->patient(); }

namespace Form {
namespace Internal {
class PatientFormItemDataWrapperPrivate
{
public:
    PatientFormItemDataWrapperPrivate(PatientFormItemDataWrapper *parent) :
        q(parent)
    {
    }
    
    ~PatientFormItemDataWrapperPrivate()
    {
    }
    
    // Scan all FormItem for patient data representation and feed _availablePatientData
    void scanFormItemDataForAvailableData(const QList<FormMain *> &emptyRootForms)
    {
        _availablePatientData.clear();
        foreach(FormMain *form, emptyRootForms) {
            foreach(FormItem *item, form->flattenFormItemChildren()) {
                if (item->itemData())
                    _availablePatientData << item->patientDataRepresentation();
            }
        }
        _availablePatientData.removeAll(-1);
    }

    // Clear cache then create all EpisodeModel for each available forms empty root
    void clearCacheAndCreateAllEpisodeModels(const QList<FormMain *> &forms)
    {
        // Disconnect and delete all EpisodeModels
        qDeleteAll(_episodeModels.values());
        _episodeModels.clear();
        // Recreate all internal EpisodeModels
        foreach(Form::FormMain *emptyrootform, forms) {
            foreach(Form::FormMain *form, emptyrootform->flattenFormMainChildren()) {
                EpisodeModel *model = new EpisodeModel(form, q);
                // Never use the internal cache of the model
                model->setUseFormContentCache(false);
                model->initialize();
                _episodeModels.insert(form, model);
            }
        }
//        connectModels();
    }

    // Populate all available EpisodeModels with their latest saved content
    void populateEpisodeModelsWithLastEpisode()
    {
        foreach(EpisodeModel *model, _episodeModels.values()) {
            if (!model->populateFormWithLatestValidEpisodeContent()) {
                LOG_ERROR_FOR(q, "EpisodeModel can not be populated");
            }
        }
    }

    void disconnectModels()
    {}

    // Connect editing EpisodeModel (models used to populated editing forms) to the current object
    void connectEditingEpisodeModels()
    {
        QHashIterator<Form::FormMain *, EpisodeModel *> it(_episodeModels);
        while (it.hasNext()) {
            it.next();
            // We must use the uuid because pointer are different than the editing collection
            EpisodeModel *editing = episodeManager().episodeModel(it.key()->uuid());
            if (editing) {
                QObject::connect(editing, SIGNAL(episodeChanged(QModelIndex)), q, SLOT(editingModelEpisodeChanged(QModelIndex)), Qt::UniqueConnection);
                QObject::connect(editing, SIGNAL(rowsInserted(QModelIndex, int, int)), q, SLOT(editingModelRowsInserted(QModelIndex, int, int)));
                QObject::connect(editing, SIGNAL(rowsRemoved(QModelIndex, int, int)), q, SLOT(editingModelRowsRemoved(QModelIndex, int, int)));
            }
        }
    }

    // Get the internal model corresponding to the editing model (returns 0 if model is not found)
    EpisodeModel *getInternalEpisodeModel(EpisodeModel *editing)
    {
        QHashIterator<Form::FormMain *, EpisodeModel *> it(_episodeModels);
        while (it.hasNext()) {
            it.next();
            // We must use the uuid because pointer are different than the editing collection
            if (editing->formUid() == it.value()->formUid())
                return it.value();
        }
        return 0;
    }

    // Refresh the EpisodeModel corresponding to the following index (get its last episode and populate the form)
    void refreshInternals(const QModelIndex &index)
    {
        // Get the editing model that was modified
        EpisodeModel *editing = qobject_cast<EpisodeModel*>(const_cast<QAbstractItemModel*>(index.model()));
        if (!editing)
            return;

        // Get the internal model corresponding to this model
        EpisodeModel *model = getInternalEpisodeModel(editing);
        if (!model)
            return;

        // reload the lastepisode from database
        model->refreshFilter();
        model->populateFormWithLatestValidEpisodeContent();
    }

public:
    QList<int> _availablePatientData;
    QHash<Form::FormMain *, EpisodeModel *> _episodeModels;
    
private:
    PatientFormItemDataWrapper *q;
};
} // namespace Internal
} // end namespace Form

/*! Constructor of the Form::PatientFormItemDataWrapper class */
PatientFormItemDataWrapper::PatientFormItemDataWrapper(QObject *parent) :
    QObject(parent),
    d(new PatientFormItemDataWrapperPrivate(this))
{
    setObjectName("Form::PatientFormItemDataWrapper");
    connect(&formManager(), SIGNAL(patientFormsLoaded()), this, SLOT(onCurrentPatientFormsLoaded()), Qt::DirectConnection);
}

/*! Destructor of the Form::PatientFormItemDataWrapper class */
PatientFormItemDataWrapper::~PatientFormItemDataWrapper()
{
    if (d)
        delete d;
    d = 0;
}

/*! Initializes the object with the default values. Return true if initialization was completed. */
bool PatientFormItemDataWrapper::initialize()
{
    onCurrentPatientChanged();
    return true;
}

bool PatientFormItemDataWrapper::isDataAvailable(int ref) const
{
    return d->_availablePatientData.contains(ref);
}

QVariant PatientFormItemDataWrapper::data(int ref, int role) const
{
    Q_UNUSED(role);
    if (!d->_availablePatientData.contains(ref))
        return QVariant();

    // Find the FormMain parent that contains the item with the correct 'ref'
    const QList<Form::FormMain*> &forms = d->_episodeModels.uniqueKeys();
    foreach(Form::FormMain *main, forms) {
        foreach(Form::FormItem *item, main->flattenFormItemChildren()) {
            if (!item->itemData() || item->patientDataRepresentation() == -1)
                continue;
            // TODO: if the lastepisode does not contain the data, try to find the lastest recorded value
            if (item->patientDataRepresentation() == ref) {
                return item->itemData()->data(ref, Form::IFormItemData::PatientModelRole);
            }
        }
    }
    return QVariant();
}

void PatientFormItemDataWrapper::onCurrentPatientChanged()
{
    d->disconnectModels();
}

void PatientFormItemDataWrapper::onCurrentPatientFormsLoaded()
{
    // Get Duplicates FormCollections
    QList<FormMain *> forms = formManager().allDuplicatesEmptyRootForms();
    d->scanFormItemDataForAvailableData(forms);

    // Create all EpisodeModel
    d->clearCacheAndCreateAllEpisodeModels(forms);

    // Populate each forms with its lastest recorded episode
    d->populateEpisodeModelsWithLastEpisode();

    // Connect editing models
    d->connectEditingEpisodeModels();
}

void PatientFormItemDataWrapper::editingModelEpisodeChanged(const QModelIndex &index)
{
    if (index.column() != EpisodeModel::XmlContent)
        return;

    d->refreshInternals(index);
}

void PatientFormItemDataWrapper::editingModelRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(first);
    Q_UNUSED(last);
    d->refreshInternals(parent);
}

void PatientFormItemDataWrapper::editingModelRowsRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(first);
    Q_UNUSED(last);
    d->refreshInternals(parent);
}
