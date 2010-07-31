/*
    This file is part of Akregator.

    Copyright (C) 2005 Frank Osterfeld <frank.osterfeld@kdemail.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include "dragobjects.h"
#include "feed.h"

#include <tqcstring.h>

namespace Akregator {

class Article;

ArticleDrag::ArticleDrag(const TQValueList<Article>& articles, TQWidget* dragSource, const char* name)
: KURLDrag(articleURLs(articles), dragSource, name), m_items(articlesToDragItems(articles))
{}

bool ArticleDrag::canDecode(const TQMimeSource* e)
{
    return e->provides("akregator/articles");
}

bool ArticleDrag::decode(const TQMimeSource* e, TQValueList<ArticleDragItem>& articles)
{
    articles.clear();
    TQByteArray array = e->encodedData("akregator/articles");
    
    TQDataStream stream(array, IO_ReadOnly);

    while (!stream.atEnd())
    {
        ArticleDragItem i;
        stream >> i.feedURL;
        stream >> i.guid;
        articles.append(i);
    }

    return true;
}

const char* ArticleDrag::format(int i) const
{
    if (i == 0)
        return "text/uri-list";
    else if (i == 1)
        return "akregator/articles";

    return 0;
}

TQByteArray ArticleDrag::encodedData(const char* mime) const
{
    TQCString mimetype(mime);
    if (mimetype == "akregator/articles")
    {
        TQByteArray ba;
        TQDataStream stream(ba, IO_WriteOnly);

        TQValueList<ArticleDragItem>::ConstIterator end = m_items.end();
        for (TQValueList<ArticleDragItem>::ConstIterator it = m_items.begin(); it != end; ++it)
        {
            stream << (*it).feedURL;
            stream << (*it).guid;
        } 
        return ba;
    }
    else
    {
        return KURLDrag::encodedData(mime);
    }
}

TQValueList<ArticleDragItem> ArticleDrag::articlesToDragItems(const TQValueList<Article>& articles)
{
    TQValueList<ArticleDragItem> items;
    
    TQValueList<Article>::ConstIterator end(articles.end());

    for (TQValueList<Article>::ConstIterator it = articles.begin(); it != end; ++it)
    {
        ArticleDragItem i;
        i.feedURL = (*it).feed() ? (*it).feed()->xmlUrl() : "";
        i.guid = (*it).guid();
        items.append(i);
    }

    return items;
}

KURL::List ArticleDrag::articleURLs(const TQValueList<Article>& articles)
{
    KURL::List urls;
    TQValueList<Article>::ConstIterator end(articles.end());
    for (TQValueList<Article>::ConstIterator it = articles.begin(); it != end; ++it)
        urls.append((*it).link());
    return urls;
}

} // namespace Akregator
