#include <numeric>
#include <functional>
#include <qptrstack.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <qlistbox.h>
#include <qlayout.h>
#include <qtextstream.h>
#include <qfile.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kmessagebox.h>

#include "task.h"
#include "karm.h"
#include "adddlg.h"
#include "idle.h"
#include "preferences.h"
#include "listviewiterator.h"
#include "subtreeiterator.h"
#include "karmutility.h"

#define T_LINESIZE 1023

Karm::Karm( QWidget *parent, const char *name )
  : QListView( parent, name )
{
  _preferences = Preferences::instance();

  connect(this, SIGNAL(doubleClicked(QListViewItem *)),
          this, SLOT(changeTimer(QListViewItem *)));

  addColumn(i18n("Task Name"));
  addColumn(i18n("Session Time"));
  addColumn(i18n("Total Time"));
  setAllColumnsShowFocus(true);

  // set up the minuteTimer
  _minuteTimer = new QTimer(this);
  connect(_minuteTimer, SIGNAL(timeout()), this, SLOT(minuteUpdate()));
  _minuteTimer->start(1000 * secsPerMinutes);

  // Set up the idle detection.
  _idleTimer = new IdleTimer(_preferences->idlenessTimeout());
  connect(_idleTimer, SIGNAL(extractTime(int)), this, SLOT(extractTime(int)));
  connect(_idleTimer, SIGNAL(stopTimer()), this, SLOT(stopAllTimers()));
  connect(_preferences, SIGNAL(idlenessTimeout(int)), _idleTimer, SLOT(setMaxIdle(int)));
  connect(_preferences, SIGNAL(detectIdleness(bool)), _idleTimer, SLOT(toggleOverAllIdleDetection(bool)));
  if (!_idleTimer->isIdleDetectionPossible())
    _preferences->disableIdleDetection();

  // Setup auto save timer
  _autoSaveTimer = new QTimer(this);
  connect(_preferences, SIGNAL(autoSave(bool)), this, SLOT(autoSaveChanged(bool)));
  connect(_preferences, SIGNAL(autoSavePeriod(int)),
          this, SLOT(autoSavePeriodChanged(int)));
  connect(_autoSaveTimer, SIGNAL(timeout()), this, SLOT(save()));

}

Karm::~Karm()
{
  save();
}

void Karm::load()
{
  QFile f(_preferences->saveFile());

  if( !f.exists() )
    return;

  if( !f.open( IO_ReadOnly ) )
    return;

  QString line;

  QPtrStack<Task> stack;
  Task *task;

  QTextStream stream(&f);

  while( !stream.atEnd() ) {
    //lukas: this breaks for non-latin1 chars!!!
    //if ( file.readLine( line, T_LINESIZE ) == 0 )
    //	break;

    line = stream.readLine();

    if (line.isNull())
	break;

    long minutes;
    int level;
    QString name;

    if (!parseLine(line, &minutes, &name, &level))
      continue;

    unsigned int stackLevel = stack.count();
    for (unsigned int i = level; i<=stackLevel ; i++) {
      stack.pop();
    }

    if (level == 1) {
      task = new Task(name, minutes, 0, this);
    }
    else {
      Task *parent = stack.top();
      task = new Task(name, minutes, 0, parent);
      setRootIsDecorated(true);
      parent->setOpen(true);
    }
    stack.push(task);
  }
  f.close();

	setSelected(firstChild(), true);
	setCurrentItem(firstChild());

  emit( sessionTimeChanged() );
}

bool Karm::parseLine(QString line, long *time, QString *name, int *level)
{
  if (line.find('#') == 0) {
    // A comment line
    return false;
  }

  int index = line.find('\t');
  if (index == -1) {
    // This doesn't seem like a valid record
    return false;
  }

  QString levelStr = line.left(index);
  QString rest = line.remove(0,index+1);

  index = rest.find('\t');
  if (index == -1) {
    // This doesn't seem like a valid record
    return false;
  }

  QString timeStr = rest.left(index);
  *name = rest.remove(0,index+1);

  bool ok;
  *time = timeStr.toLong(&ok);

  if (!ok) {
    // the time field was not a number
    return false;
  }
  *level = levelStr.toInt(&ok);
  if (!ok) {
    // the time field was not a number
    return false;
  }
  return true;
}

void Karm::save()
{
 QFile f(_preferences->saveFile());

 if ( !f.open( IO_WriteOnly | IO_Truncate ) ) {
   QString msg = i18n( "There was an error trying to save your data file.\n"
                       "Time accumulated during this session will not be saved!\n");
   KMessageBox::error(0, msg );
   return;
 }
 const char * comment = "# Karm save data\n";

 f.writeBlock(comment, strlen(comment));  //comment
 f.flush();

 QTextStream stream(&f);

 for (QListViewItem *child =firstChild(); child; child = child->nextSibling()) {
   writeTaskToFile(&stream, child, 1);
 }
 f.close();
}

void Karm::writeTaskToFile(QTextStream *strm, QListViewItem *item, int level)
{
  Task * task = (Task *) item;
  //lukas: correct version for non-latin1 users
  QString _line = QString::fromLatin1("%1\t%2\t%3\n").arg(level).
          arg(task->totalTime()).arg(task->name());

  *strm << _line;

  QListViewItem * child;
  for (child=item->firstChild(); child; child=child->nextSibling()) {
    writeTaskToFile(strm, child, level+1);
  }
}

void Karm::startTimer()
{
  Task *item = ((Task *) currentItem());
  if (item != 0 && activeTasks.findRef(item) == -1) {
    _idleTimer->startIdleDetection();
    item->setRunning(true);
    activeTasks.append(item);
    emit updateButtons();
    if ( activeTasks.count() == 1 )
        emit timerActive();
    
    emit tasksChanged( activeTasks);
  }
}

void Karm::stopAllTimers()
{
  for(unsigned int i=0; i<activeTasks.count();i++) {
    activeTasks.at(i)->setRunning(false);
  }
  _idleTimer->stopIdleDetection();
  activeTasks.clear();
  emit updateButtons();
  emit timerInactive();
  emit tasksChanged( activeTasks);
}

void Karm::resetSessionTimeForAllTasks()
{
    typedef ListViewIterator<Karm, Task> AllIter;
    for_each( AllIter( this ), AllIter(), std::mem_fun( &Task::resetSessionTime ) );

    emit( sessionTimeChanged() );
}

void Karm::stopTimer(Task *item)
{
  if (item != 0 && activeTasks.findRef(item) != -1) {
    activeTasks.removeRef(item);
    item->setRunning(false);
    if (activeTasks.count()== 0) {
      _idleTimer->stopIdleDetection();
      emit timerInactive();
    }
    emit updateButtons();
  }
    emit tasksChanged( activeTasks);
}

void Karm::stopCurrentTimer()
{
  stopTimer((Task *) currentItem());
}

void Karm::changeTimer(QListViewItem *)
{
  Task *item = ((Task *) currentItem());
  if (item != 0 && activeTasks.findRef(item) == -1) {
    // Stop all the other timers.
    for (unsigned int i=0; i<activeTasks.count();i++) {
      (activeTasks.at(i))->setRunning(false);
    }
    activeTasks.clear();

    // Start the new timer.
    startTimer();
  }
  else {
    stopCurrentTimer();
  }
}

void Karm::minuteUpdate()
{
  addTimeToActiveTasks(1);
  if (activeTasks.count() != 0)
    emit(timerTick());
}

void Karm::addTimeToActiveTasks(int minutes)
{
  for(unsigned int i=0; i<activeTasks.count();i++) {
    Task *task = activeTasks.at(i);
    QListViewItem *item = task;
    while (item) {
      ((Task *) item)->incrementTime(minutes);
      item = item->parent();
    }
  }
  // Does not need to emit( sessionTimeChanged() ) because all calling routines emit it instead.
}

void Karm::newTask()
{
  newTask(i18n("New Task"), 0);
}

void Karm::newTask(QString caption, QListViewItem *parent)
{
  AddTaskDialog *dialog = new AddTaskDialog(caption, false, true);
  int result = dialog->exec();

  if (result == QDialog::Accepted) {
    QString taskName = i18n("Unnamed Task");
    if (!dialog->taskName().isEmpty()) {
      taskName = dialog->taskName();
    }

    long total, totalDiff, session, sessionDiff;
    dialog->status( &total, &totalDiff, &session, &sessionDiff );
    Task *task;
    if (parent == 0)
      task = new Task(taskName, total, session, this);
    else
      task = new Task(taskName, total, session, parent);

    updateParents( (QListViewItem *) task, totalDiff, sessionDiff );
    setCurrentItem(task);
    setSelected(task, true);
    emit( sessionTimeChanged() );
  }
  delete dialog;
}

void Karm::newSubTask()
{
  Task *item = static_cast<Task*>( currentItem() );
  if(!item)
    return;
  item->setSessionTime( 0 );
  item->setTotalTime( 0 );
  newTask(i18n("New Sub Task"), item);
  // newTask will emit( sessionTimeChanged() ).
  item->setOpen(true);
  setRootIsDecorated(true);
}

void Karm::editTask()
{
  Task *task = (Task *) currentItem();
  if (!task)
  return;

  bool leafTask = task->childCount() == 0;
  AddTaskDialog *dialog = new AddTaskDialog(i18n("Edit Task"), true, leafTask);
  dialog->setTask(task->name(),
                  task->totalTime(),
                  task->sessionTime());
  int result = dialog->exec();
  if (result == QDialog::Accepted) {
    QString taskName = i18n("Unnamed Task");
    if (!dialog->taskName().isEmpty()) {
      taskName = dialog->taskName();
    }
    task->setName(taskName);

    // update session time as well if the time was changed
    long total, session, totalDiff, sessionDiff;
    total = session = totalDiff = sessionDiff = 0;
    dialog->status( &total, &totalDiff, &session, &sessionDiff );

    task->setTotalTime( total);
    task->setSessionTime( session );

    if( sessionDiff || totalDiff ) {
      emit sessionTimeChanged();
    }

    // Update the parents for this task.
    updateParents( (QListViewItem *) task, totalDiff, sessionDiff );
  }
  delete dialog;
}

namespace 
{

/**
 * Add all the times for each parent.
 *
 * This is a recursive routine to add all of the times for each parent along a top-level branch of the listview.
 * If task is a parent, add all the times of the leaf tasks under it, both total and session.
 * Then go to the next task on the subtree and add it up again for that task.
 */
void accumulateParentTotals ( Task* task )
{
    if ( !task || task->childCount() == 0 )
        return;

    typedef SubtreeIterator<Karm, Task> SubIter;
    using std::accumulate;

    long total   = accumulate( SubIter( task ), SubIter(), 0, addTaskTotalTime );
    long session = accumulate( SubIter( task ), SubIter(), 0, addTaskSessionTime );
    task->setTotalTime( total );
    task->setSessionTime( session );

    SubIter next( task );
    ++next;
    accumulateParentTotals( *next );
}

void callAccumulateParentTotals( Task* task )
{
    // If a task has no parent, it is a top-level task in the list view. So add up all the times for it.
    if ( ! task->QListViewItem::parent() )
        accumulateParentTotals( task );
}

}

void Karm::updateParents( QListViewItem*, long, long )
{
    typedef ListViewIterator<Karm, Task> AllIter;
    for_each( AllIter( this ), AllIter(), callAccumulateParentTotals );
}

void Karm::deleteTask()
{
  Task *item = ((Task *) currentItem());
  if (item == 0) {
    KMessageBox::information(0,i18n("No task selected"));
    return;
  }

  int response = KMessageBox::Yes;
  if ( _preferences->promptDelete() ) {
      if (item->childCount() == 0) {
          response = KMessageBox::questionYesNo(0,
                  i18n( "Are you sure you want to delete the task named\n\"%1\"").arg(item->name()),
                  i18n( "Deleting Task"));
      }
      else {
          response = KMessageBox::questionYesNo(0,
                  i18n( "Are you sure you want to delete the task named\n\"%1\"\n"
                      "NOTE: all its subtasks will also be deleted!").arg(item->name()),
                  i18n( "Deleting Task"));
      }
  }

  if (response == KMessageBox::Yes) {

    // Remove chilren from the active set of tasks.
    stopChildCounters(item);
    stopTimer(item);

    // Stop idle detection if no more counters is running
    if (activeTasks.count() == 0) {
      _idleTimer->stopIdleDetection();
      emit timerInactive();
    }
    emit tasksChanged( activeTasks );

    item->setSessionTime( 0 );
    item->setTotalTime( 0 );
    updateParents( item, 0, 0 );
    delete item;

    // remove root decoration if there is no more children.
    bool anyChilds = false;
    for(QListViewItem *child=firstChild(); child; child=child->nextSibling()) {
      if (child->childCount() != 0) {
        anyChilds = true;
        break;
      }
    }
    if (!anyChilds) {
      setRootIsDecorated(false);
    }
    emit( sessionTimeChanged() );
  }
}

void Karm::stopChildCounters(Task *item)
{
  for (QListViewItem *child=item->firstChild(); child; child=child->nextSibling()) {
    stopChildCounters((Task *)child);
  }
  activeTasks.removeRef(item);
}


void Karm::extractTime(int minutes)
{
  addTimeToActiveTasks(-minutes);
  emit(sessionTimeChanged());
}

void Karm::autoSaveChanged(bool on)
{
  if (on) {
    if (!_autoSaveTimer->isActive()) {
      _autoSaveTimer->start(_preferences->autoSavePeriod()*1000*secsPerMinutes);
    }
  }
  else {
    if (_autoSaveTimer->isActive()) {
      _autoSaveTimer->stop();
    }
  }
}

void Karm::autoSavePeriodChanged(int /*minutes*/)
{
  autoSaveChanged(_preferences->autoSave());
}

#include "karm.moc"
