/*
 * Copyright (C) 2003 Helge Deller <deller@kde.org>
 */

#include <qstringlist.h>

#include <ktrader.h>
#include <klocale.h>
#include <kdebug.h>

#include "kmobileclient.h"

#define KMOBILECLIENT_DEBUG_AREA 0
#define PRINT_DEBUG kdDebug(KMOBILECLIENT_DEBUG_AREA) << "KMobileClient: "

KMobileClient::KMobileClient()
    : DCOPClient()
{
  // initialize Application and Object of remote DCOP-aware KMobile application
  m_kmobileApp = "kmobile";
  m_kmobileObj = "kmobileIface";

  bool ok = attach();
  PRINT_DEBUG << QString("attached to DCOP server %1\n").arg(ok?"sucessful.":"failed.");

  isKMobileAvailable();
}

KMobileClient::~KMobileClient()
{
  detach();
  PRINT_DEBUG << QString("detached from server\n");
}


bool KMobileClient::isKMobileAvailable()
{
  bool available = isApplicationRegistered(m_kmobileApp);
  PRINT_DEBUG << QString("KMobile DCOP server: %1\n").arg(available?"available.":"not available");
  if (!available)
	startKMobileApplication();
  return available;
}

bool KMobileClient::startKMobileApplication()
{
  QByteArray data;
  QDataStream arg(data, IO_WriteOnly);
  arg << QString("kmobile") << QStringList();
  QCString replyType; 
  QByteArray replyData;
  bool ok = call("klauncher", "klauncher", "kdeinit_exec_wait(QString,QStringList)", data, replyType, replyData);
  PRINT_DEBUG << QString("DCOP-CALL to klauncher: %1\n").arg(ok?"ok.":"failed.");
  return ok;
}


/**
 * DCOP - USAGE
 */

#define USE_EVENTLOOP true
#define TIMEOUT (-1)


#define PREPARE( FUNC, PARAMS ) \
  QByteArray data; \
  QDataStream arg(data, IO_WriteOnly); \
  arg << PARAMS; \
  QCString replyType; \
  QByteArray replyData; \
  bool ok = call(m_kmobileApp, m_kmobileObj, FUNC, data, replyType, replyData, USE_EVENTLOOP, TIMEOUT); \
  PRINT_DEBUG << QString("DCOP-CALL to %1: %2\n").arg(FUNC).arg(ok?"ok.":"failed.")
  
#define RETURN_TYPE( FUNC, PARAMS, RETURN_TYPE ) \
  PREPARE( FUNC, PARAMS ); \
  QDataStream reply(replyData, IO_ReadOnly); \
  RETURN_TYPE ret; \
  if (ok) \
	reply >> ret; \
  return ret;

#define RETURN_TYPE_DEFAULT( FUNC, PARAMS, RETURN_TYPE, RETURN_DEFAULT ) \
  PREPARE( FUNC, PARAMS ); \
  QDataStream reply(replyData, IO_ReadOnly); \
  RETURN_TYPE ret = RETURN_DEFAULT; \
  if (ok) \
	reply >> ret; \
  return ret;

#define RETURN_QSTRING( FUNC, PARAMS ) \
  RETURN_TYPE( FUNC, PARAMS, QString )

#define RETURN_BOOL( FUNC, PARAMS ) \
  RETURN_TYPE_DEFAULT( FUNC, PARAMS, bool, false )

#define RETURN_INT( FUNC, PARAMS ) \
  RETURN_TYPE_DEFAULT( FUNC, PARAMS, int , 0 )



QStringList KMobileClient::deviceNames()
{
  RETURN_TYPE( "deviceNames()", QString::fromLatin1(""), QStringList );
}

void KMobileClient::removeDevice( QString deviceName )
{
  PREPARE( "removeDevice(QString)", deviceName );
  Q_UNUSED(ok);
}

void KMobileClient::configDevice( QString deviceName )
{
  PREPARE( "configDevice(QString)", deviceName );
  Q_UNUSED(ok);
}


bool KMobileClient::connectDevice( QString deviceName )
{
  RETURN_BOOL( "connectDevice(QString)", deviceName );
}

bool KMobileClient::disconnectDevice( QString deviceName )
{
  RETURN_BOOL( "disconnectDevice(QString)", deviceName );
}

bool KMobileClient::connected( QString deviceName )
{
  RETURN_BOOL( "connected(QString)", deviceName );
}


QString KMobileClient::deviceClassName( QString deviceName )
{
  RETURN_QSTRING( "deviceClassName(QString)", deviceName );
}

QString KMobileClient::deviceName( QString deviceName )
{
  RETURN_QSTRING( "deviceName(QString)", deviceName );
}

QString KMobileClient::revision( QString deviceName )
{
  RETURN_QSTRING( "revision(QString)", deviceName );
}

int KMobileClient::classType( QString deviceName )
{
  RETURN_INT( "classType(QString)", deviceName );
}

int KMobileClient::capabilities( QString deviceName )
{
  RETURN_INT( "capabilities(QString)", deviceName );
}

QString KMobileClient::nameForCap( QString deviceName, int cap )
{
  RETURN_QSTRING( "nameForCap(QString,int)", deviceName << cap );
}

QString KMobileClient::iconFileName( QString deviceName )
{
  RETURN_QSTRING( "iconFileName(QString)", deviceName );
}

int KMobileClient::numAddresses( QString deviceName )
{
  RETURN_INT( "numAddresses(QString)", deviceName );
}

QString KMobileClient::readAddress( QString deviceName, int index )
{
  RETURN_QSTRING( "readAddress(QString,int)", deviceName << index );
}

bool KMobileClient::storeAddress( QString deviceName, int index, QString vcard, bool append )
{
  RETURN_BOOL( "storeAddress(QString,int,QString,bool)", deviceName << index << vcard << append );
}

int KMobileClient::numCalendarEntries( QString deviceName )
{
  RETURN_INT( "numCalendarEntries(QString)", deviceName );
}

int KMobileClient::numNotes( QString deviceName )
{
  RETURN_INT( "numNotes(QString)", deviceName );
}

QString KMobileClient::readNote( QString deviceName, int index )
{
  RETURN_QSTRING( "readNote(QString,int)", deviceName << index );
}

bool KMobileClient::storeNote( QString deviceName, int index, QString note )
{
  RETURN_BOOL( "storeNote(QString,int,QString)", deviceName << index << note );
}


#include "kmobileclient.moc"
