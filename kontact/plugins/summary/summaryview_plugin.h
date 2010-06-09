/*
 *   Copyright 2010 Ryan Rix <ry@n.rix.si>
 * 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SUMMARYVIEW_PLUGIN_H
#define SUMMARYVIEW_PLUGIN_H

#include <kontactinterface/plugin.h>
#include <kparts/part.h>

class KDialog;

class SummaryPlugin : public KontactInterface::Plugin
{
  Q_OBJECT

  public:
    SummaryPlugin( KontactInterface::Core *core, const QVariantList & );
    ~SummaryPlugin();

    virtual void readProperties( const KConfigGroup &config );
    virtual void saveProperties( KConfigGroup &config );


    
public Q_SLOTS:
  void optionsPreferences();
  QWidget* createConfigurationInterface(QWidget* parent);
  
Q_SIGNALS:
  QWidget* sigCreateConfigurationInterface(QWidget* parent);
    
  private slots:
    void showPart();

  protected:
    KParts::ReadOnlyPart *createPart();

private:
  KDialog* m_dialog;
  KParts::ReadOnlyPart *m_part;
  
};

#endif