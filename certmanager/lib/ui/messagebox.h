/*
    messagebox.h

    This file is part of libkleopatra, the KDE keymanagement library
    Copyright (c) 2007 Klarälvdalens Datakonsult AB

    Libkleopatra is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    Libkleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef __KLEO_UI_MESSAGEBOX_H__
#define __KLEO_UI_MESSAGEBOX_H__

#include <kmessagebox.h>

namespace GpgME {
    class SigningResult;
    class EncryptionResult;
    class DecryptionResult;
    class VerificationResult;
}

namespace Kleo {
    class Job;
}

class QWidget;
class QString;

namespace Kleo {

    class MessageBox {
    public:
        static void information( TQWidget * parent, const GpgME::SigningResult & result, const Kleo::Job * job, const TQString & caption, int options=KMessageBox::Notify );
        static void information( TQWidget * parent, const GpgME::SigningResult & result, const Kleo::Job * job, int options=KMessageBox::Notify );
        static void error( TQWidget * parent, const GpgME::SigningResult & result, const Kleo::Job * job, const TQString & caption, int options=KMessageBox::Notify );
        static void error( TQWidget * parent, const GpgME::SigningResult & result, const Kleo::Job * job, int options=KMessageBox::Notify );

        static void information( TQWidget * parent, const GpgME::EncryptionResult & result, const Kleo::Job * job, const TQString & caption, int options=KMessageBox::Notify );
        static void information( TQWidget * parent, const GpgME::EncryptionResult & result, const Kleo::Job * job, int options=KMessageBox::Notify );
        static void error( TQWidget * parent, const GpgME::EncryptionResult & result, const Kleo::Job * job, const TQString & caption, int options=KMessageBox::Notify );
        static void error( TQWidget * parent, const GpgME::EncryptionResult & result, const Kleo::Job * job, int options=KMessageBox::Notify );

        static void information( TQWidget * parent, const GpgME::SigningResult & sresult, const GpgME::EncryptionResult & eresult, const Kleo::Job * job, const TQString & caption, int options=KMessageBox::Notify );
        static void information( TQWidget * parent, const GpgME::SigningResult & sresult, const GpgME::EncryptionResult & eresult, const Kleo::Job * job, int options=KMessageBox::Notify );
        static void error( TQWidget * parent, const GpgME::SigningResult & sresult, const GpgME::EncryptionResult & eresult, const Kleo::Job * job, const TQString & caption, int options=KMessageBox::Notify );
        static void error( TQWidget * parent, const GpgME::SigningResult & sresult, const GpgME::EncryptionResult & eresult, const Kleo::Job * job, int options=KMessageBox::Notify );

        static void auditLog( TQWidget * parent, const Kleo::Job * job, const TQString & caption );
        static void auditLog( TQWidget * parent, const Kleo::Job * job );
        static void auditLog( TQWidget * parent, const TQString & log, const TQString & caption );
        static void auditLog( TQWidget * parent, const TQString & log );

    private:
        static void make( TQWidget * parent, TQMessageBox::Icon icon, const TQString & text, const Kleo::Job * job, const TQString & caption, int options );
    };


}

#endif /* __KLEO_UI_MESSAGEBOX_H__ */
