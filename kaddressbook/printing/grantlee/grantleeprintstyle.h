/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef GRANTLEEPRINTSTYLE_H
#define GRANTLEEPRINTSTYLE_H

#include "printstyle.h"
#include <grantlee/templateloader.h>

namespace Grantlee {
class Engine;
}

namespace KABPrinting {

class PrintProgress;

class GrantleePrintStyle : public PrintStyle
{
    Q_OBJECT

public:
    explicit GrantleePrintStyle( PrintingWizard *parent );
    ~GrantleePrintStyle();

    void print( const KABC::Addressee::List &, PrintProgress * );
private:
    Grantlee::Engine *mEngine;
    Grantlee::FileSystemTemplateLoader::Ptr mTemplateLoader;
};

class GrantleeStyleFactory : public PrintStyleFactory
{
public:
    explicit GrantleeStyleFactory( PrintingWizard *parent );

    PrintStyle *create() const;
    QString description() const;
};

}

#endif // GRANTLEEPRINTSTYLE_H
