#include "krsslocalresource.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "util.h"

#include <QtDBus/QDBusConnection>
#include <KFileDialog>
#include <KDebug>
#include <KStandardDirs>
#include <KLocale>
#include <QtXml/QXmlStreamReader>
#include <QtXml/QXmlStreamWriter>
#include <QMessageBox>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <krss/rssitem.h>
#include <krssresource/krssresource_export.h>

using namespace Akonadi;
using namespace KRssResource;
using namespace boost;

KRssLocalResource::KRssLocalResource( const QString &id )
  : ResourceBase( id )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  //policy.setCacheTimeout( CACHE_TIMEOUT );
  //policy.setIntervalCheckTime( INTERVAL_CHECK_TIME );

  policy.setInheritFromParent( false );
  policy.setSyncOnDemand( false );
  policy.setLocalParts( QStringList() << KRss::Item::HeadersPart << KRss::Item::ContentPart );
  
  
  //changeRecorder()->fetchCollection( true );
  
  changeRecorder()->itemFetchScope().fetchFullPayload( false );
  //changeRecorder()->itemFetchScope().fetchAllAttributes( true );
  
}

KRssLocalResource::~KRssLocalResource()
{
}

QString KRssLocalResource::mimeType()
{
  return QLatin1String("application/rss+xml");
}

void KRssLocalResource::retrieveCollections()
{
  // TODO: this method is called when Akonadi wants to have all the
  // collections your resource provides.
  // Be sure to set the remote ID and the content MIME types
  
    const QString path = Settings::self()->path();
    
    /* We'll parse the opml file */
    QFile file( path );
    /* If we can't open it, let's show an error message. */
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
	error(i18n("Couldn't open ") + path);
	return;
    }
    
    QXmlStreamReader reader( &file );
    
    OpmlReader parser;
        
    while ( !reader.atEnd() ) {
        reader.readNext();

        if ( reader.isStartElement() ) {
	    //check if the file is formatted opml, before parsing it
	    //TODO: move this checking to inside the parser.readOpml implementation
            if ( reader.name().toString().toLower() == QLatin1String("opml") ) {
                kDebug() << "OPML version" << reader.attributes().value( QLatin1String("version") ).toString();
                parser.readOpml( reader );
            }
            else {
                reader.raiseError( i18n ( "The file is not a valid OPML document." ) );
            }
        }
    }    
    
    QList<shared_ptr<const ParsedNode> > parsedNodes = parser.topLevelNodes();
    
    // create a top-level collection
    Collection top;
    top.setParent( Collection::root() );
    top.setRemoteId( path );
    top.setName( path );
    top.setContentMimeTypes( QStringList( Collection::mimeType() ) );
    
    //it customizes the root collection with an opml icon
    top.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setIconName( QString("application-opml+xml") );
    //TODO: modify CMakeLists.txt so that it installs the icon
    
    Collection::List list;
    list = buildCollectionTree(parser.topLevelNodes(), list, top); 
      
    collectionsRetrieved( list );

}

Collection::List KRssLocalResource::buildCollectionTree( QList<shared_ptr<const ParsedNode> > listOfNodes, 
				   Collection::List &list, Collection &parent)
{
    list << parent;
  
    foreach(const shared_ptr<const ParsedNode> parsedNode, listOfNodes) {
      if (!parsedNode->isFolder()) {
	    Collection c = (static_pointer_cast<const ParsedFeed>(parsedNode))->toAkonadiCollection();
	    c.setContentMimeTypes( c.contentMimeTypes() );
	    c.setParent( parent );

	    c.setCachePolicy( policy );
	    
	    //it customizes the collection with an rss icon
	    c.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setIconName( QString("application-rss+xml") );
	    
	    list << c;
	}
	else {
	    shared_ptr<const ParsedFolder> parsedFolder = static_pointer_cast<const ParsedFolder>(parsedNode);
	    Collection folder;
	    folder.setParent( parent );
	    folder.setName( parsedFolder->title() );
	    folder.setRemoteId( Settings::self()->path() + parsedFolder->title() );
	    folder.setContentMimeTypes( QStringList( Collection::mimeType() ) );
	    list = buildCollectionTree( parsedFolder->children(), list, folder );
	}
    }
  
    return list;
}

void KRssLocalResource::retrieveItems( const Akonadi::Collection &collection )
{   
// TODO: this method is called when Akonadi wants to know about all the
// items in the given collection. You can but don't have to provide all the
// data for each item, remote ID and MIME type are enough at this stage.
// Depending on how your resource accesses the data, there are several
// different ways to tell Akonadi when you are done.

      Syndication::Loader * const loader = Syndication::Loader::create();
      connect( loader, SIGNAL( loadingComplete( Syndication::Loader*, Syndication::FeedPtr, Syndication::ErrorCode ) ),
		this, SLOT( slotLoadingComplete( Syndication::Loader*, Syndication::FeedPtr, Syndication::ErrorCode ) ) );
      KUrl xmlUrl( collection.remoteId() ); 
      loader->loadFrom( xmlUrl );
      
}

void KRssLocalResource::slotLoadingComplete(Syndication::Loader* loader, Syndication::FeedPtr feed, 
					    Syndication::ErrorCode status)
{
     Q_UNUSED(loader);
     
     if (status != Syndication::Success) {
	    kWarning() << "Error while parsing xml file";
	    itemsRetrievalDone();
	    return;
     }

     m_syndItems = feed->items();
     Akonadi::Item::List items;
     foreach ( const Syndication::ItemPtr& syndItem, m_syndItems ) {
	  Akonadi::Item item( mimeType() );
	  item.setRemoteId( syndItem->id() );
	  item.setPayload<KRss::RssItem>( Util::fromSyndicationItem( syndItem ) );
	  item.setFlag( KRss::RssItem::flagNew() );
	  items << item;
     }

     itemsRetrieved( items );
 
}

bool KRssLocalResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
    
  // TODO: this method is called when Akonadi wants more data for a given item.
  // You can only provide the parts that have been requested but you are allowed
  // to provide all in one go

  itemRetrieved( item );
  return true;
}

void KRssLocalResource::aboutToQuit()
{
  // TODO: any cleanup you need to do while there is still an active
  // event loop. The resource will terminate after this method returns
}

void KRssLocalResource::configure( WId windowId )
{
  Q_UNUSED( windowId );

  const QString oldPath = Settings::self()->path();
  
  KUrl startUrl;
  if ( oldPath.isEmpty() )
    startUrl = KUrl( QDir::homePath() );
  else
    startUrl = KUrl( oldPath );

  const QString title = i18nc("@title:window", "Select an OPML Document");
  QString newPath = KFileDialog::getOpenFileName( startUrl, QLatin1String("*.opml|") + i18n("OPML Document (*.opml)"),
                                              0, title );
  
  if ( newPath.isEmpty() )
    newPath = KStandardDirs::locateLocal( "appdata", QLatin1String("feeds.opml") );
    
  Settings::self()->setPath( newPath );
  Settings::self()->writeConfig();
  synchronize();
  
}

void KRssLocalResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  Q_UNUSED( collection );
  
  changeCommitted( item );
}

void KRssLocalResource::itemRemoved( const Akonadi::Item &item )
{  
  changeCommitted( item );
}

void KRssLocalResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( item );
  Q_UNUSED( parts );

  // TODO: this method is called when somebody else, e.g. a client application,
  // has changed an item managed by your resource.
}

void KRssLocalResource::collectionChanged(const Akonadi::Collection& collection)
{
  using namespace Akonadi;
  
  Q_UNUSED( collection );
  
  // fetching all collections containing rss feeds recursively, starting at the root collection
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
  job->fetchScope().setContentMimeTypes( QStringList() << mimeType() );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( fetchCollectionsFinished( KJob* ) ) );
  
}

void KRssLocalResource::fetchCollectionsFinished(KJob *job) {
  
   if ( job->error() ) {
     qDebug() << "Error occurred";
     return;
   }

   CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );

   const Collection::List collections = fetchJob->collections();
  
   const QString path = Settings::self()->path();
  
   writeFeedsToOpml( path, collections );
}

void KRssLocalResource::writeFeedsToOpml(const QString &path, const QList<Akonadi::Collection>& feeds)
{
  QFile file( path );
  /* If we can't open it, let's show an error message. */
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      error(i18n("Couldn't open ") + path);
      return;
  }
        
  QXmlStreamWriter writer( &file );
  writer.setAutoFormatting( true );
  writer.writeStartDocument();
  OpmlWriter::writeOpml( writer, Util::toParsedFeedList( feeds ));
  writer.writeEndDocument();

}



AKONADI_RESOURCE_MAIN( KRssLocalResource )

#include "krsslocalresource.moc"
