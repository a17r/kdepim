/* pilotRecord.cc			KPilot
**
** Copyright (C) 1998-2001 by Dan Pilone
**
** This is a wrapper for pilot-link's general
** Pilot database structures. These records are
*** just collections of bits. See PilotAppCategory
** for interpreting the bits in a meaningful way.
**
** As a crufty hack, the non-inline parts of
** PilotAppCategory live in this file as well.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
** MA 02111-1307, USA.
*/

/*
** Bug reports and questions can be sent to kde-pim@kde.org
*/
#include "options.h"

#include <string.h>

#include <qtextcodec.h>
#include <qregexp.h>

// PilotAppCategory includes pilotRecord and we
// provide its implementation here as well.
//
#include "pilotAppCategory.h"



static const char *pilotRecord_id =
	"$Id$";

/* static */ int PilotRecord::fAllocated = 0;
/* static */ int PilotRecord::fDeleted = 0;

/* static */ void PilotRecord::allocationInfo()
{
#ifdef DEBUG
	FUNCTIONSETUP;
	DEBUGKPILOT << fname
		<< ": Allocated " << fAllocated
		<< "  Deleted " << fDeleted;
#endif
}

PilotRecord::PilotRecord(void *data, int len, int attrib, int cat,
	pi_uid_t uid) :
	fData(0L),
	fLen(len),
	fAttrib(attrib),
	fCat(cat),
	fID(uid)
{
	FUNCTIONSETUP;
	fData = new char[len];

	memcpy(fData, data, len);

	fAllocated++;
	(void) pilotRecord_id;
}

PilotRecord::PilotRecord(PilotRecord * orig)
{
	FUNCTIONSETUP;
	fData = new char[orig->getLen()];

	memcpy(fData, orig->getData(), orig->getLen());
	fLen = orig->getLen();
	fAttrib = orig->getAttrib();
	fCat = orig->getCat();
	fID = orig->getID();

	fAllocated++;
}

PilotRecord & PilotRecord::operator = (PilotRecord & orig)
{
	FUNCTIONSETUP;
	if (fData)
		delete[]fData;
	fData = new char[orig.getLen()];

	memcpy(fData, orig.getData(), orig.getLen());
	fLen = orig.getLen();
	fAttrib = orig.getAttrib();
	fCat = orig.getCat();
	fID = orig.getID();
	return *this;
}

void PilotRecord::setData(const char *data, int len)
{
	FUNCTIONSETUP;
	if (fData)
		delete[]fData;
	fData = new char[len];

	memcpy(fData, data, len);
	fLen = len;
}


/* static */ QTextCodec *PilotAppCategory::pilotCodec = 0L;

static const char *latin1 = "ISO8859-1" ;
// Other names of encodings are in the config dialog source

/* static */ QTextCodec *PilotAppCategory::createCodec(const char *p)
{
	FUNCTIONSETUP;

	if (!p) p=latin1;
#ifdef DEBUG
	DEBUGKPILOT << ": Creating codec for " << p << endl;
#endif
	QTextCodec *q = QTextCodec::codecForName(p);
	if (!q) q = QTextCodec::codecForName(latin1);
	pilotCodec = q;
	return q;
}

/* static */ QTextCodec *PilotAppCategory::setupPilotCodec(const QString &s)
{
	FUNCTIONSETUP;
	QString cdc(s);

#ifdef DEBUG
	DEBUGKPILOT << fname
		<< ": Creating codec " << cdc << endl;
#endif
	// the codec can also be of the form "Description (codec)", so
	// if it matches the regexp ".*\\((.*)\\).*", use just the
	// value between the brackets.
	cdc.replace(QRegExp(".*\\((.*)\\).*"), "\\1");

	const char *p = 0L;
	// This latin1() is OK. The names of the encodings
	// as shown in the table in the QTextCodec docs
	// are all US-ASCII.
	if (!cdc.isEmpty()) p=cdc.latin1();

	(void) PilotAppCategory::createCodec(p);

#ifdef DEBUG
	DEBUGKPILOT << fname
		<< ": Got codec " << codecName().latin1() << " for setting "
		<< s.latin1() << endl;
#endif
	return codec();
}

/* static */ QString PilotAppCategory::codecName()
{
	return codec()->name();
}
