/*
  Copyright (c) 2012-2016 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/
#ifndef COMPOSERIMAGERESIZEWIDGET_H
#define COMPOSERIMAGERESIZEWIDGET_H

#include <QWidget>
#include <QWebElement>

class QPaintEvent;
class QMouseEvent;

namespace ComposerEditorWebEngine
{
class ComposerImageResizeWidgetPrivate;
class ComposerImageResizeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ComposerImageResizeWidget(const QWebElement &element, QWidget *parent = Q_NULLPTR);
    ~ComposerImageResizeWidget();

protected:
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    friend class ComposerImageResizeWidgetPrivate;
    ComposerImageResizeWidgetPrivate *const d;
};
}

#endif // COMPOSERIMAGERESIZEWIDGET_H
