#include "distributionlistentryview.h"
#include "imagewidget.h"
#include <interfaces/core.h>

#include <libkdepim/resourceabc.h>

#include <kabc/addressbook.h>
#include <kabc/resource.h>

#include <kdialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kurllabel.h>

#include <tqbuttongroup.h>
#include <tqcombobox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqradiobutton.h>
#include <tqstringlist.h>
#include <tqvbuttongroup.h>

KAB::DistributionListEntryView::DistributionListEntryView( KAB::Core* core, TQWidget* parent ) : TQWidget( parent ), m_core( core ), m_emailGroup( 0 )
{
    m_mainLayout = new TQVBoxLayout( this );
    m_mainLayout->setSpacing( KDialog::spacingHint() );
    m_mainLayout->setMargin( KDialog::marginHint() );

    TQBoxLayout* headerLayout = new QHBoxLayout;
    headerLayout->setSpacing( KDialog::spacingHint() * 3 );

    m_imageLabel = new TQLabel( this );
    m_imageLabel->setAutoResize( true );
    headerLayout->addWidget( m_imageLabel, 0, Qt::AlignTop );

    m_addresseeLabel = new TQLabel( this );
    headerLayout->addWidget( m_addresseeLabel, 0, Qt::AlignTop );
    headerLayout->addStretch();

    m_mainLayout->addItem( headerLayout );

    TQBoxLayout* distLayout = new QHBoxLayout;
    distLayout->setSpacing( KDialog::spacingHint() );

    TQLabel* distLabel = new TQLabel( this );
    distLabel->setText( i18n( "<b>Distribution list:</b>" ) );
    distLabel->setAlignment( Qt::SingleLine );
    distLayout->addWidget( distLabel );

    m_distListLabel = new KURLLabel( this );
    distLabel->setBuddy( m_distListLabel );
    connect( m_distListLabel, TQT_SIGNAL( leftClickedURL( const TQString& ) ), 
             this, TQT_SIGNAL( distributionListClicked( const TQString& ) ) );
    distLayout->addWidget( m_distListLabel );
    distLayout->addStretch();
    m_mainLayout->addItem( distLayout );

    TQLabel* emailLabel = new TQLabel( this );
    emailLabel->setText( i18n( "<b>Email address to use in this list:</b>" ) );
    emailLabel->setAlignment( Qt::SingleLine );
    m_mainLayout->addWidget( emailLabel );

    TQBoxLayout* emailLayout = new QHBoxLayout;
    emailLayout->setSpacing( KDialog::spacingHint() );
    emailLayout->addSpacing( 30 );

    m_radioLayout = new QGridLayout;
    emailLayout->addItem( m_radioLayout );
    emailLayout->addStretch();
    m_mainLayout->addItem( emailLayout );

    TQBoxLayout* resourceLayout = new QHBoxLayout;
    resourceLayout->setSpacing( KDialog::spacingHint() );
    m_resourceLabel = new TQLabel( this );
    resourceLayout->addWidget( m_resourceLabel );
    resourceLayout->addStretch();

    m_mainLayout->addItem( resourceLayout );
    m_mainLayout->addStretch();
}

void KAB::DistributionListEntryView::emailButtonClicked( int id )
{
    const TQString email = m_idToEmail[ id ];
    if ( m_entry.email == email )
        return;
    m_list.removeEntry( m_entry.addressee, m_entry.email );
    m_entry.email = email;
    m_list.insertEntry( m_entry.addressee, m_entry.email );
    m_core->addressBook()->insertAddressee( m_list ); 
}

void KAB::DistributionListEntryView::clear()
{
    setEntry( KPIM::DistributionList(), KPIM::DistributionList::Entry() );
}

void KAB::DistributionListEntryView::setEntry( const KPIM::DistributionList& list, const KPIM::DistributionList::Entry& entry )
{    
    m_list = list;
    m_entry = entry;

    delete m_emailGroup;
    m_emailGroup = 0;

    TQPixmap pixmap;
    pixmap.convertFromImage( m_entry.addressee.photo().data() );
    m_imageLabel->setPixmap( pixmap.isNull() ? KGlobal::iconLoader()->loadIcon( "personal", KIcon::Desktop ) : pixmap );
    m_addresseeLabel->setText( i18n( "Formatted name, role, organization", "<qt><h2>%1</h2><p>%2<br/>%3</p></qt>" ).arg( m_entry.addressee.formattedName(), m_entry.addressee.role(), m_entry.addressee.organization() ) );
    m_distListLabel->setURL( m_list.name() );
    m_distListLabel->setText( m_list.name() );
    m_resourceLabel->setText( i18n( "<b>Address book:</b> %1" ).arg( m_entry.addressee.resource() ? m_entry.addressee.resource()->resourceName() : TQString() ) );
    m_resourceLabel->setAlignment( Qt::SingleLine );
 
    m_emailGroup = new TQVButtonGroup( this );
    m_emailGroup->setFlat( true );
    m_emailGroup->setExclusive( true );
    m_emailGroup->setFrameShape( TQFrame::NoFrame );

    const TQString preferred = m_entry.email.isNull() ? m_entry.addressee.preferredEmail() : m_entry.email;
    const TQStringList mails = m_entry.addressee.emails();
    m_idToEmail.clear();
    for ( TQStringList::ConstIterator it = mails.begin(); it != mails.end(); ++it )
    {
        TQRadioButton* button = new TQRadioButton( m_emailGroup );
        button->setText( *it );
        m_idToEmail.insert( m_emailGroup->insert( button ), *it );
        if ( *it == preferred )
            button->setChecked( true );
        button->setShown( true );
    }
    connect( m_emailGroup, TQT_SIGNAL( clicked( int ) ), 
             this, TQT_SLOT( emailButtonClicked( int ) ) ); 
    m_radioLayout->addWidget( m_emailGroup, 0, 0 );
    m_emailGroup->setShown( true );
    m_mainLayout->invalidate();
}


#include "distributionlistentryview.moc"
