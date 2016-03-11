/*
    Boost Software License - Version 1.0 - August 17th, 2003

    Permission is hereby granted, free of charge, to any person or organization
    obtaining a copy of the software and accompanying documentation covered by
    this license (the "Software") to use, reproduce, display, distribute,
    execute, and transmit the Software, and to prepare derivative works of the
    Software, and to permit third-parties to whom the Software is furnished to
    do so, all subject to the following:

    The copyright notices in the Software and this entire statement, including
    the above license grant, this restriction and the following disclaimer,
    must be included in all copies of the Software, in whole or in part, and
    all derivative works of the Software, unless such copies or derivative
    works are solely in the form of machine-executable object code generated by
    a source language processor.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
    SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
    FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.

*/

#include "invitationsettings.h"
#include "ui_invitationsettings.h"
#include "settings/globalsettings.h"
#include "pimcommon/widgets/configureimmutablewidgetutils.h"
using namespace PimCommon::ConfigureImmutableWidgetUtils;

#include <KLocalizedString>
#include <KMessageBox>

using namespace MessageViewer;

InvitationSettings::InvitationSettings( QWidget *parent )
    : QWidget( parent ), mInvitationUi( new Ui_InvitationSettings )
{
    mInvitationUi->setupUi( this );

    mInvitationUi->mDeleteInvitations->setText(
                i18n( GlobalSettings::self()->
                      deleteInvitationEmailsAfterSendingReplyItem()->label().toUtf8() ) );
    mInvitationUi->mDeleteInvitations->setWhatsThis(
                i18n( GlobalSettings::self()->
                      deleteInvitationEmailsAfterSendingReplyItem()->whatsThis().toUtf8() ) );
    connect( mInvitationUi->mDeleteInvitations, SIGNAL(toggled(bool)),
             SIGNAL(changed()) );

    mInvitationUi->mLegacyMangleFromTo->setWhatsThis(
                i18n( GlobalSettings::self()->legacyMangleFromToHeadersItem()->whatsThis().toUtf8() ) );
    connect( mInvitationUi->mLegacyMangleFromTo, SIGNAL(stateChanged(int)),
             this, SIGNAL(changed()) );

    mInvitationUi->mLegacyBodyInvites->setWhatsThis(
                i18n( GlobalSettings::self()->legacyBodyInvitesItem()->whatsThis().toUtf8() ) );
    connect( mInvitationUi->mLegacyBodyInvites, SIGNAL(toggled(bool)),
             this, SLOT(slotLegacyBodyInvitesToggled(bool)) );
    connect( mInvitationUi->mLegacyBodyInvites, SIGNAL(stateChanged(int)),
             this, SIGNAL(changed()) );

    mInvitationUi->mExchangeCompatibleInvitations->setWhatsThis(
                i18n( GlobalSettings::self()->exchangeCompatibleInvitationsItem()->whatsThis().toUtf8() ) );
    connect( mInvitationUi->mExchangeCompatibleInvitations, SIGNAL(stateChanged(int)),
             this, SIGNAL(changed()) );

    //Laurent BUG:257723: in kmail2 it's not possible to not send automatically.
    mInvitationUi->mAutomaticSending->hide();
    mInvitationUi->mAutomaticSending->setWhatsThis(
                i18n( GlobalSettings::self()->automaticSendingItem()->whatsThis().toUtf8() ) );
    connect( mInvitationUi->mAutomaticSending, SIGNAL(stateChanged(int)),
             this, SIGNAL(changed()) );
}

InvitationSettings::~InvitationSettings()
{
    delete mInvitationUi;
    mInvitationUi = 0;
}

void InvitationSettings::slotLegacyBodyInvitesToggled( bool on )
{
    if ( on ) {
        const QString txt = i18n( "<qt>Invitations are normally sent as attachments to "
                                  "a mail. This switch changes the invitation mails to "
                                  "be sent in the text of the mail instead; this is "
                                  "necessary to send invitations and replies to "
                                  "Microsoft Outlook.<br />But, when you do this, you no "
                                  "longer get descriptive text that mail programs "
                                  "can read; so, to people who have email programs "
                                  "that do not understand the invitations, the "
                                  "resulting messages look very odd.<br />People that have email "
                                  "programs that do understand invitations will still "
                                  "be able to work with this.</qt>" );
        KMessageBox::information( this, txt, QString(), QLatin1String("LegacyBodyInvitesWarning") );
    }
    // Invitations in the body are autosent in any case (no point in editing raw ICAL)
    // So the autosend option is only available if invitations are sent as attachment.
    mInvitationUi->mAutomaticSending->setEnabled( !mInvitationUi->mLegacyBodyInvites->isChecked() );
}

void InvitationSettings::doLoadFromGlobalSettings()
{
    loadWidget(mInvitationUi->mLegacyMangleFromTo, GlobalSettings::self()->legacyMangleFromToHeadersItem());
    mInvitationUi->mLegacyBodyInvites->blockSignals( true );
    loadWidget(mInvitationUi->mLegacyBodyInvites, GlobalSettings::self()->legacyBodyInvitesItem());
    mInvitationUi->mLegacyBodyInvites->blockSignals( false );
    loadWidget(mInvitationUi->mExchangeCompatibleInvitations, GlobalSettings::self()->exchangeCompatibleInvitationsItem());
    loadWidget(mInvitationUi->mAutomaticSending, GlobalSettings::self()->automaticSendingItem());
    //TODO verify it
    mInvitationUi->mAutomaticSending->setEnabled( !mInvitationUi->mLegacyBodyInvites->isChecked() );
    loadWidget(mInvitationUi->mDeleteInvitations, GlobalSettings::self()->deleteInvitationEmailsAfterSendingReplyItem());
}

void InvitationSettings::save()
{
    saveCheckBox(mInvitationUi->mLegacyMangleFromTo, GlobalSettings::self()->legacyMangleFromToHeadersItem());
    saveCheckBox(mInvitationUi->mLegacyBodyInvites, GlobalSettings::self()->legacyBodyInvitesItem());
    saveCheckBox(mInvitationUi->mExchangeCompatibleInvitations, GlobalSettings::self()->exchangeCompatibleInvitationsItem());
    saveCheckBox(mInvitationUi->mAutomaticSending, GlobalSettings::self()->automaticSendingItem());
    saveCheckBox(mInvitationUi->mDeleteInvitations, GlobalSettings::self()->deleteInvitationEmailsAfterSendingReplyItem());
}

QString InvitationSettings::helpAnchor() const
{
    return QString::fromLatin1( "configure-misc-invites" );
}

void InvitationSettings::doResetToDefaultsOther()
{
    const bool bUseDefaults = GlobalSettings::self()->useDefaults( true );
    loadWidget(mInvitationUi->mLegacyMangleFromTo, GlobalSettings::self()->legacyMangleFromToHeadersItem());
    mInvitationUi->mLegacyBodyInvites->blockSignals( true );
    loadWidget(mInvitationUi->mLegacyBodyInvites, GlobalSettings::self()->legacyBodyInvitesItem());
    mInvitationUi->mLegacyBodyInvites->blockSignals( false );
    loadWidget(mInvitationUi->mExchangeCompatibleInvitations, GlobalSettings::self()->exchangeCompatibleInvitationsItem());
    loadWidget(mInvitationUi->mAutomaticSending, GlobalSettings::self()->automaticSendingItem());
    //TODO verify it
    mInvitationUi->mAutomaticSending->setEnabled( !mInvitationUi->mLegacyBodyInvites->isChecked() );
    loadWidget(mInvitationUi->mDeleteInvitations, GlobalSettings::self()->deleteInvitationEmailsAfterSendingReplyItem());
    GlobalSettings::self()->useDefaults( bUseDefaults );
}
