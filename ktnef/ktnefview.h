/*
    ktnefview.h

    Copyright (C) 2002 Michael Goffioul <kdeprint@swing.be>

    This file is part of KTNEF, the KDE TNEF support library/program.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef KTNEFWIDGET_H
#define	KTNEFWIDGET_H

#include <k3listview.h>
#include <q3ptrlist.h>
#include <QResizeEvent>

#include <ktnef/ktnefattach.h>

class KTNEFView : public K3ListView
{
	Q_OBJECT

public:
	KTNEFView( QWidget *parent = 0 );
	~KTNEFView();

        void setAttachments(const QList<KTnef::KTNEFAttach*> &list);
        QList<KTnef::KTNEFAttach*>* getSelection();

signals:
        void dragRequested( const QList<KTnef::KTNEFAttach*>& list );

protected:
	void resizeEvent(QResizeEvent *e);
	void startDrag();

private:
        QList<KTnef::KTNEFAttach*>	attachments_;
};

#endif
