/*
	Empath - Mailer for KDE
	
	Copyright (C) 1998 Rik Hemsley rikkus@postmaster.co.uk
	
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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef RMM_DEFINES_H
#define RMM_DEFINES_H

#if QT_VERSION < 200
#define QCString QString
#endif

#ifndef NDEBUG
#include <qstring.h>
#include <qregexp.h>
#include <iostream>
#define rmmDebug(a) cerr << className() << ": " << QCString((a)) << endl;
#else
#define rmmDebug(a)
#endif

#define toCRLF(a) (a).replace(QRegExp("\n"), QCString("\r\n"))
#define toLF(a) (a).replace(QRegExp("\r\n"), QCString("\n"))

#endif
