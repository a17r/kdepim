/*
 * Copyright (C) 2003 Helge Deller <deller@kde.org>
 */

#ifndef _KMOBILEVIEW_H_
#define _KMOBILEVIEW_H_

#include <qiconview.h>

#include "kmobileiface.h"

class KConfig;
class KMobileItem;

/**
 * This is the main view class for kmobile.  Most of the non-menu,
 * non-toolbar, and non-statusbar (e.g., non frame) GUI code should go
 * here.
 *
 * This kmobile uses an HTML component as an example.
 *
 * @short Main view
 * @author Helge Deller <deller@kde.org>
 * @version 0.1
 */
class KMobileView : public QIconView, public kmobileIface
{
    Q_OBJECT
public:
    KMobileView(QWidget *parent, KConfig *_config);
    virtual ~KMobileView();

    /**
     * DCOP implementation
     */
    QStringList deviceNames();

    void removeDevice( QString deviceName );
    void configDevice( QString deviceName );

    bool connectDevice( QString deviceName );
    bool disconnectDevice( QString deviceName );
    bool connected( QString deviceName );

    QString deviceClassName( QString deviceName );
    QString deviceName( QString deviceName );
    QString revision( QString deviceName );
    int classType( QString deviceName );

    int capabilities( QString deviceName );
    QString nameForCap( QString deviceName, int cap );

    QString iconFileName( QString deviceName );

    int     numAddresses( QString deviceName );
    QString readAddress( QString deviceName, int index );
    bool    storeAddress( QString deviceName, int index, QString vcard, bool append );

    int numCalendarEntries( QString deviceName );

    int numNotes( QString deviceName );
    QString readNote( QString deviceName, int index );
    bool storeNote( QString deviceName, int index, QString note );

public:
    void saveAll();
    void restoreAll();

protected:
    KMobileItem * findDevice( const QString &deviceName ) const;

protected slots:
    void slotDoubleClicked( QIconViewItem * item );

signals:
    /**
     * Use this signal to change the content of the statusbar
     */
    void signalChangeStatusbar(const QString& text);

private:
    KConfig *m_config;

};

#endif // _KMOBILEVIEW_H_
