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

#ifndef SUBJECTLINEEDITWITHAUTOCORRECTION_H
#define SUBJECTLINEEDITWITHAUTOCORRECTION_H

#include "pimcommon_export.h"

#include "libkdepim/widgets/spellchecklineedit.h"

namespace PimCommon {
class AutoCorrection;
class PIMCOMMON_EXPORT SubjectLineEditWithAutoCorrection : public KPIM::SpellCheckLineEdit
{
    Q_OBJECT
public:
    explicit SubjectLineEditWithAutoCorrection(QWidget* parent, const QString& configFile);
    ~SubjectLineEditWithAutoCorrection();

    PimCommon::AutoCorrection* autocorrection() const;

    void setAutocorrection(PimCommon::AutoCorrection *autocorrect);
    void setAutocorrectionLanguage(const QString &language);

protected:
    void keyPressEvent ( QKeyEvent *e );

private:
    PimCommon::AutoCorrection *mAutoCorrection;
};
}

#endif // SUBJECTLINEEDITWITHAUTOCORRECTION_H
