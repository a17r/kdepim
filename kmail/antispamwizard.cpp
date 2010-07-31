/*
    This file is part of KMail.
    Copyright (c) 2003 Andreas Gungl <a.gungl@gmx.de>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "antispamwizard.h"
#include "kcursorsaver.h"
#include "accountmanager.h"
#include "kmfilter.h"
#include "kmfilteraction.h"
#include "kmfiltermgr.h"
#include "kmkernel.h"
#include "kmfolderseldlg.h"
#include "kmfoldertree.h"
#include "kmmainwin.h"
#include "networkaccount.h"
#include "folderrequester.h"

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>

#include <tqdom.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqtooltip.h>
#include <tqwhatsthis.h>

using namespace KMail;

AntiSpamWizard::AntiSpamWizard( WizardMode mode,
                                TQWidget* parent, KMFolderTree * mainFolderTree )
  : KWizard( parent ),
    mInfoPage( 0 ),
    mSpamRulesPage( 0 ),
    mVirusRulesPage( 0 ),
    mSummaryPage( 0 ),
    mMode( mode )
{
  // read the configuration for the anti-spam tools
  ConfigReader reader( mMode, mToolList );
  reader.readAndMergeConfig();
  mToolList = reader.getToolList();

#ifndef NDEBUG
  if ( mMode == AntiSpam )
    kdDebug(5006) << endl << "Considered anti-spam tools: " << endl;
  else
    kdDebug(5006) << endl << "Considered anti-virus tools: " << endl;
#endif
  for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
        it != mToolList.end(); ++it ) {
#ifndef NDEBUG
    kdDebug(5006) << "Predefined tool: " << (*it).getId() << endl;
    kdDebug(5006) << "Config version: " << (*it).getVersion() << endl;
    kdDebug(5006) << "Selection priority: " << (*it).getPrio() << endl;
    kdDebug(5006) << "Displayed name: " << (*it).getVisibleName() << endl;
    kdDebug(5006) << "Executable: " << (*it).getExecutable() << endl;
    kdDebug(5006) << "WhatsThis URL: " << (*it).getWhatsThisText() << endl;
    kdDebug(5006) << "Filter name: " << (*it).getFilterName() << endl;
    kdDebug(5006) << "Detection command: " << (*it).getDetectCmd() << endl;
    kdDebug(5006) << "Learn spam command: " << (*it).getSpamCmd() << endl;
    kdDebug(5006) << "Learn ham command: " << (*it).getHamCmd() << endl;
    kdDebug(5006) << "Detection header: " << (*it).getDetectionHeader() << endl;
    kdDebug(5006) << "Detection pattern: " << (*it).getDetectionPattern() << endl;
    kdDebug(5006) << "Use as RegExp: " << (*it).isUseRegExp() << endl;
    kdDebug(5006) << "Supports Bayes Filter: " << (*it).useBayesFilter() << endl;
    kdDebug(5006) << "Type: " << (*it).getType() << endl << endl;
#endif
  }

  setCaption( ( mMode == AntiSpam ) ? i18n( "Anti-Spam Wizard" )
                                    : i18n( "Anti-Virus Wizard" ) );
  mInfoPage = new ASWizInfoPage( mMode, 0, "" );
  addPage( mInfoPage,
           ( mMode == AntiSpam )
           ? i18n( "Welcome to the KMail Anti-Spam Wizard" )
           : i18n( "Welcome to the KMail Anti-Virus Wizard" ) );
  connect( mInfoPage, TQT_SIGNAL( selectionChanged( void ) ),
            this, TQT_SLOT( checkProgramsSelections( void ) ) );

  if ( mMode == AntiSpam ) {
    mSpamRulesPage = new ASWizSpamRulesPage( 0, "", mainFolderTree );
    addPage( mSpamRulesPage, i18n( "Options to fine-tune the handling of spam messages" ));
    connect( mSpamRulesPage, TQT_SIGNAL( selectionChanged( void ) ),
             this, TQT_SLOT( slotBuildSummary( void ) ) );
  }
  else {
    mVirusRulesPage = new ASWizVirusRulesPage( 0, "", mainFolderTree );
    addPage( mVirusRulesPage, i18n( "Options to fine-tune the handling of virus messages" ));
    connect( mVirusRulesPage, TQT_SIGNAL( selectionChanged( void ) ),
             this, TQT_SLOT( checkVirusRulesSelections( void ) ) );
  }

  connect( this, TQT_SIGNAL( helpClicked( void) ),
            this, TQT_SLOT( slotHelpClicked( void ) ) );

  setNextEnabled( mInfoPage, false );

  if ( mMode == AntiSpam ) {
    mSummaryPage = new ASWizSummaryPage( 0, "" );
    addPage( mSummaryPage, i18n( "Summary of changes to be made by this wizard" ) );
    setNextEnabled( mSpamRulesPage, true );
    setFinishEnabled( mSummaryPage, true );
  }

  TQTimer::singleShot( 0, this, TQT_SLOT( checkToolAvailability( void ) ) );
}


void AntiSpamWizard::accept()
{
  if ( mSpamRulesPage ) {
    kdDebug( 5006 ) << "Folder name for messages classified as spam is "
                    << mSpamRulesPage->selectedSpamFolderName() << endl;
    kdDebug( 5006 ) << "Folder name for messages classified as unsure is "
                    << mSpamRulesPage->selectedUnsureFolderName() << endl;
  }
  if ( mVirusRulesPage )
    kdDebug( 5006 ) << "Folder name for viruses is "
                    << mVirusRulesPage->selectedFolderName() << endl;

  KMFilterActionDict dict;
  TQValueList<KMFilter*> filterList;
  bool replaceExistingFilters = false;

  // Let's start with virus detection and handling,
  // so we can avoid spam checks for viral messages
  if ( mMode == AntiVirus ) {
    for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) &&
         ( mVirusRulesPage->pipeRulesSelected() && (*it).isVirusTool() ) )
      {
        // pipe messages through the anti-virus tools,
        // one single filter for each tool
        // (could get combined but so it's easier to understand for the user)
        KMFilter* pipeFilter = new KMFilter();
        TQPtrList<KMFilterAction>* pipeFilterActions = pipeFilter->actions();
        KMFilterAction* pipeFilterAction = dict["filter app"]->create();
        pipeFilterAction->argsFromString( (*it).getDetectCmd() );
        pipeFilterActions->append( pipeFilterAction );
        KMSearchPattern* pipeFilterPattern = pipeFilter->pattern();
        pipeFilterPattern->setName( uniqueNameFor( (*it).getFilterName() ) );
        pipeFilterPattern->append( KMSearchRule::createInstance( "<size>",
                                   KMSearchRule::FuncIsGreaterOrEqual, "0" ) );
        pipeFilter->setApplyOnOutbound( false);
        pipeFilter->setApplyOnInbound();
        pipeFilter->setApplyOnExplicit();
        pipeFilter->setStopProcessingHere( false );
        pipeFilter->setConfigureShortcut( false );

        filterList.append( pipeFilter );
      }
    }

    if ( mVirusRulesPage->moveRulesSelected() )
    {
      // Sort out viruses depending on header fields set by the tools
      KMFilter* virusFilter = new KMFilter();
      TQPtrList<KMFilterAction>* virusFilterActions = virusFilter->actions();
      KMFilterAction* virusFilterAction1 = dict["transfer"]->create();
      virusFilterAction1->argsFromString( mVirusRulesPage->selectedFolderName() );
      virusFilterActions->append( virusFilterAction1 );
      if ( mVirusRulesPage->markReadRulesSelected() ) {
        KMFilterAction* virusFilterAction2 = dict["set status"]->create();
        virusFilterAction2->argsFromString( "R" ); // Read
        virusFilterActions->append( virusFilterAction2 );
      }
      KMSearchPattern* virusFilterPattern = virusFilter->pattern();
      virusFilterPattern->setName( uniqueNameFor( i18n( "Virus handling" ) ) );
      virusFilterPattern->setOp( KMSearchPattern::OpOr );
      for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
            it != mToolList.end(); ++it ) {
        if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ))
        {
          if ( (*it).isVirusTool() )
          {
              const TQCString header = (*it).getDetectionHeader().ascii();
              const TQString & pattern = (*it).getDetectionPattern();
              if ( (*it).isUseRegExp() )
                virusFilterPattern->append(
                  KMSearchRule::createInstance( header,
                  KMSearchRule::FuncRegExp, pattern ) );
              else
                virusFilterPattern->append(
                  KMSearchRule::createInstance( header,
                  KMSearchRule::FuncContains, pattern ) );
          }
        }
      }
      virusFilter->setApplyOnOutbound( false);
      virusFilter->setApplyOnInbound();
      virusFilter->setApplyOnExplicit();
      virusFilter->setStopProcessingHere( true );
      virusFilter->setConfigureShortcut( false );

      filterList.append( virusFilter );
    }
  }
  else { // AntiSpam mode
    // TODO Existing filters with same name are replaced. This is hardcoded
    // ATM and needs to be replaced with a value from a (still missing)
    // checkbox in the GUI. At least, the replacement is announced in the GUI.
    replaceExistingFilters = true;
    for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) &&
         (*it).isSpamTool() && !(*it).isDetectionOnly() )
      {
        // pipe messages through the anti-spam tools,
        // one single filter for each tool
        // (could get combined but so it's easier to understand for the user)
        KMFilter* pipeFilter = new KMFilter();
        TQPtrList<KMFilterAction>* pipeFilterActions = pipeFilter->actions();
        KMFilterAction* pipeFilterAction = dict["filter app"]->create();
        pipeFilterAction->argsFromString( (*it).getDetectCmd() );
        pipeFilterActions->append( pipeFilterAction );
        KMSearchPattern* pipeFilterPattern = pipeFilter->pattern();
        if ( replaceExistingFilters )
          pipeFilterPattern->setName( (*it).getFilterName() );
        else
          pipeFilterPattern->setName( uniqueNameFor( (*it).getFilterName() ) );
        pipeFilterPattern->append( KMSearchRule::createInstance( "<size>",
                                   KMSearchRule::FuncIsLessOrEqual, "256000" ) );
        pipeFilter->setApplyOnOutbound( false);
        pipeFilter->setApplyOnInbound();
        pipeFilter->setApplyOnExplicit();
        pipeFilter->setStopProcessingHere( false );
        pipeFilter->setConfigureShortcut( false );

        filterList.append( pipeFilter );
      }
    }

    // Sort out spam depending on header fields set by the tools
    KMFilter* spamFilter = new KMFilter();
    TQPtrList<KMFilterAction>* spamFilterActions = spamFilter->actions();
    if ( mSpamRulesPage->moveSpamSelected() )
    {
      KMFilterAction* spamFilterAction1 = dict["transfer"]->create();
      spamFilterAction1->argsFromString( mSpamRulesPage->selectedSpamFolderName() );
      spamFilterActions->append( spamFilterAction1 );
    }
    KMFilterAction* spamFilterAction2 = dict["set status"]->create();
    spamFilterAction2->argsFromString( "P" ); // Spam
    spamFilterActions->append( spamFilterAction2 );
    if ( mSpamRulesPage->markAsReadSelected() ) {
      KMFilterAction* spamFilterAction3 = dict["set status"]->create();
      spamFilterAction3->argsFromString( "R" ); // Read
      spamFilterActions->append( spamFilterAction3 );
    }
    KMSearchPattern* spamFilterPattern = spamFilter->pattern();
    if ( replaceExistingFilters )
      spamFilterPattern->setName( i18n( "Spam handling" ) );
    else
      spamFilterPattern->setName( uniqueNameFor( i18n( "Spam handling" ) ) );
    spamFilterPattern->setOp( KMSearchPattern::OpOr );
    for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) )
      {
          if ( (*it).isSpamTool() )
          {
            const TQCString header = (*it).getDetectionHeader().ascii();
            const TQString & pattern = (*it).getDetectionPattern();
            if ( (*it).isUseRegExp() )
              spamFilterPattern->append(
                KMSearchRule::createInstance( header,
                KMSearchRule::FuncRegExp, pattern ) );
            else
              spamFilterPattern->append(
                KMSearchRule::createInstance( header,
                KMSearchRule::FuncContains, pattern ) );
          }
      }
    }
    spamFilter->setApplyOnOutbound( false);
    spamFilter->setApplyOnInbound();
    spamFilter->setApplyOnExplicit();
    spamFilter->setStopProcessingHere( true );
    spamFilter->setConfigureShortcut( false );
    filterList.append( spamFilter );

    if ( mSpamRulesPage->moveUnsureSelected() )
    {
      // Sort out messages classified as unsure
      bool atLeastOneUnsurePattern = false;
      KMFilter* unsureFilter = new KMFilter();
      TQPtrList<KMFilterAction>* unsureFilterActions = unsureFilter->actions();
      KMFilterAction* unsureFilterAction1 = dict["transfer"]->create();
      unsureFilterAction1->argsFromString( mSpamRulesPage->selectedUnsureFolderName() );
      unsureFilterActions->append( unsureFilterAction1 );
      KMSearchPattern* unsureFilterPattern = unsureFilter->pattern();
      if ( replaceExistingFilters )
        unsureFilterPattern->setName( i18n( "Semi spam (unsure) handling" ) );
      else
        unsureFilterPattern->setName( uniqueNameFor( i18n( "Semi spam (unsure) handling" ) ) );
      unsureFilterPattern->setOp( KMSearchPattern::OpOr );
      for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
            it != mToolList.end(); ++it ) {
        if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) )
        {
            if ( (*it).isSpamTool() && (*it).hasTristateDetection())
            {
              atLeastOneUnsurePattern = true;
              const TQCString header = (*it).getDetectionHeader().ascii();
              const TQString & pattern = (*it).getDetectionPattern2();
              if ( (*it).isUseRegExp() )
                unsureFilterPattern->append(
                  KMSearchRule::createInstance( header,
                  KMSearchRule::FuncRegExp, pattern ) );
              else
                unsureFilterPattern->append(
                  KMSearchRule::createInstance( header,
                  KMSearchRule::FuncContains, pattern ) );
            }
        }
      }
      unsureFilter->setApplyOnOutbound( false);
      unsureFilter->setApplyOnInbound();
      unsureFilter->setApplyOnExplicit();
      unsureFilter->setStopProcessingHere( true );
      unsureFilter->setConfigureShortcut( false );

      if ( atLeastOneUnsurePattern )
        filterList.append( unsureFilter );
      else
        delete unsureFilter;
    }

    // Classify messages manually as Spam
    KMFilter* classSpamFilter = new KMFilter();
    classSpamFilter->setIcon( "mail_spam" );
    TQPtrList<KMFilterAction>* classSpamFilterActions = classSpamFilter->actions();
    KMFilterAction* classSpamFilterActionFirst = dict["set status"]->create();
    classSpamFilterActionFirst->argsFromString( "P" );
    classSpamFilterActions->append( classSpamFilterActionFirst );
    for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() )
          && (*it).useBayesFilter() && !(*it).isDetectionOnly() )
      {
        KMFilterAction* classSpamFilterAction = dict["execute"]->create();
        classSpamFilterAction->argsFromString( (*it).getSpamCmd() );
        classSpamFilterActions->append( classSpamFilterAction );
      }
    }
    if ( mSpamRulesPage->moveSpamSelected() )
    {
      KMFilterAction* classSpamFilterActionLast = dict["transfer"]->create();
      classSpamFilterActionLast->argsFromString( mSpamRulesPage->selectedSpamFolderName() );
      classSpamFilterActions->append( classSpamFilterActionLast );
    }

    KMSearchPattern* classSpamFilterPattern = classSpamFilter->pattern();
    if ( replaceExistingFilters )
      classSpamFilterPattern->setName( i18n( "Classify as spam" ) );
    else
      classSpamFilterPattern->setName( uniqueNameFor( i18n( "Classify as spam" ) ) );
    classSpamFilterPattern->append( KMSearchRule::createInstance( "<size>",
                                    KMSearchRule::FuncIsGreaterOrEqual, "0" ) );
    classSpamFilter->setApplyOnOutbound( false);
    classSpamFilter->setApplyOnInbound( false );
    classSpamFilter->setApplyOnExplicit( false );
    classSpamFilter->setStopProcessingHere( true );
    classSpamFilter->setConfigureShortcut( true );
    classSpamFilter->setConfigureToolbar( true );
    filterList.append( classSpamFilter );

    // Classify messages manually as not Spam / as Ham
    KMFilter* classHamFilter = new KMFilter();
    classHamFilter->setIcon( "mail_ham" );
    TQPtrList<KMFilterAction>* classHamFilterActions = classHamFilter->actions();
    KMFilterAction* classHamFilterActionFirst = dict["set status"]->create();
    classHamFilterActionFirst->argsFromString( "H" );
    classHamFilterActions->append( classHamFilterActionFirst );
    for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() )
          && (*it).useBayesFilter() && !(*it).isDetectionOnly() )
      {
        KMFilterAction* classHamFilterAction = dict["execute"]->create();
        classHamFilterAction->argsFromString( (*it).getHamCmd() );
        classHamFilterActions->append( classHamFilterAction );
      }
    }
    KMSearchPattern* classHamFilterPattern = classHamFilter->pattern();
    if ( replaceExistingFilters )
      classHamFilterPattern->setName( i18n( "Classify as NOT spam" ) );
    else
      classHamFilterPattern->setName( uniqueNameFor( i18n( "Classify as NOT spam" ) ) );
    classHamFilterPattern->append( KMSearchRule::createInstance( "<size>",
                                    KMSearchRule::FuncIsGreaterOrEqual, "0" ) );
    classHamFilter->setApplyOnOutbound( false);
    classHamFilter->setApplyOnInbound( false );
    classHamFilter->setApplyOnExplicit( false );
    classHamFilter->setStopProcessingHere( true );
    classHamFilter->setConfigureShortcut( true );
    classHamFilter->setConfigureToolbar( true );
    filterList.append( classHamFilter );
  }

  /* Now that all the filters have been added to the list, tell
   * the filter manager about it. That will emit filterListUpdate
   * which will result in the filter list in kmmainwidget being
   * initialized. This should happend only once. */
  if ( !filterList.isEmpty() )
    KMKernel::self()->filterMgr()->appendFilters(
          filterList, replaceExistingFilters );

  TQDialog::accept();
}


void AntiSpamWizard::checkProgramsSelections()
{
  bool status = false;
  bool supportUnsure = false;

  mSpamToolsUsed = false;
  mVirusToolsUsed = false;
  for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
        it != mToolList.end(); ++it ) {
    if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) )
    {
      status = true;
      if ( (*it).isSpamTool() ) {
        mSpamToolsUsed = true;
        if ( (*it).hasTristateDetection() )
          supportUnsure = true;
      }
      if ( (*it).isVirusTool() )
        mVirusToolsUsed = true;
    }
  }

  if ( mMode == AntiSpam ) {
    mSpamRulesPage->allowUnsureFolderSelection( supportUnsure );
    slotBuildSummary();
  }

  if ( ( mMode == AntiVirus ) && mVirusToolsUsed )
    checkVirusRulesSelections();

  setNextEnabled( mInfoPage, status );
}


void AntiSpamWizard::checkVirusRulesSelections()
{
  setFinishEnabled( mVirusRulesPage, anyVirusOptionChecked() );
}


void AntiSpamWizard::checkToolAvailability()
{
  // this can take some time to find the tools
  KCursorSaver busy( KBusyPtr::busy() );

  bool found = false;
  for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
        it != mToolList.end(); ++it ) {
    TQString text( i18n("Scanning for %1...").arg( (*it).getId() ) );
    mInfoPage->setScanProgressText( text );
    if ( (*it).isSpamTool() && (*it).isServerBased() ) {
      // check the configured account for pattern in <server>
      TQString pattern = (*it).getServerPattern();
      kdDebug(5006) << "Testing for server pattern:" << pattern << endl;

      AccountManager* mgr = kmkernel->acctMgr();
      KMAccount* account = mgr->first();
      while ( account ) {
        if ( account->type() == "pop" || account->type().contains( "imap" ) ) {
          const NetworkAccount * n = dynamic_cast<const NetworkAccount*>( account );
          if ( n && n->host().lower().contains( pattern.lower() ) ) {
            mInfoPage->addAvailableTool( (*it).getVisibleName() );
            found = true;
          }
        }
        account = mgr->next();
      }
    }
    else {
      // check the availability of the application
      KApplication::kApplication()->processEvents( 200 );
      if ( !checkForProgram( (*it).getExecutable() ) ) {
        mInfoPage->addAvailableTool( (*it).getVisibleName() );
        found = true;
      }
    }
  }
  if ( found )
    mInfoPage->setScanProgressText( ( mMode == AntiSpam )
                                    ? i18n("Scanning for anti-spam tools finished.")
                                    : i18n("Scanning for anti-virus tools finished.") );
  else
    mInfoPage->setScanProgressText( ( mMode == AntiSpam )
                                    ? i18n("<p>No spam detection tools have been found. "
                                           "Install your spam detection software and "
                                           "re-run this wizard.</p>")
                                    : i18n("Scanning complete. No anti-virus tools found.") );
}


void AntiSpamWizard::slotHelpClicked()
{
  if ( mMode == AntiSpam )
    kapp->invokeHelp( "the-anti-spam-wizard", "kmail" );
  else
    kapp->invokeHelp( "the-anti-virus-wizard", "kmail" );
}


void AntiSpamWizard::slotBuildSummary()
{
  TQString text;
  TQString newFilters;
  TQString replaceFilters;

  if ( mMode == AntiVirus ) {
    text = ""; // TODO add summary for the virus part
  }
  else { // AntiSpam mode
    if ( mSpamRulesPage->markAsReadSelected() )
      text = i18n( "<p>Messages classified as spam are marked as read." );
    else
      text = i18n( "<p>Messages classified as spam are not marked as read." );

    if ( mSpamRulesPage->moveSpamSelected() )
      text += i18n( "<br>Spam messages are moved into the folder named <i>" )
            + mSpamRulesPage->selectedSpamFolderName() + "</i>.</p>";
    else
      text += i18n( "<br>Spam messages are not moved into a certain folder.</p>" );

    for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) &&
         (*it).isSpamTool() && !(*it).isDetectionOnly() ) {
        sortFilterOnExistance( (*it).getFilterName(), newFilters, replaceFilters );
      }
    }
    sortFilterOnExistance( i18n( "Spam handling" ), newFilters, replaceFilters );

    // The need for a andling of status "probably spam" depends on the tools chosen
    if ( mSpamRulesPage->moveUnsureSelected() ) {
      bool atLeastOneUnsurePattern = false;
      for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
            it != mToolList.end(); ++it ) {
        if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) ) {
            if ( (*it).isSpamTool() && (*it).hasTristateDetection())
              atLeastOneUnsurePattern = true;
        }
      }
      if ( atLeastOneUnsurePattern ) {
        sortFilterOnExistance( i18n( "Semi spam (unsure) handling" ),
                               newFilters, replaceFilters );
        text += i18n( "<p>The folder for messages classified as unsure (probably spam) is <i>" )
              + mSpamRulesPage->selectedUnsureFolderName() + "</i>.</p>";
      }
    }

    // Manual classification via toolbar icon / manually applied filter action
    sortFilterOnExistance( i18n( "Classify as spam" ),
                            newFilters, replaceFilters );
    sortFilterOnExistance( i18n( "Classify as NOT spam" ),
                            newFilters, replaceFilters );

    // Show the filters in the summary
    if ( !newFilters.isEmpty() )
      text += i18n( "<p>The wizard will create the following filters:<ul>" )
            + newFilters + "</ul></p>";
    if ( !replaceFilters.isEmpty() )
      text += i18n( "<p>The wizard will replace the following filters:<ul>" )
            + replaceFilters + "</ul></p>";
  }

  mSummaryPage->setSummaryText( text );
}


int AntiSpamWizard::checkForProgram( const TQString &executable )
{
  kdDebug(5006) << "Testing for executable:" << executable << endl;
  KProcess process;
  process << executable;
  process.setUseShell( true );
  process.start( KProcess::Block );
  return process.exitStatus();
}


bool AntiSpamWizard::anyVirusOptionChecked()
{
  return ( mVirusRulesPage->moveRulesSelected()
           || mVirusRulesPage->pipeRulesSelected() );
}


const TQString AntiSpamWizard::uniqueNameFor( const TQString & name )
{
  return KMKernel::self()->filterMgr()->createUniqueName( name );
}


void AntiSpamWizard::sortFilterOnExistance(
        const TQString & intendedFilterName,
        TQString & newFilters, TQString & replaceFilters )
{
  if ( uniqueNameFor( intendedFilterName ) == intendedFilterName )
    newFilters += "<li>" + intendedFilterName + "</li>";
  else
    replaceFilters += "<li>" + intendedFilterName + "</li>";
}


//---------------------------------------------------------------------------
AntiSpamWizard::SpamToolConfig::SpamToolConfig( TQString toolId,
      int configVersion, int prio, TQString name, TQString exec,
      TQString url, TQString filter, TQString detection, TQString spam, TQString ham,
      TQString header, TQString pattern, TQString pattern2, TQString serverPattern,
      bool detectionOnly, bool regExp, bool bayesFilter, bool tristateDetection,
      WizardMode type )
  : mId( toolId ), mVersion( configVersion ), mPrio( prio ),
    mVisibleName( name ), mExecutable( exec ), mWhatsThisText( url ),
    mFilterName( filter ), mDetectCmd( detection ), mSpamCmd( spam ),
    mHamCmd( ham ), mDetectionHeader( header ), mDetectionPattern( pattern ),
    mDetectionPattern2( pattern2 ), mServerPattern( serverPattern ),
    mDetectionOnly( detectionOnly ),
    mUseRegExp( regExp ), mSupportsBayesFilter( bayesFilter ),
    mSupportsUnsure( tristateDetection ), mType( type )
{
}


bool AntiSpamWizard::SpamToolConfig::isServerBased() const
{
  return !mServerPattern.isEmpty();
}


//---------------------------------------------------------------------------
AntiSpamWizard::ConfigReader::ConfigReader( WizardMode mode,
                                            TQValueList<SpamToolConfig> & configList )
  : mToolList( configList ),
    mMode( mode )
{
  if ( mMode == AntiSpam )
    mConfig = new KConfig( "kmail.antispamrc", true );
  else
    mConfig = new KConfig( "kmail.antivirusrc", true );
}

AntiSpamWizard::ConfigReader::~ConfigReader( )
{
  delete mConfig;
}


void AntiSpamWizard::ConfigReader::readAndMergeConfig()
{
  TQString groupName = ( mMode == AntiSpam )
                      ? TQString("Spamtool #%1")
                      : TQString("Virustool #%1");
  // read the configuration from the global config file
  mConfig->setReadDefaults( true );
  KConfigGroup general( mConfig, "General" );
  int registeredTools = general.readNumEntry( "tools", 0 );
  for (int i = 1; i <= registeredTools; i++)
  {
    KConfigGroup toolConfig( mConfig, groupName.arg( i ) );
    if( !toolConfig.readBoolEntry( "HeadersOnly", false ) )
      mToolList.append( readToolConfig( toolConfig ) );
  }

  // read the configuration from the user config file
  // and merge newer config data
  mConfig->setReadDefaults( false );
  KConfigGroup user_general( mConfig, "General" );
  int user_registeredTools = user_general.readNumEntry( "tools", 0 );
  for (int i = 1; i <= user_registeredTools; i++)
  {
    KConfigGroup toolConfig( mConfig, groupName.arg( i ) );
    if( !toolConfig.readBoolEntry( "HeadersOnly", false ) )
      mergeToolConfig( readToolConfig( toolConfig ) );
  }
  // Make sure to have add least one tool listed even when the
  // config file was not found or whatever went wrong
  // Currently only works for spam tools
  if ( mMode == AntiSpam ) {
    if ( registeredTools < 1 && user_registeredTools < 1 )
      mToolList.append( createDummyConfig() );
    sortToolList();
  }
}


AntiSpamWizard::SpamToolConfig
    AntiSpamWizard::ConfigReader::readToolConfig( KConfigGroup & configGroup )
{
  TQString id = configGroup.readEntry( "Ident" );
  int version = configGroup.readNumEntry( "Version" );
#ifndef NDEBUG
  kdDebug(5006) << "Found predefined tool: " << id << endl;
  kdDebug(5006) << "With config version  : " << version << endl;
#endif
  int prio = configGroup.readNumEntry( "Priority", 1 );
  TQString name = configGroup.readEntry( "VisibleName" );
  TQString executable = configGroup.readEntry( "Executable" );
  TQString url = configGroup.readEntry( "URL" );
  TQString filterName = configGroup.readEntry( "PipeFilterName" );
  TQString detectCmd = configGroup.readEntry( "PipeCmdDetect" );
  TQString spamCmd = configGroup.readEntry( "ExecCmdSpam" );
  TQString hamCmd = configGroup.readEntry( "ExecCmdHam" );
  TQString header = configGroup.readEntry( "DetectionHeader" );
  TQString pattern = configGroup.readEntry( "DetectionPattern" );
  TQString pattern2 = configGroup.readEntry( "DetectionPattern2" );
  TQString serverPattern = configGroup.readEntry( "ServerPattern" );
  bool detectionOnly = configGroup.readBoolEntry( "DetectionOnly", false );
  bool useRegExp = configGroup.readBoolEntry( "UseRegExp" );
  bool supportsBayes = configGroup.readBoolEntry( "SupportsBayes", false );
  bool supportsUnsure = configGroup.readBoolEntry( "SupportsUnsure", false );
  return SpamToolConfig( id, version, prio, name, executable, url,
                         filterName, detectCmd, spamCmd, hamCmd,
                         header, pattern, pattern2, serverPattern,
                         detectionOnly, useRegExp,
                         supportsBayes, supportsUnsure, mMode );
}


AntiSpamWizard::SpamToolConfig AntiSpamWizard::ConfigReader::createDummyConfig()
{
  return SpamToolConfig( "spamassassin", 0, 1,
                        "SpamAssassin", "spamassassin -V",
                        "http://spamassassin.org", "SpamAssassin Check",
                        "spamassassin -L",
                        "sa-learn -L --spam --no-rebuild --single",
                        "sa-learn -L --ham --no-rebuild --single",
                        "X-Spam-Flag", "yes", "", "",
                        false, false, true, false, AntiSpam );
}


void AntiSpamWizard::ConfigReader::mergeToolConfig( AntiSpamWizard::SpamToolConfig config )
{
  bool found = false;
  for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
        it != mToolList.end(); ++it ) {
#ifndef NDEBUG
    kdDebug(5006) << "Check against tool: " << (*it).getId() << endl;
    kdDebug(5006) << "Against version   : " << (*it).getVersion() << endl;
#endif
    if ( (*it).getId() == config.getId() )
    {
      found = true;
      if ( (*it).getVersion() < config.getVersion() )
      {
#ifndef NDEBUG
        kdDebug(5006) << "Replacing config ..." << endl;
#endif
        mToolList.remove( it );
        mToolList.append( config );
      }
      break;
    }
  }
  if ( !found )
    mToolList.append( config );
}


void AntiSpamWizard::ConfigReader::sortToolList()
{
  TQValueList<SpamToolConfig> tmpList;
  SpamToolConfig config;

  while ( !mToolList.isEmpty() ) {
    TQValueListIterator<SpamToolConfig> highest;
    int priority = 0; // ascending
    for ( TQValueListIterator<SpamToolConfig> it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( (*it).getPrio() > priority ) {
        priority = (*it).getPrio();
        highest = it;
      }
    }
    config = (*highest);
    tmpList.append( config );
    mToolList.remove( highest );
  }
  for ( TQValueListIterator<SpamToolConfig> it = tmpList.begin();
        it != tmpList.end(); ++it ) {
    mToolList.append( (*it) );
  }
}


//---------------------------------------------------------------------------
ASWizPage::ASWizPage( TQWidget * parent, const char * name,
                      const TQString *bannerName )
  : TQWidget( parent, name )
{
  TQString banner = "kmwizard.png";
  if ( bannerName && !bannerName->isEmpty() )
    banner = *bannerName;

  mLayout = new TQHBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );
  mPixmap = new TQPixmap( UserIcon(banner) );
  mBannerLabel = new TQLabel( this );
  mBannerLabel->setPixmap( *mPixmap );
  mBannerLabel->setScaledContents( false );
  mBannerLabel->setFrameShape( TQFrame::StyledPanel );
  mBannerLabel->setFrameShadow( TQFrame::Sunken );

  mLayout->addWidget( mBannerLabel );
  mLayout->addItem( new TQSpacerItem( 5, 5, TQSizePolicy::Minimum, TQSizePolicy::Expanding ) );
}


//---------------------------------------------------------------------------
ASWizInfoPage::ASWizInfoPage( AntiSpamWizard::WizardMode mode,
                              TQWidget * parent, const char * name )
  : ASWizPage( parent, name )
{
  TQBoxLayout * layout = new TQVBoxLayout( mLayout );

  mIntroText = new TQLabel( this );
  mIntroText->setText(
    ( mode == AntiSpamWizard::AntiSpam )
    ? i18n(
      "The wizard will search for any tools to do spam detection\n"
      "and setup KMail to work with them."
      )
    : i18n(
      "<p>Here you can get some assistance in setting up KMail's filter "
      "rules to use some commonly-known anti-virus tools.</p>"
      "<p>The wizard can detect those tools on your computer as "
      "well as create filter rules to classify messages using these "
      "tools and to separate messages containing viruses. "
      "The wizard will not take any existing filter "
      "rules into consideration: it will always append the new rules.</p>"
      "<p><b>Warning:</b> As KMail appears to be frozen during the scan of the "
      "messages for viruses, you may encounter problems with "
      "the responsiveness of KMail because anti-virus tool "
      "operations are usually time consuming; please consider "
      "deleting the filter rules created by the wizard to get "
      "back to the former behavior."
      ) );
  layout->addWidget( mIntroText );

  mScanProgressText = new TQLabel( this );
  mScanProgressText->setText( "" ) ;
  layout->addWidget( mScanProgressText );

  mToolsList = new KListBox( this );
  mToolsList->hide();
  mToolsList->setSelectionMode( TQListBox::Multi );
  mToolsList->setRowMode( TQListBox::FixedNumber );
  mToolsList->setRowMode( 10 );
  layout->addWidget( mToolsList );
  connect( mToolsList, TQT_SIGNAL(selectionChanged()),
           this, TQT_SLOT(processSelectionChange(void)) );

  mSelectionHint = new TQLabel( this );
  mSelectionHint->setText( "" );
  layout->addWidget( mSelectionHint );

  layout->addStretch();
}


void ASWizInfoPage::setScanProgressText( const TQString &toolName )
{
  mScanProgressText->setText( toolName );
}


void ASWizInfoPage::addAvailableTool( const TQString &visibleName )
{
  TQString listName = visibleName;
  mToolsList->insertItem( listName );
  if ( !mToolsList->isVisible() )
  {
    mToolsList->show();
    mToolsList->setSelected( 0, true );
    mSelectionHint->setText( i18n("<p>Please select the tools to be used "
                                  "for the detection and go "
                                  "to the next page.</p>") );
  }
}

bool ASWizInfoPage::isProgramSelected( const TQString &visibleName )
{
  TQString listName = visibleName;
  return mToolsList->isSelected( mToolsList->findItem( listName ) );
}


void ASWizInfoPage::processSelectionChange()
{
  emit selectionChanged();
}


//---------------------------------------------------------------------------
ASWizSpamRulesPage::ASWizSpamRulesPage( TQWidget * parent, const char * name,
                                        KMFolderTree * mainFolderTree )
  : ASWizPage( parent, name )
{
  TQVBoxLayout *layout = new TQVBoxLayout( mLayout );

  mMarkRules = new TQCheckBox( i18n("&Mark detected spam messages as read"), this );
  TQWhatsThis::add( mMarkRules,
      i18n( "Mark messages which have been classified as spam as read.") );
  layout->addWidget( mMarkRules);

  mMoveSpamRules = new TQCheckBox( i18n("Move &known spam to:"), this );
  TQWhatsThis::add( mMoveSpamRules,
      i18n( "The default folder for spam messages is the trash folder, "
            "but you may change that in the folder view below.") );
  layout->addWidget( mMoveSpamRules );

  mFolderReqForSpamFolder = new FolderRequester( this, mainFolderTree );
  mFolderReqForSpamFolder->setFolder( "trash" );
  mFolderReqForSpamFolder->setMustBeReadWrite( true );
  mFolderReqForSpamFolder->setShowOutbox( false );
  mFolderReqForSpamFolder->setShowImapFolders( false );

  TQHBoxLayout *hLayout1 = new TQHBoxLayout( layout );
  hLayout1->addSpacing( KDialog::spacingHint() * 3 );
  hLayout1->addWidget( mFolderReqForSpamFolder );

  mMoveUnsureRules = new TQCheckBox( i18n("Move &probable spam to:"), this );
  TQWhatsThis::add( mMoveUnsureRules,
      i18n( "The default folder is the inbox folder, but you may change that "
            "in the folder view below.<p>"
            "Not all tools support a classification as unsure. If you haven't "
            "selected a capable tool, you can't select a folder as well.") );
  layout->addWidget( mMoveUnsureRules );

  mFolderReqForUnsureFolder = new FolderRequester( this, mainFolderTree );
  mFolderReqForUnsureFolder->setFolder( "inbox" );
  mFolderReqForUnsureFolder->setMustBeReadWrite( true );
  mFolderReqForUnsureFolder->setShowOutbox( false );
  mFolderReqForUnsureFolder->setShowImapFolders( false );

  TQHBoxLayout *hLayout2 = new TQHBoxLayout( layout );
  hLayout2->addSpacing( KDialog::spacingHint() * 3 );
  hLayout2->addWidget( mFolderReqForUnsureFolder );

  layout->addStretch();

  connect( mMarkRules, TQT_SIGNAL(clicked()),
            this, TQT_SLOT(processSelectionChange(void)) );
  connect( mMoveSpamRules, TQT_SIGNAL(clicked()),
            this, TQT_SLOT(processSelectionChange(void)) );
  connect( mMoveUnsureRules, TQT_SIGNAL(clicked()),
            this, TQT_SLOT(processSelectionChange(void)) );
  connect( mFolderReqForSpamFolder, TQT_SIGNAL(folderChanged(KMFolder*)),
            this, TQT_SLOT(processSelectionChange(KMFolder*)) );
  connect( mFolderReqForUnsureFolder, TQT_SIGNAL(folderChanged(KMFolder*)),
            this, TQT_SLOT(processSelectionChange(KMFolder*)) );

  mMarkRules->setChecked( true );
  mMoveSpamRules->setChecked( true );
}


bool ASWizSpamRulesPage::markAsReadSelected() const
{
  return mMarkRules->isChecked();
}


bool ASWizSpamRulesPage::moveSpamSelected() const
{
  return mMoveSpamRules->isChecked();
}


bool ASWizSpamRulesPage::moveUnsureSelected() const
{
  return mMoveUnsureRules->isChecked();
}


TQString ASWizSpamRulesPage::selectedSpamFolderName() const
{
  TQString name = "trash";
  if ( mFolderReqForSpamFolder->folder() )
    name = mFolderReqForSpamFolder->folder()->idString();
  return name;
}


TQString ASWizSpamRulesPage::selectedUnsureFolderName() const
{
  TQString name = "inbox";
  if ( mFolderReqForUnsureFolder->folder() )
    name = mFolderReqForUnsureFolder->folder()->idString();
  return name;
}


void ASWizSpamRulesPage::processSelectionChange()
{
  mFolderReqForSpamFolder->setEnabled( mMoveSpamRules->isChecked() );
  mFolderReqForUnsureFolder->setEnabled( mMoveUnsureRules->isChecked() );
  emit selectionChanged();
}


void ASWizSpamRulesPage::processSelectionChange( KMFolder* )
{
  processSelectionChange();
}


void ASWizSpamRulesPage::allowUnsureFolderSelection( bool enabled )
{
  mMoveUnsureRules->setEnabled( enabled );
  mMoveUnsureRules->setShown( enabled );
  mFolderReqForUnsureFolder->setEnabled( enabled );
  mFolderReqForUnsureFolder->setShown( enabled );
}


//---------------------------------------------------------------------------
ASWizVirusRulesPage::ASWizVirusRulesPage( TQWidget * parent, const char * name,
                                  KMFolderTree * mainFolderTree )
  : ASWizPage( parent, name )
{
  TQGridLayout *grid = new TQGridLayout( mLayout, 5, 1, KDialog::spacingHint() );

  mPipeRules = new TQCheckBox( i18n("Check messages using the anti-virus tools"), this );
  TQWhatsThis::add( mPipeRules,
      i18n( "Let the anti-virus tools check your messages. The wizard "
            "will create appropriate filters. The messages are usually "
            "marked by the tools so that following filters can react "
            "on this and, for example, move virus messages to a special folder.") );
  grid->addWidget( mPipeRules, 0, 0 );

  mMoveRules = new TQCheckBox( i18n("Move detected viral messages to the selected folder"), this );
  TQWhatsThis::add( mMoveRules,
      i18n( "A filter to detect messages classified as virus-infected and to move "
            "those messages into a predefined folder is created. The "
            "default folder is the trash folder, but you may change that "
            "in the folder view.") );
  grid->addWidget( mMoveRules, 1, 0 );

  mMarkRules = new TQCheckBox( i18n("Additionally, mark detected viral messages as read"), this );
  mMarkRules->setEnabled( false );
  TQWhatsThis::add( mMarkRules,
      i18n( "Mark messages which have been classified as "
            "virus-infected as read, as well as moving them "
            "to the selected folder.") );
  grid->addWidget( mMarkRules, 2, 0 );

  TQString s = "trash";
  mFolderTree = new SimpleFolderTree( this, mainFolderTree, s, true );
  grid->addWidget( mFolderTree, 3, 0 );

  connect( mPipeRules, TQT_SIGNAL(clicked()),
            this, TQT_SLOT(processSelectionChange(void)) );
  connect( mMoveRules, TQT_SIGNAL(clicked()),
            this, TQT_SLOT(processSelectionChange(void)) );
  connect( mMarkRules, TQT_SIGNAL(clicked()),
            this, TQT_SLOT(processSelectionChange(void)) );
  connect( mMoveRules, TQT_SIGNAL( toggled( bool ) ),
           mMarkRules, TQT_SLOT( setEnabled( bool ) ) );
}

bool ASWizVirusRulesPage::pipeRulesSelected() const
{
  return mPipeRules->isChecked();
}


bool ASWizVirusRulesPage::moveRulesSelected() const
{
  return mMoveRules->isChecked();
}

bool ASWizVirusRulesPage::markReadRulesSelected() const
{
  return mMarkRules->isChecked();
}


TQString ASWizVirusRulesPage::selectedFolderName() const
{
  TQString name = "trash";
  if ( mFolderTree->folder() )
    name = mFolderTree->folder()->idString();
  return name;
}

void ASWizVirusRulesPage::processSelectionChange()
{
  emit selectionChanged();
}


//---------------------------------------------------------------------------
ASWizSummaryPage::ASWizSummaryPage( TQWidget * parent, const char * name )
  : ASWizPage( parent, name )
{
  TQBoxLayout * layout = new TQVBoxLayout( mLayout );

  mSummaryText = new TQLabel( this );
  layout->addWidget( mSummaryText );
  layout->addStretch();
}


void ASWizSummaryPage::setSummaryText( const TQString & text )
{
  mSummaryText->setText( text );
}


#include "antispamwizard.moc"
