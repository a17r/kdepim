/*                                                                      
    This file is part of KAddressBook.                                  
    Copyright (c) 2002 Mike Pilone <mpilone@slac.com>                   
                                                                        
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
                                                                        
    As a special exception, permission is given to link this program    
    with any edition of Qt, and distribute the resulting executable,    
    without including the source code for Qt in the source distribution.
*/                                                                      

#ifndef JUMPBUTTONBAR_H
#define JUMPBUTTONBAR_H

#include <qsizepolicy.h>
#include <qwidget.h>

#include "viewmanager.h"

class QChar;
class QResizeEvent;

/**
  Used to draw the jump button bar on the right of the view.
 */
class JumpButtonBar : public QWidget
{
  Q_OBJECT
  
  public:
    JumpButtonBar( ViewManager *parent, const char *name = 0 );
    ~JumpButtonBar();
    
    QSizePolicy sizePolicy() const;    
    
  protected slots:
    void letterClicked();
    
  signals:
    /**
      Emitted whenever a letter is selected by the user.
     */
    void jumpToLetter( const QChar &ch );
};

#endif
