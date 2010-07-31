/*
 *  dcophandler.cpp  -  handler for DCOP calls by other applications
 *  Program:  kalarm
 *  Copyright © 2002-2006,2008 by David Jarvie <djarvie@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kalarm.h"

#include <stdlib.h>

#include <kdebug.h>

#include "alarmcalendar.h"
#include "daemon.h"
#include "functions.h"
#include "kalarmapp.h"
#include "kamail.h"
#include "karecurrence.h"
#include "mainwindow.h"
#include "preferences.h"
#include "dcophandler.moc"

static const char*  DCOP_OBJECT_NAME = "request";   // DCOP name of KAlarm's request interface
#ifdef OLD_DCOP
static const char*  DCOP_OLD_OBJECT_NAME = "display";
#endif


/*=============================================================================
= DcopHandler
= This class's function is to handle DCOP requests by other applications.
=============================================================================*/
DcopHandler::DcopHandler()
	: DCOPObject(DCOP_OBJECT_NAME),
	  TQWidget()
{
	kdDebug(5950) << "DcopHandler::DcopHandler()\n";
}


bool DcopHandler::cancelEvent(const TQString& url,const TQString& eventId)
{
	return theApp()->deleteEvent(url, eventId);
}

bool DcopHandler::triggerEvent(const TQString& url,const TQString& eventId)
{
	return theApp()->triggerEvent(url, eventId);
}

bool DcopHandler::scheduleMessage(const TQString& message, const TQString& startDateTime, int lateCancel, unsigned flags,
                                  const TQString& bgColor, const TQString& fgColor, const TQString& font,
                                  const KURL& audioFile, int reminderMins, const TQString& recurrence,
                                  int subRepeatInterval, int subRepeatCount)
{
	DateTime start;
	KARecurrence recur;
	if (!convertRecurrence(start, recur, startDateTime, recurrence, subRepeatInterval))
		return false;
	return scheduleMessage(message, start, lateCancel, flags, bgColor, fgColor, font, audioFile, reminderMins, recur, subRepeatInterval, subRepeatCount);
}

bool DcopHandler::scheduleMessage(const TQString& message, const TQString& startDateTime, int lateCancel, unsigned flags,
                                  const TQString& bgColor, const TQString& fgColor, const TQString& font,
                                  const KURL& audioFile, int reminderMins,
                                  int recurType, int recurInterval, int recurCount)
{
	DateTime start;
	KARecurrence recur;
	if (!convertRecurrence(start, recur, startDateTime, recurType, recurInterval, recurCount))
		return false;
	return scheduleMessage(message, start, lateCancel, flags, bgColor, fgColor, font, audioFile, reminderMins, recur);
}

bool DcopHandler::scheduleMessage(const TQString& message, const TQString& startDateTime, int lateCancel, unsigned flags,
                                  const TQString& bgColor, const TQString& fgColor, const TQString& font,
                                  const KURL& audioFile, int reminderMins,
                                  int recurType, int recurInterval, const TQString& endDateTime)
{
	DateTime start;
	KARecurrence recur;
	if (!convertRecurrence(start, recur, startDateTime, recurType, recurInterval, endDateTime))
		return false;
	return scheduleMessage(message, start, lateCancel, flags, bgColor, fgColor, font, audioFile, reminderMins, recur);
}

bool DcopHandler::scheduleFile(const KURL& file, const TQString& startDateTime, int lateCancel, unsigned flags, const TQString& bgColor,
                               const KURL& audioFile, int reminderMins, const TQString& recurrence,
                               int subRepeatInterval, int subRepeatCount)
{
	DateTime start;
	KARecurrence recur;
	if (!convertRecurrence(start, recur, startDateTime, recurrence, subRepeatInterval))
		return false;
	return scheduleFile(file, start, lateCancel, flags, bgColor, audioFile, reminderMins, recur, subRepeatInterval, subRepeatCount);
}

bool DcopHandler::scheduleFile(const KURL& file, const TQString& startDateTime, int lateCancel, unsigned flags, const TQString& bgColor,
                               const KURL& audioFile, int reminderMins, int recurType, int recurInterval, int recurCount)
{
	DateTime start;
	KARecurrence recur;
	if (!convertRecurrence(start, recur, startDateTime, recurType, recurInterval, recurCount))
		return false;
	return scheduleFile(file, start, lateCancel, flags, bgColor, audioFile, reminderMins, recur);
}

bool DcopHandler::scheduleFile(const KURL& file, const TQString& startDateTime, int lateCancel, unsigned flags, const TQString& bgColor,
                               const KURL& audioFile, int reminderMins, int recurType, int recurInterval, const TQString& endDateTime)
{
	DateTime start;
	KARecurrence recur;
	if (!convertRecurrence(start, recur, startDateTime, recurType, recurInterval, endDateTime))
		return false;
	return scheduleFile(file, start, lateCancel, flags, bgColor, audioFile, reminderMins, recur);
}

bool DcopHandler::scheduleCommand(const TQString& commandLine, const TQString& startDateTime, int lateCancel, unsigned flags,
                                  const TQString& recurrence, int subRepeatInterval, int subRepeatCount)
{
	DateTime start;
	KARecurrence recur;
	if (!convertRecurrence(start, recur, startDateTime, recurrence, subRepeatInterval))
		return false;
	return scheduleCommand(commandLine, start, lateCancel, flags, recur, subRepeatInterval, subRepeatCount);
}

bool DcopHandler::scheduleCommand(const TQString& commandLine, const TQString& startDateTime, int lateCancel, unsigned flags,
                                  int recurType, int recurInterval, int recurCount)
{
	DateTime start = convertStartDateTime(startDateTime);
	if (!start.isValid())
		return false;
	KARecurrence recur;
	if (!convertRecurrence(start, recur, startDateTime, recurType, recurInterval, recurCount))
		return false;
	return scheduleCommand(commandLine, start, lateCancel, flags, recur);
}

bool DcopHandler::scheduleCommand(const TQString& commandLine, const TQString& startDateTime, int lateCancel, unsigned flags,
                                  int recurType, int recurInterval, const TQString& endDateTime)
{
	DateTime start;
	KARecurrence recur;
	if (!convertRecurrence(start, recur, startDateTime, recurType, recurInterval, endDateTime))
		return false;
	return scheduleCommand(commandLine, start, lateCancel, flags, recur);
}

bool DcopHandler::scheduleEmail(const TQString& fromID, const TQString& addresses, const TQString& subject, const TQString& message,
                                const TQString& attachments, const TQString& startDateTime, int lateCancel, unsigned flags,
                                const TQString& recurrence, int subRepeatInterval, int subRepeatCount)
{
	DateTime start;
	KARecurrence recur;
	if (!convertRecurrence(start, recur, startDateTime, recurrence, subRepeatInterval))
		return false;
	return scheduleEmail(fromID, addresses, subject, message, attachments, start, lateCancel, flags, recur, subRepeatInterval, subRepeatCount);
}

bool DcopHandler::scheduleEmail(const TQString& fromID, const TQString& addresses, const TQString& subject, const TQString& message,
                                const TQString& attachments, const TQString& startDateTime, int lateCancel, unsigned flags,
                                int recurType, int recurInterval, int recurCount)
{
	DateTime start;
	KARecurrence recur;
	if (!convertRecurrence(start, recur, startDateTime, recurType, recurInterval, recurCount))
		return false;
	return scheduleEmail(fromID, addresses, subject, message, attachments, start, lateCancel, flags, recur);
}

bool DcopHandler::scheduleEmail(const TQString& fromID, const TQString& addresses, const TQString& subject, const TQString& message,
                                const TQString& attachments, const TQString& startDateTime, int lateCancel, unsigned flags,
                                int recurType, int recurInterval, const TQString& endDateTime)
{
	DateTime start;
	KARecurrence recur;
	if (!convertRecurrence(start, recur, startDateTime, recurType, recurInterval, endDateTime))
		return false;
	return scheduleEmail(fromID, addresses, subject, message, attachments, start, lateCancel, flags, recur);
}

bool DcopHandler::edit(const TQString& eventID)
{
	return KAlarm::edit(eventID);
}

bool DcopHandler::editNew(const TQString& templateName)
{
	return KAlarm::editNew(templateName);
}


/******************************************************************************
* Schedule a message alarm, after converting the parameters from strings.
*/
bool DcopHandler::scheduleMessage(const TQString& message, const DateTime& start, int lateCancel, unsigned flags,
                                  const TQString& bgColor, const TQString& fgColor, const TQString& fontStr,
                                  const KURL& audioFile, int reminderMins, const KARecurrence& recurrence,
                                  int subRepeatInterval, int subRepeatCount)
{
	unsigned kaEventFlags = convertStartFlags(start, flags);
	TQColor bg = convertBgColour(bgColor);
	if (!bg.isValid())
		return false;
	TQColor fg;
	if (fgColor.isEmpty())
		fg = Preferences::defaultFgColour();
	else
	{
		fg.setNamedColor(fgColor);
		if (!fg.isValid())
		{
			kdError(5950) << "DCOP call: invalid foreground color: " << fgColor << endl;
			return false;
		}
	}
	TQFont font;
	if (fontStr.isEmpty())
		kaEventFlags |= KAEvent::DEFAULT_FONT;
	else
	{
		if (!font.fromString(fontStr))    // N.B. this doesn't do good validation
		{
			kdError(5950) << "DCOP call: invalid font: " << fontStr << endl;
			return false;
		}
	}
	return theApp()->scheduleEvent(KAEvent::MESSAGE, message, start.dateTime(), lateCancel, kaEventFlags, bg, fg, font,
	                               audioFile.url(), -1, reminderMins, recurrence, subRepeatInterval, subRepeatCount);
}

/******************************************************************************
* Schedule a file alarm, after converting the parameters from strings.
*/
bool DcopHandler::scheduleFile(const KURL& file,
                               const DateTime& start, int lateCancel, unsigned flags, const TQString& bgColor,
                               const KURL& audioFile, int reminderMins, const KARecurrence& recurrence,
                               int subRepeatInterval, int subRepeatCount)
{
	unsigned kaEventFlags = convertStartFlags(start, flags);
	TQColor bg = convertBgColour(bgColor);
	if (!bg.isValid())
		return false;
	return theApp()->scheduleEvent(KAEvent::FILE, file.url(), start.dateTime(), lateCancel, kaEventFlags, bg, Qt::black, TQFont(),
	                               audioFile.url(), -1, reminderMins, recurrence, subRepeatInterval, subRepeatCount);
}

/******************************************************************************
* Schedule a command alarm, after converting the parameters from strings.
*/
bool DcopHandler::scheduleCommand(const TQString& commandLine,
                                  const DateTime& start, int lateCancel, unsigned flags,
                                  const KARecurrence& recurrence, int subRepeatInterval, int subRepeatCount)
{
	unsigned kaEventFlags = convertStartFlags(start, flags);
	return theApp()->scheduleEvent(KAEvent::COMMAND, commandLine, start.dateTime(), lateCancel, kaEventFlags, Qt::black, Qt::black, TQFont(),
	                               TQString::null, -1, 0, recurrence, subRepeatInterval, subRepeatCount);
}

/******************************************************************************
* Schedule an email alarm, after validating the addresses and attachments.
*/
bool DcopHandler::scheduleEmail(const TQString& fromID, const TQString& addresses, const TQString& subject,
                                const TQString& message, const TQString& attachments,
                                const DateTime& start, int lateCancel, unsigned flags,
                                const KARecurrence& recurrence, int subRepeatInterval, int subRepeatCount)
{
	unsigned kaEventFlags = convertStartFlags(start, flags);
	uint senderId = 0;
	if (!fromID.isEmpty())
	{
		senderId = KAMail::identityUoid(fromID);
		if (!senderId)
		{
			kdError(5950) << "DCOP call scheduleEmail(): unknown sender ID: " << fromID << endl;
			return false;
		}
	}
	EmailAddressList addrs;
	TQString bad = KAMail::convertAddresses(addresses, addrs);
	if (!bad.isEmpty())
	{
		kdError(5950) << "DCOP call scheduleEmail(): invalid email addresses: " << bad << endl;
		return false;
	}
	if (addrs.isEmpty())
	{
		kdError(5950) << "DCOP call scheduleEmail(): no email address\n";
		return false;
	}
	TQStringList atts;
	bad = KAMail::convertAttachments(attachments, atts);
	if (!bad.isEmpty())
	{
		kdError(5950) << "DCOP call scheduleEmail(): invalid email attachment: " << bad << endl;
		return false;
	}
	return theApp()->scheduleEvent(KAEvent::EMAIL, message, start.dateTime(), lateCancel, kaEventFlags, Qt::black, Qt::black, TQFont(),
	                               TQString::null, -1, 0, recurrence, subRepeatInterval, subRepeatCount, senderId, addrs, subject, atts);
}


/******************************************************************************
* Convert the start date/time string to a DateTime. The date/time string is in
* the format YYYY-MM-DD[THH:MM[:SS]] or [T]HH:MM[:SS]
*/
DateTime DcopHandler::convertStartDateTime(const TQString& startDateTime)
{
	DateTime start;
	if (startDateTime.length() > 10)
	{
		// Both a date and a time are specified
		start = TQDateTime::fromString(startDateTime, Qt::ISODate);
	}
	else
	{
		// Check whether a time is specified
		TQString t;
		if (startDateTime[0] == 'T')
			t = startDateTime.mid(1);     // it's a time: remove the leading 'T'
		else if (!startDateTime[2].isDigit())
			t = startDateTime;            // it's a time with no leading 'T'

		if (t.isEmpty())
		{
			// It's a date
			start = TQDate::fromString(startDateTime, Qt::ISODate);
		}
		else
		{
			// It's a time, so use today as the date
			start.set(TQDate::currentDate(), TQTime::fromString(t, Qt::ISODate));
		}
	}
	if (!start.isValid())
		kdError(5950) << "DCOP call: invalid start date/time: " << startDateTime << endl;
	return start;
}

/******************************************************************************
* Convert the flag bits to KAEvent flag bits.
*/
unsigned DcopHandler::convertStartFlags(const DateTime& start, unsigned flags)
{
	unsigned kaEventFlags = 0;
	if (flags & REPEAT_AT_LOGIN) kaEventFlags |= KAEvent::REPEAT_AT_LOGIN;
	if (flags & BEEP)            kaEventFlags |= KAEvent::BEEP;
	if (flags & SPEAK)           kaEventFlags |= KAEvent::SPEAK;
	if (flags & CONFIRM_ACK)     kaEventFlags |= KAEvent::CONFIRM_ACK;
	if (flags & REPEAT_SOUND)    kaEventFlags |= KAEvent::REPEAT_SOUND;
	if (flags & AUTO_CLOSE)      kaEventFlags |= KAEvent::AUTO_CLOSE;
	if (flags & EMAIL_BCC)       kaEventFlags |= KAEvent::EMAIL_BCC;
	if (flags & SCRIPT)          kaEventFlags |= KAEvent::SCRIPT;
	if (flags & EXEC_IN_XTERM)   kaEventFlags |= KAEvent::EXEC_IN_XTERM;
	if (flags & SHOW_IN_KORG)    kaEventFlags |= KAEvent::COPY_KORGANIZER;
	if (flags & DISABLED)        kaEventFlags |= KAEvent::DISABLED;
	if (start.isDateOnly())      kaEventFlags |= KAEvent::ANY_TIME;
	return kaEventFlags;
}

/******************************************************************************
* Convert the background colour string to a TQColor.
*/
TQColor DcopHandler::convertBgColour(const TQString& bgColor)
{
	if (bgColor.isEmpty())
		return Preferences::defaultBgColour();
	TQColor bg(bgColor);
	if (!bg.isValid())
			kdError(5950) << "DCOP call: invalid background color: " << bgColor << endl;
	return bg;
}

bool DcopHandler::convertRecurrence(DateTime& start, KARecurrence& recurrence, 
                                    const TQString& startDateTime, const TQString& icalRecurrence,
                                    int& subRepeatInterval)
{
	start = convertStartDateTime(startDateTime);
	if (!start.isValid())
		return false;
	if (!recurrence.set(icalRecurrence))
		return false;
	if (subRepeatInterval  &&  recurrence.type() == KARecurrence::NO_RECUR)
	{
		subRepeatInterval = 0;
		kdWarning(5950) << "DCOP call: no recurrence specified, so sub-repetition ignored" << endl;
	}
	return true;
}

bool DcopHandler::convertRecurrence(DateTime& start, KARecurrence& recurrence, const TQString& startDateTime,
                                    int recurType, int recurInterval, int recurCount)
{
	start = convertStartDateTime(startDateTime);
	if (!start.isValid())
		return false;
	return convertRecurrence(recurrence, start, recurType, recurInterval, recurCount, TQDateTime());
}

bool DcopHandler::convertRecurrence(DateTime& start, KARecurrence& recurrence, const TQString& startDateTime,
                                    int recurType, int recurInterval, const TQString& endDateTime)
{
	start = convertStartDateTime(startDateTime);
	if (!start.isValid())
		return false;
	TQDateTime end;
	if (endDateTime.find('T') < 0)
	{
		if (!start.isDateOnly())
		{
			kdError(5950) << "DCOP call: alarm is date-only, but recurrence end is date/time" << endl;
			return false;
		}
		end.setDate(TQDate::fromString(endDateTime, Qt::ISODate));
	}
	else
	{
		if (start.isDateOnly())
		{
			kdError(5950) << "DCOP call: alarm is timed, but recurrence end is date-only" << endl;
			return false;
		}
		end = TQDateTime::fromString(endDateTime, Qt::ISODate);
	}
	if (!end.isValid())
	{
		kdError(5950) << "DCOP call: invalid recurrence end date/time: " << endDateTime << endl;
		return false;
	}
	return convertRecurrence(recurrence, start, recurType, recurInterval, 0, end);
}

bool DcopHandler::convertRecurrence(KARecurrence& recurrence, const DateTime& start, int recurType,
                                    int recurInterval, int recurCount, const TQDateTime& end)
{
	KARecurrence::Type type;
	switch (recurType)
	{
		case MINUTELY:  type = KARecurrence::MINUTELY;  break;
		case DAILY:     type = KARecurrence::DAILY;  break;
		case WEEKLY:    type = KARecurrence::WEEKLY;  break;
		case MONTHLY:   type = KARecurrence::MONTHLY_DAY;  break;
		case YEARLY:    type = KARecurrence::ANNUAL_DATE;  break;
			break;
		default:
			kdError(5950) << "DCOP call: invalid recurrence type: " << recurType << endl;
			return false;
	}
	recurrence.set(type, recurInterval, recurCount, start, end);
	return true;
}


#ifdef OLD_DCOP
/*=============================================================================
= DcopHandlerOld
= This class's function is simply to act as a receiver for DCOP requests.
=============================================================================*/
DcopHandlerOld::DcopHandlerOld()
	: TQWidget(),
	  DCOPObject(DCOP_OLD_OBJECT_NAME)
{
	kdDebug(5950) << "DcopHandlerOld::DcopHandlerOld()\n";
}

/******************************************************************************
* Process a DCOP request.
*/
bool DcopHandlerOld::process(const TQCString& func, const TQByteArray& data, TQCString& replyType, TQByteArray&)
{
	kdDebug(5950) << "DcopHandlerOld::process(): " << func << endl;
	enum
	{
		OPERATION      = 0x0007,    // mask for main operation
		  HANDLE       = 0x0001,
		  CANCEL       = 0x0002,
		  TRIGGER      = 0x0003,
		  SCHEDULE     = 0x0004,
		ALARM_TYPE     = 0x00F0,    // mask for SCHEDULE alarm type
		  MESSAGE      = 0x0010,
		  FILE         = 0x0020,
		  COMMAND      = 0x0030,
		  EMAIL        = 0x0040,
		SCH_FLAGS      = 0x0F00,    // mask for SCHEDULE flags
		  REP_COUNT    = 0x0100,
		  REP_END      = 0x0200,
		  FONT         = 0x0400,
		PRE_096        = 0x1000,           // old-style pre-0.9.6 deprecated method
		PRE_091        = 0x2000 | PRE_096  // old-style pre-0.9.1 deprecated method
	};
	replyType = "void";
	int function;
	if (func == "handleEvent(const TQString&,const TQString&)"
	||       func == "handleEvent(TQString,TQString)")
		function = HANDLE;
	else if (func == "cancelEvent(const TQString&,const TQString&)"
	||       func == "cancelEvent(TQString,TQString)")
		function = CANCEL;
	else if (func == "triggerEvent(const TQString&,const TQString&)"
	||       func == "triggerEvent(TQString,TQString)")
		function = TRIGGER;

	//                scheduleMessage(message, dateTime, colour, colourfg, flags, audioURL, reminder, recurrence)
	else if (func == "scheduleMessage(const TQString&,const TQDateTime&,const TQColor&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,const TQString&)"
	||       func == "scheduleMessage(TQString,TQDateTime,TQColor,TQColor,Q_UINT32,TQString,Q_UINT32,TQString)")
		function = SCHEDULE | MESSAGE;
	//                scheduleMessage(message, dateTime, colour, colourfg, font, flags, audioURL, reminder, recurrence)
	else if (func == "scheduleMessage(const TQString&,const TQDateTime&,const TQColor&,const TQColor&,const TQFont&,Q_UINT32,const TQString&,Q_INT32,const TQString&)"
	||       func == "scheduleMessage(TQString,TQDateTime,TQColor,TQColor,TQFont,Q_UINT32,TQString,Q_UINT32,TQString)")
		function = SCHEDULE | MESSAGE | FONT;
	//                scheduleFile(URL, dateTime, colour, flags, audioURL, reminder, recurrence)
	else if (func == "scheduleFile(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,Q_INT32,const TQString&)"
	||       func == "scheduleFile(TQString,TQDateTime,TQColor,Q_UINT32,TQString,Q_UINT32,TQString)")
		function = SCHEDULE | FILE;
	//                scheduleCommand(commandLine, dateTime, flags, recurrence)
	else if (func == "scheduleCommand(const TQString&,const TQDateTime&,Q_UINT32,const TQString&)"
	||       func == "scheduleCommand(TQString,TQDateTime,Q_UINT32,TQString)")
		function = SCHEDULE | COMMAND;
	//                scheduleEmail(addresses, subject, message, attachments, dateTime, flags, recurrence)
	else if (func == "scheduleEmail(const TQString&,const TQString&,const TQString&,const TQString&,const TQDateTime&,Q_UINT32,const TQString&)"
	||       func == "scheduleEmail(TQString,TQString,TQString,TQString,TQDateTime,Q_UINT32,TQString)")
		function = SCHEDULE | EMAIL;

	//                scheduleMessage(message, dateTime, colour, colourfg, flags, audioURL, reminder, recurType, interval, recurCount)
	else if (func == "scheduleMessage(const TQString&,const TQDateTime&,const TQColor&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,Q_INT32,Q_INT32,Q_INT32)"
	||       func == "scheduleMessage(TQString,TQDateTime,TQColor,TQColor,Q_UINT32,TQString,Q_INT32,Q_INT32,Q_INT32,Q_INT32)")
		function = SCHEDULE | MESSAGE | REP_COUNT;
	//                scheduleFile(URL, dateTime, colour, flags, audioURL, reminder, recurType, interval, recurCount)
	else if (func == "scheduleFile(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,Q_INT32,Q_INT32,Q_INT32)"
	||       func == "scheduleFile(TQString,TQDateTime,TQColor,Q_UINT32,TQString,Q_INT32,Q_INT32,Q_INT32,Q_INT32)")
		function = SCHEDULE | FILE | REP_COUNT;
	//                scheduleCommand(commandLine, dateTime, flags, recurType, interval, recurCount)
	else if (func == "scheduleCommand(const TQString&,const TQDateTime&,Q_UINT32,Q_INT32,Q_INT32,Q_INT32)"
	||       func == "scheduleCommand(TQString,TQDateTime,Q_UINT32,Q_INT32,Q_INT32,Q_INT32)")
		function = SCHEDULE | COMMAND | REP_COUNT;
	//                scheduleEmail(addresses, subject, message, attachments, dateTime, flags, recurType, interval, recurCount)
	else if (func == "scheduleEmail(const TQString&,const TQString&,const TQString&,const TQString&,const TQDateTime&,Q_UINT32,Q_INT32,Q_INT32,Q_INT32)"
	||       func == "scheduleEmail(TQString,TQString,TQString,TQString,TQDateTime,Q_UINT32,Q_INT32,Q_INT32,Q_INT32)")
		function = SCHEDULE | EMAIL | REP_COUNT;

	//                scheduleMessage(message, dateTime, colour, colourfg, flags, audioURL, reminder, recurType, interval, endTime)
	else if (func == "scheduleMessage(const TQString&,const TQDateTime&,const TQColor&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,Q_INT32,Q_INT32,const TQDateTime&)"
	||       func == "scheduleMessage(TQString,TQDateTime,TQColor,TQColor,Q_UINT32,TQString,Q_INT32,Q_INT32,Q_INT32,TQDateTime)")
		function = SCHEDULE | MESSAGE | REP_END;
	//                scheduleFile(URL, dateTime, colour, flags, audioURL, reminder, recurType, interval, endTime)
	else if (func == "scheduleFile(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,Q_INT32,Q_INT32,const TQDateTime&)"
	||       func == "scheduleFile(TQString,TQDateTime,TQColor,Q_UINT32,TQString,Q_INT32,Q_INT32,Q_INT32,TQDateTime)")
		function = SCHEDULE | FILE | REP_END;
	//                scheduleCommand(commandLine, dateTime, flags, recurType, interval, endTime)
	else if (func == "scheduleCommand(const TQString&,const TQDateTime&,Q_UINT32,Q_INT32,Q_INT32,const TQDateTime&)"
	||       func == "scheduleCommand(TQString,TQDateTime,Q_UINT32,Q_INT32,Q_INT32,TQDateTime)")
		function = SCHEDULE | COMMAND | REP_END;
	//                scheduleEmail(addresses, subject, message, attachments, dateTime, flags, recurType, interval, endTime)
	else if (func == "scheduleEmail(const TQString&,const TQString&,const TQString&,const TQString&,const TQDateTime&,Q_UINT32,Q_INT32,Q_INT32,const TQDateTime&)"
	||       func == "scheduleEmail(TQString,TQString,TQString,TQString,TQDateTime,Q_UINT32,Q_INT32,Q_INT32,TQDateTime)")
		function = SCHEDULE | EMAIL | REP_END;

	// Deprecated methods: backwards compatibility with KAlarm pre-0.9.6
	//                scheduleMessage(message, dateTime, colour, flags, audioURL, reminder, recurrence)
	else if (func == "scheduleMessage(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,const TQString&)"
	||       func == "scheduleMessage(TQString,TQDateTime,TQColor,Q_UINT32,TQString,Q_UINT32,TQString)")
		function = SCHEDULE | MESSAGE | PRE_096;
	//                scheduleMessage(message, dateTime, colour, font, flags, audioURL, reminder, recurrence)
	else if (func == "scheduleMessage(const TQString&,const TQDateTime&,const TQColor&,const TQFont&,Q_UINT32,const TQString&,Q_INT32,const TQString&)"
	||       func == "scheduleMessage(TQString,TQDateTime,TQColor,TQFont,Q_UINT32,TQString,Q_UINT32,TQString)")
		function = SCHEDULE | MESSAGE | FONT | PRE_096;
	//                scheduleMessage(message, dateTime, colour, flags, audioURL, reminder, recurType, interval, recurCount)
	else if (func == "scheduleMessage(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,Q_INT32,Q_INT32,Q_INT32)"
	||       func == "scheduleMessage(TQString,TQDateTime,TQColor,Q_UINT32,TQString,Q_INT32,Q_INT32,Q_INT32,Q_INT32)")
		function = SCHEDULE | MESSAGE | REP_COUNT | PRE_096;
	//                scheduleMessage(message, dateTime, colour, flags, audioURL, reminder, recurType, interval, endTime)
	else if (func == "scheduleMessage(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,Q_INT32,Q_INT32,const TQDateTime&)"
	||       func == "scheduleMessage(TQString,TQDateTime,TQColor,Q_UINT32,TQString,Q_INT32,Q_INT32,Q_INT32,TQDateTime)")
		function = SCHEDULE | MESSAGE | REP_END | PRE_096;

	// Deprecated methods: backwards compatibility with KAlarm pre-0.9.1
	//                scheduleMessage(message, dateTime, colour, flags, audioURL)
	else if (func == "scheduleMessage(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,const TQString&)"
	||       func == "scheduleMessage(TQString,TQDateTime,TQColor,Q_UINT32,TQString)")
		function = SCHEDULE | MESSAGE | PRE_091;
	//                scheduleFile(URL, dateTime, colour, flags, audioURL)
	else if (func == "scheduleFile(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,const TQString&)"
	||       func == "scheduleFile(TQString,TQDateTime,TQColor,Q_UINT32,TQString)")
		function = SCHEDULE | FILE | PRE_091;
	//                scheduleMessage(message, dateTime, colour, flags, audioURL, recurType, interval, recurCount)
	else if (func == "scheduleMessage(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,Q_INT32,Q_INT32)"
	||       func == "scheduleMessage(TQString,TQDateTime,TQColor,Q_UINT32,TQString,Q_INT32,Q_INT32,Q_INT32)")
		function = SCHEDULE | MESSAGE | REP_COUNT | PRE_091;
	//                scheduleFile(URL, dateTime, colour, flags, audioURL, recurType, interval, recurCount)
	else if (func == "scheduleFile(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,Q_INT32,Q_INT32)"
	||       func == "scheduleFile(TQString,TQDateTime,TQColor,Q_UINT32,TQString,Q_INT32,Q_INT32,Q_INT32)")
		function = SCHEDULE | FILE | REP_COUNT | PRE_091;
	//                scheduleMessage(message, dateTime, colour, flags, audioURL, recurType, interval, endTime)
	else if (func == "scheduleMessage(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,Q_INT32,const TQDateTime&)"
	||       func == "scheduleMessage(TQString,TQDateTime,TQColor,Q_UINT32,TQString,Q_INT32,Q_INT32,TQDateTime)")
		function = SCHEDULE | MESSAGE | REP_END | PRE_091;
	//                scheduleFile(URL, dateTime, colour, flags, audioURL, recurType, interval, endTime)
	else if (func == "scheduleFile(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,const TQString&,Q_INT32,Q_INT32,const TQDateTime&)"
	||       func == "scheduleFile(TQString,TQDateTime,TQColor,Q_UINT32,TQString,Q_INT32,Q_INT32,TQDateTime)")
		function = SCHEDULE | FILE | REP_END | PRE_091;

	// Obsolete methods: backwards compatibility with KAlarm pre-0.7
	else if (func == "scheduleMessage(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,Q_INT32,Q_INT32)"
	||       func == "scheduleMessage(TQString,TQDateTime,TQColor,Q_UINT32,Q_INT32,Q_INT32)"
	||       func == "scheduleFile(const TQString&,const TQDateTime&,const TQColor&,Q_UINT32,Q_INT32,Q_INT32)"
	||       func == "scheduleFile(TQString,TQDateTime,TQColor,Q_UINT32,Q_INT32,Q_INT32)"
	||       func == "scheduleCommand(const TQString&,const TQDateTime&,Q_UINT32,Q_INT32,Q_INT32)"
	||       func == "scheduleCommand(TQString,TQDateTime,Q_UINT32,Q_INT32,Q_INT32)"
	// Obsolete methods: backwards compatibility with KAlarm pre-0.6
	||       func == "cancelMessage(const TQString&,const TQString&)"
	||       func == "cancelMessage(TQString,TQString)"
	||       func == "displayMessage(const TQString&,const TQString&)"
	||       func == "displayMessage(TQString,TQString)")
	{
		kdError(5950) << "DcopHandlerOld::process(): obsolete DCOP function call: '" << func << "'" << endl;
		return false;
	}
	else
	{
		kdError(5950) << "DcopHandlerOld::process(): unknown DCOP function" << endl;
		return false;
	}

	switch (function & OPERATION)
	{
		case HANDLE:        // trigger or cancel event with specified ID from calendar file
		case CANCEL:        // cancel event with specified ID from calendar file
		case TRIGGER:       // trigger event with specified ID in calendar file
		{

			TQDataStream arg(data, IO_ReadOnly);
			TQString urlString, vuid;
			arg >> urlString >> vuid;
			switch (function)
			{
				case HANDLE:
					return theApp()->handleEvent(urlString, vuid);
				case CANCEL:
					return theApp()->deleteEvent(urlString, vuid);
				case TRIGGER:
					return theApp()->triggerEvent(urlString, vuid);
			}
			break;
		}
		case SCHEDULE:      // schedule a new event
		{
			KAEvent::Action action;
			switch (function & ALARM_TYPE)
			{
				case MESSAGE:  action = KAEvent::MESSAGE;  break;
				case FILE:     action = KAEvent::FILE;     break;
				case COMMAND:  action = KAEvent::COMMAND;  break;
				case EMAIL:    action = KAEvent::EMAIL;    break;
				default:  return false;
			}
			TQDataStream  arg(data, IO_ReadOnly);
			TQString      text, audioFile, mailSubject;
			float        audioVolume = -1;
			EmailAddressList mailAddresses;
			TQStringList  mailAttachments;
			TQDateTime    dateTime, endTime;
			TQColor       bgColour;
			TQColor       fgColour(Qt::black);
			TQFont        font;
			Q_UINT32     flags;
			int          lateCancel = 0;
			KARecurrence recurrence;
			Q_INT32      reminderMinutes = 0;
			if (action == KAEvent::EMAIL)
			{
				TQString addresses, attachments;
				arg >> addresses >> mailSubject >> text >> attachments;
				TQString bad = KAMail::convertAddresses(addresses, mailAddresses);
				if (!bad.isEmpty())
				{
					kdError(5950) << "DcopHandlerOld::process(): invalid email addresses: " << bad << endl;
					return false;
				}
				if (mailAddresses.isEmpty())
				{
					kdError(5950) << "DcopHandlerOld::process(): no email address\n";
					return false;
				}
				bad = KAMail::convertAttachments(attachments, mailAttachments);
				if (!bad.isEmpty())
				{
					kdError(5950) << "DcopHandlerOld::process(): invalid email attachment: " << bad << endl;
					return false;
				}
			}
			else
				arg >> text;
			arg.readRawBytes((char*)&dateTime, sizeof(dateTime));
			if (action != KAEvent::COMMAND)
				arg.readRawBytes((char*)&bgColour, sizeof(bgColour));
			if (action == KAEvent::MESSAGE  &&  !(function & PRE_096))
				arg.readRawBytes((char*)&fgColour, sizeof(fgColour));
			if (function & FONT)
			{
				arg.readRawBytes((char*)&font, sizeof(font));
				arg >> flags;
			}
			else
			{
				arg >> flags;
				flags |= KAEvent::DEFAULT_FONT;
			}
			if (flags & KAEvent::LATE_CANCEL)
				lateCancel = 1;
			if (action == KAEvent::MESSAGE  ||  action == KAEvent::FILE)
			{
				arg >> audioFile;
				if (!(function & PRE_091))
					arg >> reminderMinutes;
			}
			if (function & (REP_COUNT | REP_END))
			{			
				KARecurrence::Type recurType;
				Q_INT32 recurCount = 0;
				Q_INT32 recurInterval;
				Q_INT32 type;
				arg >> type >> recurInterval;
				switch (type)
				{
					case 1:  recurType = KARecurrence::MINUTELY;     break;
					case 3:  recurType = KARecurrence::DAILY;        break;
					case 4:  recurType = KARecurrence::WEEKLY;       break;
					case 6:  recurType = KARecurrence::MONTHLY_DAY;  break;
					case 7:  recurType = KARecurrence::ANNUAL_DATE;  break;
					default:
						kdError(5950) << "DcopHandlerOld::process(): invalid simple repetition type: " << type << endl;
						return false;
				}
				if (function & REP_COUNT)
					arg >> recurCount;
				else
					arg.readRawBytes((char*)&endTime, sizeof(endTime));
				recurrence.set(recurType, recurInterval, recurCount,
				               DateTime(dateTime, flags & KAEvent::ANY_TIME), endTime);
			}
			else if (!(function & PRE_091))
			{
				TQString rule;
				arg >> rule;
				recurrence.set(rule);
			}
			return theApp()->scheduleEvent(action, text, dateTime, lateCancel, flags, bgColour, fgColour, font, audioFile,
			                               audioVolume, reminderMinutes, recurrence, 0, 0, 0, mailAddresses, mailSubject, mailAttachments);
		}
	}
	return false;
}
#endif // OLD_DCOP
