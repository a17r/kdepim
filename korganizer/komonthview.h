/* 	$Id$	 */

#ifndef _KOMONTHVIEW_H
#define _KOMONTHVIEW_H

#include <qlabel.h>
#include <qframe.h>
#include <qdatetime.h>
#include <qlistbox.h>
#include <qlayout.h>
#include <qintdict.h>
#include <qpushbutton.h>

#include <kapp.h>

#include "qdatelist.h"
#include "calendar.h"
#include "event.h"
#include "koeventview.h"
#include "ksellabel.h" 

class KONavButton: public QPushButton
{
  public:
    KONavButton( QPixmap pixmap, QWidget *parent, const char *name=0 ) :
      QPushButton(parent,name)
    {
      // We should probably check, if the pixmap is valid.
      setPixmap(pixmap);
    }; 
    
    QSizePolicy sizePolicy() const
    {
      return QSizePolicy(QSizePolicy::Minimum,QSizePolicy::MinimumExpanding);
    }
};


class EventListBoxItem: public QListBoxItem
{
 public:
  EventListBoxItem(const QString & s);
  void setText(const QString & s)
    { QListBoxItem::setText(s); }
  void setRecur(bool on) 
    { recur = on; }
  void setAlarm(bool on)
    { alarm = on; }
  const QPalette &palette () const { return myPalette; };
  void setPalette(const QPalette &p) { myPalette = p; };

 protected:
  virtual void paint(QPainter *);
  virtual int height(const QListBox *) const;
  virtual int width(const QListBox *) const;
 private:
  bool    recur;
  bool    alarm;
  QPixmap alarmPxmp;
  QPixmap recurPxmp;
  QPalette myPalette;
};

class KNoScrollListBox: public QListBox {
  Q_OBJECT
 public:
  KNoScrollListBox(QWidget *parent=0, const char *name=0);
  ~KNoScrollListBox() {}

 signals:
  void shiftDown();
  void shiftUp();
  void rightClick();

 protected slots:
  void keyPressEvent(QKeyEvent *);
  void keyReleaseEvent(QKeyEvent *);
  void mousePressEvent(QMouseEvent *);
};

class KSummaries: public KNoScrollListBox {
  Q_OBJECT
 public:
  KSummaries(QWidget    *parent, 
	     Calendar  *calendar, 
	     QDate       qd       = QDate::currentDate(),
	     int         index    = 0,
	     const char *name     = 0);
  ~KSummaries() {}
  QDate getDate() { return(myDate); }
  void setDate(QDate);
  void calUpdated();
  Event *getSelected();
  
  QSize minimumSizeHint() const;

 signals:
  void daySelected(int index);
  void editEventSignal(Event *);

 protected slots:
  void itemHighlighted(int);
  void itemSelected(int);

 private:
   QDate               myDate;
   int                 idx, itemIndex;
   Calendar          *myCal;
   QIntDict<Event> *currIdxs; 
};

class KOMonthView: public KOEventView {
   Q_OBJECT
 public:
   KOMonthView(Calendar *cal,
		QWidget    *parent   = 0, 
		const char *name     = 0,
		QDate       qd       = QDate::currentDate());
   ~KOMonthView();

   enum { EVENTADDED, EVENTEDITED, EVENTDELETED };

   /** Returns maximum number of days supported by the komonthview */
   virtual int maxDatesHint();

   /** Returns number of currently shown dates. */
   virtual int currentDateCount();

   /** returns the currently selected events */
   virtual QList<Incidence> getSelected();

   virtual void printPreview(CalPrinter *calPrinter,
                             const QDate &, const QDate &);

 public slots:
   virtual void updateView();
   virtual void updateConfig();
   virtual void selectDates(const QDateList);
   virtual void selectEvents(QList<Event> eventList);

   void changeEventDisplay(Event *, int);

 signals:
   void newEventSignal();  // From KOBaseView
   void newEventSignal(QDate);
   void newEventSignal(QDateTime, QDateTime);  // From KOBaseView
   void editEventSignal(Event *);  // From KOBaseView
   void deleteEventSignal(Event *);  // From KOBaseView
   void datesSelected(const QDateList);  // From KOBaseView

 protected slots:
   void resizeEvent(QResizeEvent *);
   void goBackYear();
   void goForwardYear();
   void goBackMonth();
   void goForwardMonth();
   void goBackWeek();
   void goForwardWeek();
   void daySelected(int index);
   void newEventSlot(int index);
   void doRightClickMenu();
//   void newEventSelected() { emit newEventSignal(daySummaries[*selDateIdxs.first()]->getDate()); };
   void processSelectionChange();

 protected:
   void viewChanged();
   
 private:
   // date range label.
   QLabel           *dispLabel;
   // day display vars
   QLabel           *dayNames[7];
   KSelLabel        *dayHeaders[42];
   KSummaries       *daySummaries[42];
   bool              shortdaynames;
   bool              weekStartsMonday;

   // display control vars
   KOEventPopupMenu *rightClickMenu;

   // state data.
   QDate             myDate;
   QDateList         selDates;
   Calendar        *myCal;
   QList<int>        selDateIdxs;
   QPalette          holidayPalette;
};

#endif
 
