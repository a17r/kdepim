/* This file is part of the KDE project

   Copyright (C) 1999, 2000 Rik Hemsley <rik@kde.org>
             (C) 1999, 2000 Wilco Greven <j.w.greven@student.utwente.nl>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef RMM_HEADER_H
#define RMM_HEADER_H

#include <qvaluelist.h>

#include <RMM_Enum.h>
#include <RMM_HeaderBody.h>
#include <RMM_Defines.h>

namespace RMM {

/**
 * An RHeader encapsulates an header name and an RHeaderBody.
 */
class RHeader : public RMessageComponent
{

#include "RMM_Header_generated.h"
        
    public:
        
        QCString headerName();
        RMM::HeaderType headerType();
        RHeaderBody * headerBody();

        void setName(const QCString & name);
        void setType(RMM::HeaderType t);
        void setBody(RHeaderBody * b);

    private:
 
        RHeaderBody * _newHeaderBody(RMM::HeaderType);
        void _replaceHeaderBody(RMM::HeaderType, RHeaderBody *);
        
        QCString        headerName_;
        RMM::HeaderType headerType_;
        RHeaderBody *   headerBody_;
};

typedef QValueList<RHeader> RHeaderList;
typedef QValueList<RHeader>::Iterator RHeaderListIterator;

}

#endif

// vim:ts=4:sw=4:tw=78
