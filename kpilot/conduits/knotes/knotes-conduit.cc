/* knotes-conduit.cc			KPilot
**
** Copyright (C) 2000-2001 by Adriaan de Groot
**
** This file is part of the KNotes conduit, a conduit for KPilot that
** synchronises the Pilot's memo pad application with KNotes.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, 
** MA 02139, USA.
*/

/*
** Bug reports and questions can be sent to adridg@cs.kun.nl
*/


#include "options.h"

// Only include what we really need:
// First UNIX system stuff, then std C++, 
// then Qt, then KDE, then local includes.
//
//
#include <iostream.h>
#ifndef QDIR_H
#include <qdir.h>
#endif

#ifndef QMAP_H
#include <qmap.h>
#endif

#ifndef _KGLOBAL_H
#include <kglobal.h>
#endif

#ifndef _KSTDDIRS_H
#include <kstddirs.h>
#endif

#ifndef _KMESSAGEBOX_H
#include <kmessagebox.h>
#endif

#ifndef _KSIMPLECONFIG_H
#include <ksimpleconfig.h>
#endif

#ifndef _KCONFIG_H
#include <kconfig.h>
#endif

#ifndef _DCOPCLIENT_H
#include <dcopclient.h>
#endif

#ifndef _KDEBUG_H
#include <kdebug.h>
#endif


#ifndef _KPILOT_CONDUITAPP_H
#include "conduitApp.h"
#endif

#ifndef _KPILOT_KPILOTCONFIG_H
#include "kpilotConfig.h"
#endif

#ifndef _KPILOT_KNOTES_CONDUIT_H
#include "knotes-conduit.h"
#endif

#ifndef _KPILOT_SETUPDIALOG_H
#include "setupDialog.h"
#endif

#ifndef _KPILOT_PILOTMEMO_H
#include "pilotMemo.h"
#endif



#ifndef _KPILOT_MD5WRAP_H
#include	"md5wrap.h"
#endif


// Something to allow us to check what revision
// the modules are that make up a binary distribution.
//
//
static const char *knotes_conduit_id=
	"$Id$";


// This is a generic main() function, all
// conduits look basically the same,
// except for the name of the conduit.
//
//
int main(int argc, char* argv[])
{
	ConduitApp a(argc,argv,"knotes",
		I18N_NOOP("KNotes Conduit"),
		KPILOT_VERSION);

	a.addAuthor("Adriaan de Groot",
		"KNotes Conduit author",
		"adridg@sci.kun.nl");

	KNotesConduit conduit(a.getMode());
	a.setConduit(&conduit);
	return a.exec(true /* with DCOP support */);
}


NotesSettings::NotesSettings(const QString &configPath,
	const QString &notesdir,
	KConfig& c) :
	cP(configPath)
{
	FUNCTIONSETUP;

	DEBUGCONDUIT << fname
		<< ": Reading note from "
		<< configPath
		<< " in dir "
		<< notesdir
		<< endl;

	c.setGroup("KPilot");
	id = c.readNumEntry("pilotID",0);
	checksum = c.readEntry("checksum",QString::null);
	csValid = !(checksum.isNull());

	c.setGroup("Data");
	nN = c.readEntry("name");

	QString notedata(notesdir);
	notedata.append("/.");
	notedata.append(nN);
	notedata.append("_data");
	if (QFile::exists(notedata))
	{
		DEBUGCONDUIT << fname 
			<< ": Data for note in " 
			<< notedata 
			<< endl;
		dP = notedata;
	}
	else
	{
		kdWarning() << __FUNCTION__
			<< ": No data file for note?"
			<< " (tried "
			<< notedata << ")"
			<< endl;
	}
}

QString NotesSettings::computeCheckSum() const
{
	FUNCTIONSETUP;

	if (dP.isEmpty()) return 0;

	QFile f(dP);
	if (!f.open(IO_ReadOnly))
	{
		kdWarning() << __FUNCTION__
			<< ": Couldn't open file."
			<< endl;
		return QString::null;
	}

	unsigned char data[PilotMemo::MAX_MEMO_LEN];
	int r = f.readBlock((char *)data,(int) PilotMemo::MAX_MEMO_LEN);
	if (r<1)
	{
		kdWarning() << __FUNCTION__
			<< ": Couldn't read notes file."
			<< endl;
		return QString::null;
	}

	QString s;
	MD5Context md5context;
	md5context.update(data,r);
	s=md5context.finalize();

	return s;
}

int NotesSettings::readNotesData(char *text)
{
	FUNCTIONSETUP;

	// Check that the data can actually fit into a memo.
	// If it does, read it all.
	//
	QFile f(dataPath());
	int filesize = f.size();

	if (filesize > PilotMemo::MAX_MEMO_LEN) 
	{
		kdWarning() << __FUNCTION__
			<< ": Notes file is too large ("
			<< filesize
			<< " bytes) -- truncated to "
			<< (int) PilotMemo::MAX_MEMO_LEN
			<< endl;
		filesize = PilotMemo::MAX_MEMO_LEN;
	}
		
	if (!f.open(IO_ReadOnly)) 
	{
		kdWarning() << __FUNCTION__
			<< ": Couldn't read notes file."
			<< endl;
		return 0;
	}

	memset(text,0,PilotMemo::MAX_MEMO_LEN+1);
	int len = f.readBlock(text,filesize);

	DEBUGCONDUIT << fname
		<< ": Read "
		<< len
		<< " bytes from note "
		<< dataPath()
		<< endl;

	return len;
}

static NotesMap
collectNotes()
{
	FUNCTIONSETUP;
	NotesMap m;

	// This is code taken directly from KNotes
	//
	//
	QString str_notedir = KGlobal::dirs()->
		saveLocation( "appdata", "notes/" );

#ifdef DEBUG
	{
		kdDebug() << fname << ": Notes dir = " << str_notedir << endl;
	}
#endif

	QDir notedir( str_notedir );
	QStringList notes = notedir.entryList( QDir::Files, QDir::Name );
	QStringList::ConstIterator  i;

	for (i=notes.begin() ; i !=notes.end(); ++i)
	{
		QString notename,notedata ;
		KSimpleConfig *c = 0L;
		int version ;

#ifdef DEBUG
		{
			kdDebug() << fname << ": Reading note " << *i << endl;
		}
#endif
		c = new KSimpleConfig( notedir.absFilePath(*i));
		
		c->setGroup("General");
		version = c->readNumEntry("version",1);
		if (version<2)
		{
			kdWarning() << __FUNCTION__
				<< ": Skipping old-style KNote"
				<< *i
				<< endl;
			goto EndNote;
		}

		{
		NotesSettings n(notedir.absFilePath(*i),
			notedir.absPath(),
			*c);
		m.insert(*i,n);
		}

EndNote:
		delete c;
	}

	return m;
}


static NotesMap::Iterator *
findID(NotesMap& m,unsigned long id)
{
	FUNCTIONSETUP;

	NotesMap::Iterator *i = new NotesMap::Iterator;

	for ((*i)=m.begin(); (*i)!=m.end(); ++(*i))
	{
		const NotesSettings& r = (*(*i));

		if (r.pilotID()==id)
		{
#ifdef DEBUG
			{
				kdDebug() << fname
					<< ": Found ID "
					<< id
					<< " in note "
					<< r.configPath()
					<< endl;
			}
#endif
			return i;
		}
	}

	delete i;

#ifdef DEBUG
	{
		kdDebug() << fname
			<< ": ID "
			<< id
			<< " not found."
			<< endl;
	}
#endif
		
	return 0L;
}



KNotesConduit::KNotesConduit(eConduitMode mode) : 
	BaseConduit(mode),
	fDeleteNoteForMemo(false)
{
	FUNCTIONSETUP;

}

KNotesConduit::~KNotesConduit()
{
	FUNCTIONSETUP;

}

void
KNotesConduit::readConfig()
{
	FUNCTIONSETUP;

	KConfig& c = KPilotConfig::getConfig(KNotesOptions::KNotesGroup);
	getDebugLevel(c);
	fDeleteNoteForMemo = c.readBoolEntry("DeleteNoteForMemo",false);
#ifdef DEBUG
	{
		kdDebug() << fname
			<< ": Settings "
			<< "DeleteNoteForMemo="
			<< fDeleteNoteForMemo
			<< endl;
	}
#endif
}

void
KNotesConduit::doSync()
{
	FUNCTIONSETUP;

	readConfig();

	NotesMap m = collectNotes();

	// First add all the new KNotes to the Pilot
	//
	//
	int newCount = notesToPilot(m);
	int oldCount = pilotToNotes(m);

	if (newCount || oldCount)
	{
		QString msg = i18n("Changed %1/%2 Memos/Notes")
			.arg(newCount)
			.arg(oldCount);
		addSyncLogMessage(msg.local8Bit());

		DEBUGCONDUIT << fname
			<< ": "
			<< msg
			<< endl;
	}

	DCOPClient *dcopptr = KApplication::kApplication()->dcopClient();
	if (!dcopptr)
	{
		kdWarning() << __FUNCTION__
			<< ": Can't get DCOP client."
			<< endl;
		return;
	}

	QByteArray data;
	if (dcopptr -> send("knotes",
		"KNotesIface",
		"rereadNotesDir()",
		data))
	{
		kdWarning() << __FUNCTION__
			<< ": Couldn't tell KNotes to re-read notes."
			<< endl;
	}
}


bool KNotesConduit::addNewNote(NotesSettings& s)
{
	FUNCTIONSETUP;

	// We know that this is a new memo (no pilot id)
	char text[PilotMemo::MAX_MEMO_LEN+1];
	if (!s.readNotesData(text))
	{
		return false;
	}

	PilotMemo *memo = new PilotMemo(text,0,0,0);
	PilotRecord *rec = memo->pack();
	unsigned long id = writeRecord(rec);

	KSimpleConfig *c = new KSimpleConfig(s.configPath());
	c->setGroup("KPilot");
	c->writeEntry("pilotID",id);
	c->writeEntry("checksum",s.computeCheckSum());

	s.setId(id);

	delete c;
	delete rec;
	delete memo;

	return true;
}

bool KNotesConduit::changeNote(NotesSettings& s)
{
	FUNCTIONSETUP;

	char text[PilotMemo::MAX_MEMO_LEN+1];
	if (!s.readNotesData(text))
	{
		return false;
	}

	return false;
}


int KNotesConduit::notesToPilot(NotesMap& m)
{
	FUNCTIONSETUP;
	NotesMap::Iterator i;
	int count=0;

	DEBUGCONDUIT << fname
		<< ": Adding new memos to pilot"
		<< endl;


	for (i=m.begin(); i!=m.end(); ++i)
	{
		NotesSettings& s=(*i);

		if (s.isNew()) 
		{
			addNewNote(s);
			count++;
		}
		if (s.isChanged()) 
		{
			changeNote(s);
			count++;
		}
	}

	return count;
}

bool KNotesConduit::newMemo(NotesMap& m,unsigned long id,PilotMemo *memo)
{
	FUNCTIONSETUP;

	QString noteName = memo->sensibleTitle();

	// This is code taken directly from KNotes
	//
	//
	QString str_notedir = KGlobal::dirs()->
		saveLocation( "appdata", "notes/" );

#ifdef DEBUG
	{
		kdDebug() << fname << ": Notes dir = " << str_notedir << endl;
	}
#endif

	QDir notedir( str_notedir );

	if (notedir.exists(noteName))
	{
		noteName += QString::number(id);
	}

	if (notedir.exists(noteName))
	{
		kdDebug() << fname
			<< ": Note " << noteName
			<< " already exists!"
			<< endl;
		return false;
	}

	QString dataName = "." + noteName + "_data" ;
	bool success = false;
#ifdef DEBUG
	{
		kdDebug() << fname
			<< ": Creating note "
			<< noteName
			<< endl;
	}
#endif

	{
	QFile file(notedir.absFilePath(dataName));
	file.open(IO_WriteOnly | IO_Truncate);
	if (file.isOpen())
	{
		file.writeBlock(memo->text(),strlen(memo->text()));
		success = true;
	}
	file.close();
	}

	// Write out the KNotes config file 
	if (success)
	{
	KConfig *c = new KSimpleConfig( notedir.absFilePath(noteName));
	
	c->setGroup("General");
	c->writeEntry("version",2);
	c->setGroup("KPilot");
	c->writeEntry("pilotID",id);
	c->setGroup("Data");
	c->writeEntry("name",noteName);
	c->sync();

	delete c;
	}

	if (success)
	{
		NotesSettings n(noteName,
			notedir.absFilePath(noteName),
			notedir.absFilePath(dataName),
			id);
		m.insert(noteName,n);
	}

	return success;
}

bool KNotesConduit::changeMemo(NotesMap& m,NotesMap::Iterator i,PilotMemo *memo)
{
	FUNCTIONSETUP;


// handle deleted memos (deleted on the pilot) here.
// since they're just copies of what's in knotes,
// delete the files.

	(void) m;
	NotesSettings n = *i;
#ifdef DEBUG
	{
		kdDebug() << fname
			<< ": Updating note "
			<< n.configPath()
			<< endl;
	}
#endif
	QFile file(n.dataPath());
	file.open(IO_WriteOnly | IO_Truncate);
	if (file.isOpen())
	{
		file.writeBlock(memo->text(),strlen(memo->text()));
#ifdef DEBUG
		{
			kdDebug() << fname
				<< ": Succesfully updated memo "
				<< n.configPath()
				<< endl;
		}
#endif

		return true;
	}
	else
	{
		return false;
	}
}

bool KNotesConduit::deleteNote(NotesMap& m,NotesMap::Iterator *i,
	unsigned long id)
{
	FUNCTIONSETUP;

	if (!i)
	{
#ifdef DEBUG
		{
			kdDebug() << fname
				<< ": Unknown Pilot memo "
				<< id
				<< " has been deleted."
				<< endl;
		}
#endif
		return false;
	}

#ifdef DEBUG
	{
		if (fDeleteNoteForMemo)
		{
			kdDebug() << fname
				<< ": Deleting pilot memo "
				<< id
				<< endl;
		}
		else
		{
			kdDebug() << fname
				<< ": Pilot memo "
				<< id
				<< " has been deleted, note remains."
				<< endl;
			return false;
		}
	}
#endif

	NotesSettings n = *(*i);

#ifdef DEBUG
	{
		kdDebug() << fname
			<< ": Deleting note "
			<< n.configPath()
			<< endl;
	}
#endif

	QFile::remove(n.dataPath());
	QFile::remove(n.configPath());

	m.remove(*i);

	return true;
}


int KNotesConduit::pilotToNotes(NotesMap& m)
{
	FUNCTIONSETUP;

	PilotRecord *rec;
	int count=0;
	NotesMap::Iterator *i;

	while ((rec=readNextModifiedRecord()))
	{
		unsigned long id = rec->getID();
		bool success;

		success=false;

		kdDebug() << fname 
			<< ": Read Pilot record with ID "
			<< id
			<< endl;

		i = findID(m,id);
		if (rec->getAttrib() & dlpRecAttrDeleted)
		{
			success=deleteNote(m,i,id);
		}
		else
		{
			PilotMemo *memo = new PilotMemo(rec);
			if (i)
			{
				success=changeMemo(m,*i,memo);
			}
			else
			{
				success=newMemo(m,id,memo);
			}
			delete memo;
		}
		delete rec;

		if (success) 
		{ 
			count++;
		}
	}

// we also need to tell knotes that things have changed in the dirs it owns
	DCOPClient *dcopptr = KApplication::kApplication()->dcopClient();
	if (!dcopptr)
	{
		kdWarning() << __FUNCTION__
			<< ": Can't get DCOP client."
			<< endl;
		return count;
	}

	QByteArray data;
	if (dcopptr -> send("knotes",
		"KNotesIface",
		"rereadNotesDir()",
		data))
	{
		DEBUGCONDUIT << fname
			<< ": DCOP to KNotes succesful."
			<< endl;
	}
	else
	{
		kdWarning() << __FUNCTION__
			<< ": Couln't tell KNotes about new notes."
			<< endl;
	}

	return count;
}

// aboutAndSetup is pretty much the same
// on all conduits as well.
//
//
QWidget*
KNotesConduit::aboutAndSetup()
{
	FUNCTIONSETUP;

	return new KNotesOptions(0L);
}

const char *
KNotesConduit::dbInfo()
{
	return "MemoDB";
}



/* virtual */ void
KNotesConduit::doTest()
{
	FUNCTIONSETUP;
	DCOPClient *dcopptr = KApplication::kApplication()->dcopClient();
	if (!dcopptr)
	{
		kdWarning() << __FUNCTION__
			<< ": Can't get DCOP client."
			<< endl;
		return;
	}

	NotesMap m = collectNotes();


	NotesMap::Iterator i;
	for (i=m.begin(); i!=m.end(); ++i)
	{
		if ((*i).pilotID())
		{
			DEBUGCONDUIT << fname
				<< ": Showing note " 
				<< (*i).name()
				<< endl;

			QByteArray data;
			QDataStream arg(data,IO_WriteOnly);
			arg << (*i).name() ;
			if (dcopptr -> send("knotes",
				"KNotesIface",
				"showNote(QString)",
				data))
			{
				DEBUGCONDUIT << fname
					<< ": DCOP send succesful"
					<< endl;
			}
			else
			{
				kdWarning() << __FUNCTION__
					<< ": DCOP send failed."
					<< endl;
			}
		}
		else
		{
			DEBUGCONDUIT << fname
				<< ": Note "
				<< (*i).name()
				<< " not in Pilot"
				<< endl;
		}
		DEBUGCONDUIT << fname
			<< ": Checksum = "
			<< (*i).computeCheckSum()
			<< endl;
	}

	(void) knotes_conduit_id;
}

// $Log$
// Revision 1.17  2001/05/25 16:06:52  adridg
// DEBUG breakage
//
// Revision 1.16  2001/03/27 11:10:38  leitner
// ported to Tru64 unix: changed all stream.h to iostream.h, needed some
// #ifdef DEBUG because qstringExpand etc. were not defined.
//
// Revision 1.15  2001/03/09 09:46:14  adridg
// Large-scale #include cleanup
//
// Revision 1.14  2001/03/05 23:57:53  adridg
// Added KPILOT_VERSION
//
// Revision 1.13  2001/02/26 22:13:26  adridg
// Removed misleading comments
//
// Revision 1.12  2001/02/24 14:08:13  adridg
// Massive code cleanup, split KPilotLink
//
// Revision 1.11  2001/02/09 15:59:28  habenich
// replaced "char *id" with "char *<filename>_id", because of --enable-final in configure
//
// Revision 1.10  2001/02/07 15:46:31  adridg
// Updated copyright headers for source release. Added CVS log. No code change.
//
// Revision 1.9  2001/01/06 13:23:12  adridg
// Updated version numbers, fixed debugging stuff
//
// Revision 1.8  2001/01/02 15:02:59  bero
// Fix build
//
// Revision 1.7  2000/12/30 20:28:11  adridg
// Checksumming added
//
// Revision 1.6  2000/12/29 14:17:51  adridg
// Added checksumming to KNotes conduit
//
// Revision 1.5  2000/12/22 07:47:04  adridg
// Added DCOP support to conduitApp. Breaks binary compatibility.
//
// Revision 1.4  2000/12/05 07:44:01  adridg
// Cleanup
//
// Revision 1.1  2000/11/20 00:22:28  adridg
// New KNotes conduit
//
