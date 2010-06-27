/***************************************************************************
 *   FreeMedicalForms                                                      *
 *   (C) 2008-2010 by Eric MAEKER, MD                                      *
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
 ***************************************************************************/
#ifndef MAINWINDOWACTIONHANDLER_H
#define MAINWINDOWACTIONHANDLER_H

#include <coreplugin/core_exporter.h>

#include <QObject>
#include <QMainWindow>
#include <QAction>

/**
 * \file MainWindowActionHandler.h
 * \author Eric MAEKER <eric.maeker@free.fr>
 * \version 0.0.8
 * \date 20 Oct 2009
*/

namespace Utils {
class UpdateChecker;
}

namespace Core {
    class MainWindowActions;

namespace Internal {

class CORE_EXPORT MainWindowActionHandler : public QMainWindow
{
    Q_OBJECT
public:
    MainWindowActionHandler(QWidget *parent = 0);
    ~MainWindowActionHandler();

    void createGeneralMenu();
    void connectGeneralActions();

    void createFileMenu();
    void connectFileActions();

    void createPatientMenu();
    void createPatientsActions(int actions);
    void connectPatientActions();

    void createEditMenu();
    void createEditActions();
    void connectEditActions();

    void createPluginsMenu();

    void createConfigurationMenu();
    void connectConfigurationActions();

    void createHelpMenu();
    void connectHelpActions();

    void createTemplatesMenu();

    void createActions(const Core::MainWindowActions &actions);
    void createGeneralActions(const int actions);
    void createFileActions(int actions);
    void createConfigurationActions(int actions);
    void createHelpActions(int actions);
    void createTemplatesActions(int actions);

    void createFormatMenu();
    void createUpdateMenu();

    void createFormatActions();
    void createUpdateActions();

public Q_SLOTS: // Usefull slots that interactions with actions and menus
    virtual bool updateFound();
    virtual void updateCheckerEnd(bool error);


public Q_SLOTS: // Slots automatically connected to actions is requiered
    virtual bool newFile()      {return false;}
    virtual bool openFile()     {return false;}
    virtual bool saveFile()     {return false;}
    virtual bool saveAsFile()   {return false;}
    virtual bool print()        {return false;}
    virtual bool printPreview() {return false;}

    virtual bool createNewPatient() {return false;}
    virtual bool viewPatientIdentity() {return false;}
    virtual bool removePatient() {return false;}

    virtual bool applicationPreferences() {return false;}
    virtual bool applicationConfiguratorWizard() {return false;}
    virtual bool configureMedintux()      {return false;}

    virtual bool aboutApplication();
    virtual bool aboutPlugins();
    virtual bool applicationHelp();
    virtual bool aboutQt()          {return false;}
    virtual bool debugDialog();
    virtual bool checkUpdate();

public Q_SLOTS:
    virtual void switchLanguage(QAction * action);


protected:
    QAction *aGeneralNew, *aGeneralOpen, *aGeneralSave, *aGeneralSaveAs, *aGeneralPrint, *aGeneralPrintPreview, *aGeneralQuit;
    QAction *aGeneralPatientNew, *aGeneralPatientViewIdentity, *aGeneralPatientRemove;
    QAction *aGeneralUndo, *aGeneralRedo, *aGeneralCut, *aGeneralCopy, *aGeneralPaste, *aGeneralSelectAll;
    QAction *aGeneralAppPrefs, *aGeneralAppConfigurator, *aGeneralPlugsPrefs, *aGeneralMedinTux;
    QAction *aGeneralAppAbout, *aGeneralPlugsAbout, *aGeneralAppHelp, *aGeneralQtAbout, *aGeneralDebugDialog;
    QAction *aGeneralCheckUpdate, *aGeneralUpdateAvailable;

    QAction *aNew;
    QAction *aOpen;
    QAction *aSave;
    QAction *aSaveAs;
    QAction *aPrint;
    QAction *aPrintPreview;
    QAction *aQuit;

    QAction *aPatientNew;
    QAction *aPatientViewIdentity;
    QAction *aPatientRemove;

    QAction *aUndo;
    QAction *aRedo;
    QAction *aCut;
    QAction *aCopy;
    QAction *aPaste;
    QAction *aSelectAll;
    QAction *aSearch;

    QAction *aAppPrefs;
    QAction *aAppConfigurator;
    QAction *aPlugsPrefs;
    QAction *aMedinTux;
    QActionGroup *aLanguageGroup;

    QAction *aAppAbout;
    QAction *aPlugsAbout;
    QAction *aAppHelp;
    QAction *aQtAbout;
    QAction *aDebugDialog;

    QAction *aCheckUpdate;
    QAction *aUpdateAvailable;

    Utils::UpdateChecker *m_UpdateChecker;

};

}  // end Internal
}  // end Core

#endif // MAINWINDOWACTIONHANDLER_H
