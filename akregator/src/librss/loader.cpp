/*
 * loader.cpp
 *
 * Copyright (c) 2001, 2002, 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. For licensing and distribution details, check the
 * accompanying file 'COPYING'.
 */
#include "loader.h"
#include "document.h"

#include <kio/job.h>
#include <kprocess.h>
#include <kurl.h>

#include <qdom.h>
#include <qbuffer.h>
#include <qregexp.h>
#include <qstringlist.h>

using namespace RSS;

DataRetriever::DataRetriever()
{
}

DataRetriever::~DataRetriever()
{
}

struct FileRetriever::Private
{
   Private()
      : buffer(NULL),
        lastError(0)
   {
   }

   ~Private()
   {
      delete buffer;
   }

   QBuffer *buffer;
   int lastError;
};

FileRetriever::FileRetriever()
   : d(new Private)
{
}

FileRetriever::~FileRetriever()
{
   delete d;
}

void FileRetriever::retrieveData(const KURL &url)
{
   if (d->buffer)
      return;

   d->buffer = new QBuffer;
   d->buffer->open(IO_WriteOnly);

   KIO::Job *job = KIO::get(url, false, false);
   connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
                SLOT(slotData(KIO::Job *, const QByteArray &)));
   connect(job, SIGNAL(result(KIO::Job *)), SLOT(slotResult(KIO::Job *)));
   connect(job, SIGNAL(permanentRedirection(KIO::Job *, const KURL &, const KURL &)),
                SLOT(slotPermanentRedirection(KIO::Job *, const KURL &, const KURL &)));
}

int FileRetriever::errorCode() const
{
   return d->lastError;
}

void FileRetriever::slotData(KIO::Job *, const QByteArray &data)
{
   d->buffer->writeBlock(data.data(), data.size());
}

void FileRetriever::slotResult(KIO::Job *job)
{
   QByteArray data = d->buffer->buffer();
   data.detach();

   delete d->buffer;
   d->buffer = NULL;

   d->lastError = job->error();
   emit dataRetrieved(data, d->lastError == 0);
}

void FileRetriever::slotPermanentRedirection(KIO::Job *, const KURL &, const KURL &newUrl)
{
   emit permanentRedirection(newUrl);
}

struct OutputRetriever::Private
{
   Private() : process(NULL),
      buffer(NULL),
      lastError(0)
   {
   }

   ~Private()
   {
      delete process;
      delete buffer;
   }

   KShellProcess *process;
   QBuffer *buffer;
   int lastError;
};

OutputRetriever::OutputRetriever() :
   d(new Private)
{
}

OutputRetriever::~OutputRetriever()
{
   delete d;
}

void OutputRetriever::retrieveData(const KURL &url)
{
   // Ignore subsequent calls if we didn't finish the previous job yet.
   if (d->buffer || d->process)
      return;

   d->buffer = new QBuffer;
   d->buffer->open(IO_WriteOnly);

   d->process = new KShellProcess();
   connect(d->process, SIGNAL(processExited(KProcess *)),
                       SLOT(slotExited(KProcess *)));
   connect(d->process, SIGNAL(receivedStdout(KProcess *, char *, int)),
                       SLOT(slotOutput(KProcess *, char *, int)));
   *d->process << url.path();
   d->process->start(KProcess::NotifyOnExit, KProcess::Stdout);
}

int OutputRetriever::errorCode() const
{
   return d->lastError;
}

void OutputRetriever::slotOutput(KProcess *, char *data, int length)
{
   d->buffer->writeBlock(data, length);
}

void OutputRetriever::slotExited(KProcess *p)
{
   if (!p->normalExit())
      d->lastError = p->exitStatus();

   QByteArray data = d->buffer->buffer();
   data.detach();

   delete d->buffer;
   d->buffer = NULL;

   delete d->process;
   d->process = NULL;

   emit dataRetrieved(data, p->normalExit() && p->exitStatus() == 0);
}

struct Loader::Private
{
   Private() : retriever(NULL),
      lastError(0)
   {
   }

   ~Private()
   {
      delete retriever;
   }

   DataRetriever *retriever;
   int lastError;
   KURL discoveredFeedURL;
   KURL url;
};

Loader *Loader::create()
{
   return new Loader;
}

Loader *Loader::create(QObject *object, const char *slot)
{
   Loader *loader = create();
   connect(loader, SIGNAL(loadingComplete(Loader *, Document, Status)),
           object, slot);
   return loader;
}

Loader::Loader() : d(new Private)
{
}

Loader::~Loader()
{
   delete d;
}

void Loader::loadFrom(const KURL &url, DataRetriever *retriever)
{
   if (d->retriever != NULL)
      return;

   d->url=url;
   d->retriever = retriever;

   connect(d->retriever, SIGNAL(dataRetrieved(const QByteArray &, bool)),
           this, SLOT(slotRetrieverDone(const QByteArray &, bool)));

   d->retriever->retrieveData(url);
}

int Loader::errorCode() const
{
   return d->lastError;
}

const KURL &Loader::discoveredFeedURL() const
{
   return d->discoveredFeedURL;
}

void Loader::slotRetrieverDone(const QByteArray &data, bool success)
{
   d->lastError = d->retriever->errorCode();

   delete d->retriever;
   d->retriever = NULL;

   Document rssDoc;
   Status status = Success;

   if (success) {
      QDomDocument doc;

      /* Some servers insert whitespace before the <?xml...?> declaration.
       * QDom doesn't tolerate that (and it's right, that's invalid XML),
       * so we strip that.
       */

      const char *charData = data.data();
      int len = data.count();

      while (len && QChar(*charData).isSpace()) {
         --len;
         ++charData;
      }

      QByteArray tmpData;
      tmpData.setRawData(charData, len);

      if (doc.setContent(tmpData))
         rssDoc = Document(doc);
      else
      {
         discoverFeeds(tmpData);
         status = ParseError;
      }
      
      tmpData.resetRawData(charData, len);
   } else
      status = RetrieveError;

   emit loadingComplete(this, rssDoc, status);

   delete this;
}

void Loader::discoverFeeds(const QByteArray &data)
{
    QString str, s2;
    QTextStream ts( &str, IO_WriteOnly );
    ts << data.data();
    QRegExp rx( "(rel|REL)[^=]*=[^sAa]*(service.feed|alternate|ALTERNATE)([^>]*)(HREF|href)[^=]*=[^a-zA-Z0-9-_~,./$]*([^'\">\\s]*)", false);
    if (rx.search(str)!=-1)
        s2=rx.cap(5);
    else{
    // does not support Atom/RSS autodiscovery.. try finding feeds by brute force....
        int pos=0;
        QStringList feeds;
        QString host=d->url.host();
        rx.setPattern("(<A |<a )[^hH]*(HREF|href)[^=]*=[^a-zA-Z0-9-_~,./]*([^'\">\\s]*)");
        while ( pos >= 0 ) {
            pos = rx.search( str, pos );
            s2=rx.cap(3);
            if (s2.endsWith(".rdf")|s2.endsWith(".rss")|s2.endsWith(".xml"))
                    feeds.append(s2);
            if ( pos >= 0 ) {
                pos += rx.matchedLength();
            }
        }


        s2=feeds.first();
        KURL testURL;
        // loop through, prefer feeds on same host
        for ( QStringList::Iterator it = feeds.begin(); it != feeds.end(); ++it ) {
            testURL=*it;
            if (testURL.host()==host)
            {
                s2=*it;
                break;
            }
        }
    }
    if (s2.isNull())
        return;

    if (KURL::isRelativeURL(s2))
    {
        if (s2.startsWith("//"))
        {
            s2=s2.prepend(d->url.protocol()+":");
            d->discoveredFeedURL=s2;
        }
        else
        {
            d->discoveredFeedURL=d->url;
            d->discoveredFeedURL.addPath(s2);
        }
        d->discoveredFeedURL.cleanPath();
    }
    else
        d->discoveredFeedURL=s2;
    
    d->discoveredFeedURL.cleanPath();
}
    
#include "loader.moc"
// vim:noet:ts=4
