#include <numeric>
#include <functional>
#include <algorithm>
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
#include "kdebug.h"
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
  connect(_idleTimer, SIGNAL(stopAllTimers()), this, SLOT(stopAllTimers()));
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

  connect(&kWinModule, SIGNAL(currentDesktopChanged(int)), this, SLOT(handleDesktopChange(int)));

  desktopCount = kWinModule.numberOfDesktops();
  lastDesktop = kWinModule.currentDesktop()-1;
}

Karm::~Karm()
{
  save();
}

void Karm::handleDesktopChange(int desktop)
{
  desktop--; // desktopTracker starts with 0 for desktop 1
  // start all tasks setup for running on desktop
  TaskVector::iterator it;

  // stop trackers for lastDesktop
  TaskVector tv = desktopTracker[lastDesktop];
  for (it = tv.begin(); it != tv.end(); it++) {
    stopTimerFor(*it);
  }

  // start trackers for desktop
  tv = desktopTracker[desktop];
  for (it = tv.begin(); it != tv.end(); it++) {
    startTimerFor(*it);
  }
  lastDesktop = desktop;

  emit updateButtons();
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
    DesktopListType desktops;
    if (!parseLine(line, &minutes, &name, &level, &desktops))
      continue;

    unsigned int stackLevel = stack.count();
    for (unsigned int i = level; i<=stackLevel ; i++) {
      stack.pop();
    }

    if (level == 1) {
      task = new Task(name, minutes, 0, desktops, this);
      emit( sessionTimeChanged( 0, minutes ) );
    }
    else {
      Task *parent = stack.top();
      task = new Task(name, minutes, 0, desktops, parent);
      emit( sessionTimeChanged( 0, minutes ) );
      setRootIsDecorated(true);
      parent->setOpen(true);
    }

    // update desktop trackers
    updateTrackers(task, desktops);

    stack.push(task);
  }
  f.close();

  setSelected(firstChild(), true);
  setCurrentItem(firstChild());

  applyTrackers();

  //emit( sessionTimeChanged() );
}

void Karm::applyTrackers() 
{
  TaskVector &tv = desktopTracker[kWinModule.currentDesktop()-1];
  TaskVector::iterator tit = tv.begin();
  while(tit!=tv.end()) {
    startTimerFor(*tit);
    tit++;
  }
}

void Karm::updateTrackers(Task *task, DesktopListType desktopList)
{
  // if no desktop is marked, disable auto tracking for this task
  if (desktopList.size()==0) {
    for (int i=0; i<16; i++) {
      TaskVector *v = &(desktopTracker[i]);
      TaskVector::iterator tit = std::find(v->begin(), v->end(), task);
      if (tit != v->end())
        desktopTracker[i].erase(tit);
    }

    return;
  }

  // If desktop contains entries then configure desktopTracker
  // If a desktop was disabled, it will not be stopped automatically.
  // If enabled: Start it now.
  if (desktopList.size()>0) {
    for (int i=0; i<16; i++) {
      TaskVector& v = desktopTracker[i];
      TaskVector::iterator tit = std::find(v.begin(), v.end(), task);
      // Is desktop i in the desktop list?
      if (std::find(desktopList.begin(), desktopList.end(), i) != desktopList.end()) {
        if (tit == v.end())  // not yet in start vector
          v.push_back(task); // track in desk i
      }
      else { // delete it
        if (tit != v.end())  // not in start vector any more
          v.erase(tit); // so we delete it from desktopTracker
      }
    }
    // printTrackers();
    applyTrackers();
  }
}

void Karm::printTrackers() {
  TaskVector::iterator it;
  for (int i=0; i<16; i++) {
    TaskVector& start = desktopTracker[i];
    it = start.begin();
    while (it != start.end()) {
      it++;
    }
  }
}

bool Karm::parseLine(QString line, long *time, QString *name, int *level, DesktopListType* desktops)
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
  rest = rest.remove(0,index+1);

  bool ok;

  index = rest.find('\t'); // check for optional desktops string
  if (index >= 0) {
    *name = rest.left(index);    
    QString deskLine = rest.remove(0,index+1);

    // now transform the ds string (e.g. "3", or "1,4,5") into
    // an DesktopListType
    QString ds;
    int d;
    int commaIdx = deskLine.find(',');
    while (commaIdx >= 0) {
      ds = deskLine.left(commaIdx);
      d = ds.toInt(&ok);
      if (!ok)
	return false;
      
      desktops->push_back(d);
      deskLine.remove(0,commaIdx+1);
      commaIdx = deskLine.find(',');
    }

    d = deskLine.toInt(&ok);

    if (!ok)
      return false;

    desktops->push_back(d);
  }
  else {
    *name = rest.remove(0,index+1);  
  }

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
  for (QListViewItem *child =firstChild(); child; child = child->nextSibling())
    writeTaskToFile(&stream, child, 1);

  f.close();
}

void Karm::writeTaskToFile(QTextStream *strm, QListViewItem *item, int level)
{
  Task * task = (Task *) item;
  //lukas: correct version for non-latin1 users
  QString _line = QString::fromLatin1("%1\t%2\t%3").arg(level).
          arg(task->totalTime()).arg(task->name());

  DesktopListType d = task->getDesktops();
  int dsize = d.size();
  if (dsize>0) {
    _line += '\t';
    for (int i=0; i<dsize-1; i++) {
      _line += QString::number(d[i]);
      _line += ',';
    }
    _line += QString::number(d[dsize-1]);
  }
  *strm << _line << "\n";

  QListViewItem * child;
  for (child=item->firstChild(); child; child=child->nextSibling()) {
    writeTaskToFile(strm, child, level+1);
  }
}

void Karm::startCurrentTimer()
{
  startTimerFor((Task *) currentItem());
}

void Karm::startTimerFor(Task* item)
{
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
  QListViewItemIterator item( firstChild());
  for ( ; item.current(); ++item ) {
    Task * task = (Task *) item.current();
    long sessionTime = task->sessionTime();
    long totalTime   = task->totalTime();
		long newTotal = totalTime - sessionTime;
		long totalDiff = totalTime - newTotal;
    task->setSessionTime(0);
    task->setTotalTime( newTotal );
		sessionTimeChanged( -sessionTime, -totalDiff );
  }
}

void Karm::stopTimerFor(Task* item)
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
  stopTimerFor((Task *) currentItem());
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
    startCurrentTimer();
  }
  else {
    stopCurrentTimer();
  }
}

void Karm::minuteUpdate()
{
  addTimeToActiveTasks(1);
}

void Karm::addTimeToActiveTasks(int minutes)
{
  for(unsigned int i=0; i<activeTasks.count();i++) {
    Task *task = activeTasks.at(i);
    QListViewItem *item = task;
    while (item) {
      ((Task *) item)->incrementTime(minutes);
      emit( sessionTimeChanged( minutes, minutes ) );
      item = item->parent();
    }
  }
}

void Karm::newTask()
{
  newTask(i18n("New Task"), 0);
}

void Karm::newTask(QString caption, QListViewItem *parent)
{
  AddTaskDialog *dialog = new AddTaskDialog(caption, false);
  int result = dialog->exec();

  if (result == QDialog::Accepted) {
    QString taskName = i18n("Unnamed Task");
    if (!dialog->taskName().isEmpty()) {
      taskName = dialog->taskName();
    }

    long total, totalDiff, session, sessionDiff;
    total = totalDiff = session = sessionDiff = 0;
    DesktopListType desktopList;
    dialog->status( &total, &totalDiff, &session, &sessionDiff, &desktopList);
    Task *task;
    if (parent == 0)
      task = new Task(taskName, total, session, desktopList, this);
    else
      task = new Task(taskName, total, session, desktopList, parent);

    updateParents( (QListViewItem *) task, totalDiff, sessionDiff );
    setCurrentItem(task);
    setSelected(task, true);
    emit( sessionTimeChanged( sessionDiff, totalDiff ) );
  }
  delete dialog;
}

void Karm::newSubTask()
{
  QListViewItem *item = currentItem();
  if(!item)
    return;
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

  DesktopListType desktops = task->getDesktops();
  AddTaskDialog *dialog = new AddTaskDialog(i18n("Edit Task"), true, &desktops);
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
    total = totalDiff = session = sessionDiff = 0;
    DesktopListType desktopList;
    dialog->status( &total, &totalDiff, &session, &sessionDiff, &desktopList);

    task->setTotalTime( total);
    task->setSessionTime( session );

    // If all available desktops are checked, disable auto tracking,
    // since it makes no sense to track for every desktop.
    if (desktopList.size() == (unsigned int)desktopCount)
      desktopList.clear();

    task->setDesktopList(desktopList);

    if( sessionDiff || totalDiff ) {
      emit sessionTimeChanged( sessionDiff, totalDiff );
    }

    // Update the parents for this task.
    updateParents( (QListViewItem *) task, totalDiff, sessionDiff );
    updateTrackers(task, desktopList);

    emit updateButtons();
  }
  delete dialog;
}

void Karm::updateParents( QListViewItem* task, long totalDiff, long sessionDiff )
{
  QListViewItem *item = task->parent();
  while (item) {
    Task *parrentTask = (Task *) item;
    parrentTask->setTotalTime(parrentTask->totalTime()+totalDiff);
    parrentTask->setSessionTime(parrentTask->sessionTime()+sessionDiff);
    item = item->parent();
    sessionTimeChanged( sessionDiff, totalDiff );
  }
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
    stopTimerFor(item);

    // Stop idle detection if no more counters is running
    if (activeTasks.count() == 0) {
      _idleTimer->stopIdleDetection();
      emit timerInactive();
    }
    emit tasksChanged( activeTasks );

    long sessionTime = item->sessionTime();
    long totalTime   = item->totalTime();
    updateParents( item, -totalTime, -sessionTime );
    
    DesktopListType desktopList;
    updateTrackers(item, desktopList); // remove from tracker list

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
    emit( sessionTimeChanged( -sessionTime, -totalTime ) );
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
