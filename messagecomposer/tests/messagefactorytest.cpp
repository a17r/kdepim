/*
  Copyright (C) 2010 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#include "messagefactorytest.h"

#include "cryptofunctions.h"

#include <messagecore/stringutil.h>
#include <messagecore/nodehelper.h>

#include "messagecomposer/composer.h"
#include "messagecomposer/messagefactory.h"
#include "messagecomposer/globalpart.h"
#include "messagecomposer/infopart.h"
#include <messagecomposer/messageinfo.h>
#include "messagecomposer/textpart.h"

#include "testhtmlwriter.h"
#include "testcsshelper.h"
#include <messageviewer/nodehelper.h>
#include <messageviewer/objecttreeparser.h>

#include "qtest_messagecomposer.h"
#include <kmime/kmime_dateformatter.h>

#include <kpimidentities/identitymanager.h>
#include <qtest_kde.h>
#include <QDateTime>
using namespace Message;

QTEST_KDEMAIN( MessageFactoryTest, NoGUI )

void MessageFactoryTest::testCreateReply()
{
  KMime::Message::Ptr msg = createTestMessage();
  KPIMIdentities::IdentityManager* identMan = new KPIMIdentities::IdentityManager;

  MessageFactory factory( msg, 0 );
  factory.setIdentityManager( identMan );

  MessageFactory::MessageReply reply =  factory.createReply();
  QVERIFY( reply.replyAll = true );
  kDebug() << reply.msg->body();

  QDateTime date = msg->date()->dateTime().dateTime();
  QString datetime = KGlobal::locale()->formatDate( date.date(), KLocale::LongDate );
  datetime += QLatin1String( " " ) + KGlobal::locale()->formatTime( date.time(), true );
  QString replyStr = QString::fromLatin1( "On " + datetime.toLatin1() + " you wrote:\n> All happy families are alike; each unhappy family is unhappy in its own way.\n" );
  QVERIFY( reply.msg->subject()->asUnicodeString() == QLatin1String( "Re: Test Email Subject" ) );
  QVERIFY( reply.msg->body() == replyStr.toLatin1() );
  
}

void MessageFactoryTest::testCreateForward()
{
  KMime::Message::Ptr msg = createTestMessage();
  KPIMIdentities::IdentityManager* identMan = new KPIMIdentities::IdentityManager;

  MessageFactory factory( msg, 0 );
  factory.setIdentityManager( identMan );

  KMime::Message::Ptr fw =  factory.createForward();

  QDateTime date = msg->date()->dateTime().dateTime();
  QString datetime = KGlobal::locale()->formatDate( date.date(), KLocale::LongDate );
  datetime += QLatin1String( ", " ) + KGlobal::locale()->formatTime( date.time(), true );

  QString fwdMsg = QString::fromLatin1( "\n"
                                        "----------  Forwarded Message  ----------\n"
                      "\n"
                      "Subject: Test Email Subject\n"
                      "Date: %1\n"
                      "From: me@me.me\n"
                      "To: you@you.you\n"
                      "\n"
                      "All happy families are alike; each unhappy family is unhappy in its own way.\n"
                      "-----------------------------------------" );
  fwdMsg = fwdMsg.arg( datetime );
  
  QString fwdStr = QString::fromLatin1( "On " + datetime.toLatin1() + " you wrote:\n> All happy families are alike; each unhappy family is unhappy in its own way.\n" );
  QVERIFY( fw->subject()->asUnicodeString() == QLatin1String( "Fwd: Test Email Subject" ) );
  QVERIFY( fw->body() == fwdMsg.toLatin1() );

}


void MessageFactoryTest::testCreateRedirect()
{
  KMime::Message::Ptr msg = createTestMessage();
  KPIMIdentities::IdentityManager* identMan = new KPIMIdentities::IdentityManager;

  MessageFactory factory( msg, 0 );
  factory.setIdentityManager( identMan );

  QString redirectTo = QLatin1String("redir@redir.com");
  KMime::Message::Ptr rdir =  factory.createRedirect( redirectTo );

  QDateTime date = rdir->date()->dateTime().dateTime();
  QString datetime = KGlobal::locale()->formatDate( date.date(), KLocale::LongDate );
  datetime = rdir->date()->asUnicodeString();

  kDebug() << rdir->encodedContent();
  
  QString msgId = MessageCore::StringUtil::generateMessageId( msg->sender()->asUnicodeString(), QString() );

  QRegExp rx( QString::fromAscii( "Resent-Message-ID: ([^\n]*)" ) );
  rx.indexIn( QString::fromAscii( rdir->head() ) );
  
  QString baseline = QString::fromLatin1( "From: me@me.me\n"
                                          "Subject: Test Email Subject\n"
                                          "Date: %2\n"
                                          "Disposition-Notification-To: me@me.me\n"
                                          "MIME-Version: 1.0\n"
                                          "Content-Transfer-Encoding: 7Bit\n"
                                          "Content-Type: text/plain; charset=\"us-ascii\"\n"
                                          "Resent-Message-ID: %3\n"
                                          "Resent-Date: %4\n"
                                          "To: %1\n"
                                          "Resent-To:  <>\n"
                                          "X-KMail-Redirect-From: me@me.me (by way of  <>)\n"
                                          "X-KMail-Recipients: redir@redir.com\n"
                                          "\n"
                                          "All happy families are alike; each unhappy family is unhappy in its own way." );
  baseline = baseline.arg( redirectTo ).arg( datetime ).arg( rx.cap(1) ).arg( datetime );

  kDebug() << baseline.toLatin1();

//   QString fwdStr = QString::fromLatin1( "On " + datetime.toLatin1() + " you wrote:\n> All happy families are alike; each unhappy family is unhappy in its own way.\n" );
  QVERIFY( rdir->subject()->asUnicodeString() == QLatin1String( "Test Email Subject" ) );
  QVERIFY( rdir->encodedContent() == baseline.toLatin1() );
}

void MessageFactoryTest::testCreateMDN()
{
  KMime::Message::Ptr msg = createTestMessage();
  KPIMIdentities::IdentityManager* identMan = new KPIMIdentities::IdentityManager;

  MessageFactory factory( msg, 0 );
  
  factory.setIdentityManager( identMan );

  MessageInfo::instance()->setMDNSentState( msg.get(), KMMsgMDNNone );
  KMime::Message::Ptr mdn = factory.createMDN( KMime::MDN::AutomaticAction, KMime::MDN::Displayed, KMime::MDN::SentAutomatically );

  QVERIFY( mdn );
  kDebug() << "mdn" << mdn->encodedContent();
/*
  // parse the result and make sure it is valid in various ways
  TestHtmlWriter testWriter;
  TestCSSHelper testCSSHelper;
  TestObjectTreeSource testSource( &testWriter, &testCSSHelper );
  MessageViewer::NodeHelper* nh = new MessageViewer::NodeHelper;
  MessageViewer::ObjectTreeParser otp( &testSource, nh, 0, false, false, true, 0 );
  MessageViewer::ProcessResult pResult( nh ); */

//   kDebug() << MessageCore::NodeHelper::firstChild( mdn->mainBodyPart() )->encodedContent();
//   kDebug() << MessageCore::NodeHelper::next(  MessageViewer::ObjectTreeParser::findType( mdn.get(), "multipart", "report", true, true ) )->body();


  QString mdnContent = QString::fromLatin1( "The message sent on %1 to %2 with subject \"%3\" has been displayed. "
                                "This is no guarantee that the message has been read or understood." );
  mdnContent = mdnContent.arg( KMime::DateFormatter::formatDate( KMime::DateFormatter::Localized, msg->date()->dateTime().dateTime().toTime_t() ) )
                         .arg( msg->to()->asUnicodeString() ).arg( msg->subject()->asUnicodeString() );

  kDebug() << "comparing with:" << mdnContent;
  
  QVERIFY( MessageCore::NodeHelper::next(  MessageViewer::ObjectTreeParser::findType( mdn.get(), "multipart", "report", true, true ) )->body() == mdnContent.toLatin1() );

}


KMime::Message::Ptr MessageFactoryTest::createTestMessage()
{
  Composer *composer = new Composer;
  composer->globalPart()->setFallbackCharsetEnabled( true );
  composer->infoPart()->setFrom( QString::fromLatin1( "me@me.me" ) );
  composer->infoPart()->setTo( QStringList( QString::fromLatin1( "you@you.you" ) ) );
  composer->textPart()->setWrappedPlainText( QString::fromLatin1( "All happy families are alike; each unhappy family is unhappy in its own way." ) );
  composer->infoPart()->setSubject( QLatin1String( "Test Email Subject" ) );
  composer->globalPart()->setMDNRequested( true );
  composer->exec();
  
  KMime::Message::Ptr message = KMime::Message::Ptr( composer->resultMessages().first() );
  delete composer;

  return message;
}