/*
* Top Level window for KArm.
* Distributed under the GPL.
*/


/*
 * $Id$
 */

#include <numeric>

#include "top.h"
#include <qstring.h>
#include <qkeycode.h>
#include <qlayout.h>
#include <qpopupmenu.h>
#include <kconfig.h>
#include <kaccel.h>
#include <kapplication.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kstatusbar.h>
#include <qvbox.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qptrlist.h>
#include <qevent.h>
#include <qapplication.h>
#include <kkeydialog.h>
#include <kdebug.h>

#include "kaccelmenuwatch.h"
#include "karm.h"
#include "print.h"
#include "task.h"
#include "preferences.h"
#include "tray.h"
#include "listviewiterator.h"
#include "karmutility.h"

KarmWindow::KarmWindow()
  : KMainWindow(0),
	_accel( new KAccel( this ) ),
	_watcher( new KAccelMenuWatch( _accel, this ) ),
	_karm( new Karm( this ) ),
	_totalSum( 0 ),
        _sessionSum( 0 ),
        _hideOnClose( true )
{
  setCentralWidget( _karm );
  // status bar
  startStatusBar();

  // setup PreferenceDialog.
  _preferences = Preferences::instance();
  connect(_preferences, SIGNAL(hideOnClose(bool)), this, SLOT(hideOnClose(bool)));

  // popup menus
  makeMenus();
  _watcher->updateMenus();

  // connections
  connect( _karm, SIGNAL( sessionTimeChanged() ), this, SLOT( updateTime() ) );
  connect(_karm, SIGNAL(selectionChanged  ( QListViewItem * )),this,SLOT(slotSelectionChanged()));
  connect(_karm, SIGNAL( updateButtons() ), this, SLOT(slotSelectionChanged()));
  connect( _karm, SIGNAL(timerTick()), this, SLOT(updateTime()));

  _preferences->load();
  loadGeometry();

  // FIXME: this shouldnt stay. We need to check whether the
  // file exists and if not, create a blank one and ask whether
  // we want to add a task.
  _karm->load();

  connect( _karm, SIGNAL(contextMenuRequested(QListViewItem*, const QPoint&, int)), this, SLOT(contextMenuRequest(QListViewItem*, const QPoint&, int)));

    _tray = new KarmTray( this );
  
  connect( _karm, SIGNAL( timerActive() ),   _tray, SLOT( startClock() ) );
  connect( _karm, SIGNAL( timerActive() ), this, SLOT( enableStopAll() ) );
  connect( _karm, SIGNAL( timerInactive() ), _tray, SLOT( stopClock() ) );
  connect( _karm, SIGNAL( timerInactive() ), this, SLOT( disableStopAll() ) );
  connect( _karm, SIGNAL( tasksChanged( QPtrList<Task> ) ), _tray,
                   SLOT( updateToolTip( QPtrList<Task> ) ) );
  
  slotSelectionChanged();
}

void KarmWindow::slotSelectionChanged()
{
  Task* item= static_cast<Task *>(_karm->currentItem());
  actionDelete->setEnabled(item);
  actionEdit->setEnabled(item);
  actionStart->setEnabled(item && !item->isRunning());
  actionStop->setEnabled(item && item->isRunning());
}

void KarmWindow::save()
{
  kdDebug() << i18n("Saving time data to disk.") << endl;
  _karm->save();
  saveGeometry();
}

void KarmWindow::quit()
{
  kapp->quit();
}


KarmWindow::~KarmWindow()
{
  kdDebug() << i18n("Quitting karm.") << endl;
  _karm->stopAllTimers();
  save();
}

void KarmWindow::enableStopAll()
{
  actionStopAll->setEnabled(true);
}

void KarmWindow::disableStopAll()
{
  actionStopAll->setEnabled(false);
}

void KarmWindow::hideOnClose( bool hide )
{
  _hideOnClose = hide;
}

/**
 * Calculate the sum of the session time and the total time for all leaf tasks and put it in the statusbar.
 */

void KarmWindow::updateTime()
{
  typedef ListViewIterator<Karm, Task> AllIter;
  using std::accumulate;

  _totalSum   = accumulate( AllIter( _karm ), AllIter(), 0, addTaskTotalTime );
  _sessionSum = accumulate( AllIter( _karm ), AllIter(), 0, addTaskSessionTime );

  updateStatusBar();
}

void KarmWindow::updateStatusBar()
{
  QString time;

  time = Karm::formatTime( _sessionSum );
  statusBar()->changeItem( i18n("Session: %1").arg(time), 0 );

  time = Karm::formatTime( _totalSum );
  statusBar()->changeItem( i18n("Total: %1" ).arg(time), 1);
}

void KarmWindow::startStatusBar()
{
  statusBar()->insertItem( i18n("Session"), 0, 0, true );
  statusBar()->insertItem( i18n("Total" ), 1, 0, true );
}

void KarmWindow::saveProperties( KConfig* )
{
  _karm->stopAllTimers();
  _karm->save();
}

void KarmWindow::keyBindings()
{
  KKeyDialog::configureKeys( actionCollection(), xmlFile());
}

void KarmWindow::resetSessionTime()
{
  _karm->resetSessionTimeForAllTasks();
  updateTime();
}


void KarmWindow::makeMenus()
{
  KAction
    *actionKeyBindings,
    *actionResetSession,
    *actionNew,
    *actionNewSub;

  (void) KStdAction::quit(this, SLOT(quit()), actionCollection());
  (void) KStdAction::print(this, SLOT(print()), actionCollection());
  actionKeyBindings = KStdAction::keyBindings(this, SLOT(keyBindings()),actionCollection());
  actionPreferences = KStdAction::preferences(_preferences, SLOT(showDialog()),actionCollection());
  (void) KStdAction::save(_preferences, SLOT(save()),actionCollection());
  actionResetSession = new KAction(i18n("&Reset Session Times"), CTRL + Key_R,this,
                                   SLOT(resetSessionTime()),actionCollection(),
                                   "reset_session_time");
  actionStart = new KAction(i18n("&Start"), QString::fromLatin1("1rightarrow"),
                            Key_S ,_karm,
                            SLOT(changeTimer()),actionCollection(),"start");
  actionStop = new KAction(i18n("S&top"), QString::fromLatin1("stop"),
                           Key_Escape,_karm,
                           SLOT(stopCurrentTimer()),actionCollection(),"stop");
  actionStopAll = new KAction(i18n("Stop &All Timers"), 0, _karm, SLOT(stopAllTimers()), actionCollection(), "stopAll");
  actionStopAll->setEnabled(false);
  actionNew = new KAction(i18n("&New..."), QString::fromLatin1("filenew"),
                             CTRL+Key_N,
                             _karm, SLOT(newTask()),
                             actionCollection(), "new_task");
  actionNewSub = new KAction(i18n("New &Subtask..."), QString::fromLatin1("kmultiple"),
                             CTRL+ALT+Key_N,
                             _karm, SLOT(newSubTask()),
                             actionCollection(), "new_sub_task");
  actionDelete = new KAction(i18n("&Delete..."), QString::fromLatin1("editdelete"),
                             Key_Delete,_karm,
                             SLOT(deleteTask()),actionCollection(),"delete_task");
  actionEdit = new KAction(i18n("&Edit..."), QString::fromLatin1("edit"),
                           CTRL + Key_E,_karm,
                           SLOT(editTask()),actionCollection(),"edit_task");
  createGUI( QString::fromLatin1("karmui.rc") );

  // Tool tops must be set after the createGUI.
  actionKeyBindings->setToolTip(i18n("Configure key bindings"));
  actionKeyBindings->setWhatsThis(i18n("This will let you configure keybindings which is specific to karm"));

  actionResetSession->setToolTip(i18n("Reset session time"));
  actionResetSession->setWhatsThis(i18n("This will reset the session time for all tasks."));

  actionStart->setToolTip(i18n("Start timing for selected task"));
  actionStart->setWhatsThis(i18n("This will start timing for the selected task.\n"
                            "It is even possible to time several tasks simultaneously.\n\n"
                            "You may also start timing of a tasks by double clicking the left mouse "
                            "button on a given task. This will, however, stop timing of other tasks."));

  actionStop->setToolTip(i18n("Stop timing of the selected task"));
  actionStop->setWhatsThis(i18n("Stop timing of the selected task"));

  actionStopAll->setToolTip(i18n("Stop all of the active timers"));
  actionStopAll->setWhatsThis(i18n("Stop all of the active timers"));

  actionNew->setToolTip(i18n("Create new top level task"));
  actionNew->setWhatsThis(i18n("This will create a new top level task."));

  actionDelete->setToolTip(i18n("Delete selected task"));
  actionDelete->setWhatsThis(i18n("This will delete the selected task and all its subtasks."));

  actionEdit->setToolTip(i18n("Edit name or times for selected task"));
  actionEdit->setWhatsThis(i18n("This will bring up a dialog box where you may edit the parameters for the selected task."));
  slotSelectionChanged();
}

void KarmWindow::print()
{
  MyPrinter printer(_karm);
  printer.print();
}

void KarmWindow::loadGeometry()
{
  KConfig &config = *kapp->config();

  config.setGroup( QString::fromLatin1("Main Window Geometry") );
  int w = config.readNumEntry( QString::fromLatin1("Width"), 100 );
  int h = config.readNumEntry( QString::fromLatin1("Height"), 100 );
  w = QMAX( w, sizeHint().width() );
  h = QMAX( h, sizeHint().height() );
  resize(w, h);
}


void KarmWindow::saveGeometry()
{
  KConfig &config = *KGlobal::config();
  config.setGroup( QString::fromLatin1("Main Window Geometry"));
  config.writeEntry( QString::fromLatin1("Width"), width());
  config.writeEntry( QString::fromLatin1("Height"), height());
  config.sync();
}

bool KarmWindow::queryClose()
{
  if ( _hideOnClose ) {
    hide();
    return false;
  }
  return KMainWindow::queryClose();
}

void KarmWindow::contextMenuRequest( QListViewItem*, const QPoint& point, int )
{
    if ( QPopupMenu* pop = dynamic_cast<QPopupMenu*>( factory()->container( i18n( "taskContext" ), this ) ) )
        pop->popup( point );
}

#include "top.moc"
