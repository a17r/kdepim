#ifndef KARM_TRAY_H
#define KARM_TRAY_H

#include <qptrvector.h>
#include <qpixmap.h>
#include <qptrlist.h>
#include "task.h"
// experiement
// #include <kpopupmenu.h>
#include <ksystemtray.h>

class QPopupMenu;
class QTimer;
class KSystemTray;
class KarmWindow;
// experiment
// class KPopupMenu;

class KarmTray : public KSystemTray
{
  Q_OBJECT

  public:
    KarmTray(KarmWindow * parent);
    ~KarmTray();

  private:
    int _activeIcon;
    static QPtrVector<QPixmap> *icons;
    QTimer *_taskActiveTimer;

  public slots:
    void startClock();
    void stopClock();
    void resetClock();
    void updateToolTip( QPtrList<Task> activeTasks);
    void initToolTip();

  protected slots:
    void advanceClock();
    
  // experiment
  /*
    void insertTitle(QString title);

  private:
    KPopupMenu *trayPopupMenu;
    QPopupMenu *trayPopupMenu2;
    */
};

#endif


