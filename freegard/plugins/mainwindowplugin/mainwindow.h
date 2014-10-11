/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2014 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
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
 *  Main Developer: Eric MAEKER, <eric.maeker@gmail.com>                   *
 *  Contributors:                                                          *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
#ifndef FREEGUARD_PLUGIN_MAINWINDOW_H
#define FREEGUARD_PLUGIN_MAINWINDOW_H

#include <mainwindowplugin/mainwindow_exporter.h>
#include <coreplugin/imainwindow.h>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QTextEdit;
QT_END_NAMESPACE

/**
 * \file mainwindow.h
 * \author Eric Maeker
 * \version 0.10.0
 * \date 10 Oct 2014
*/

namespace MainWin {
namespace Internal {
//class MainWindowToken;

namespace Ui {
class MainWindow;
class HeaderWidget;
}  // End Ui
}  // End Internal

class FGUARD_MAINWIN_EXPORT MainWindow: public Core::IMainWindow
{
    Q_OBJECT
    enum { MaxRecentFiles = 10 };

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    // IMainWindow Interface
    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();

    void createDockWindows();
    void refreshPatient();
    void readSettings();
    void writeSettings();
    void createStatusBar();

public Q_SLOTS: // Interface of MainWidowActionHandler
    bool newFile();
    bool openFile();
    bool saveFile();
    bool saveAsFile();

    bool applicationPreferences();
    bool configureMedintux();

    void updateCheckerEnd(bool);

    void aboutToShowRecentFiles();
    void openRecentFile();

protected:
    void closeEvent(QCloseEvent *event);
    void changeEvent(QEvent *event);

private:
    bool saveFileContent(const QString &file);
    void readFile(const QString &file);

private Q_SLOTS:
    void postCoreOpened();

public:
    Internal::Ui::MainWindow *ui;
    Internal::Ui::HeaderWidget *_headerWidget;
};

} // End MainWin

#endif  // FREEGUARD_PLUGIN_MAINWINDOW_H
