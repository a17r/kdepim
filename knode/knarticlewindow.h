/***************************************************************************
                          knarticlewindow.h  -  description
                             -------------------

    copyright            : (C) 2000 by Christian Thurner
    email                : cthurner@freepage.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef KNARTICLEWINDOW_H
#define KNARTICLEWINDOW_H

#include <kmainwindow.h>

class KNArticle;
class KNArticleWidget;
class KNArticleCollection;

class KNArticleWindow : public KMainWindow  {

  Q_OBJECT
  
  public:
    KNArticleWindow(KNArticle *art=0, KNArticleCollection *col=0, const char *name=0);
    ~KNArticleWindow();
    KNArticleWidget* artWidget()        { return artW; }
      
  protected:
    KNArticleWidget *artW;
    
  protected slots:
    void slotFileClose();
    void slotArtReply();
    void slotArtRemail();
    void slotArtForward();
    void slotArtCancel();
    void slotArtSupersede();
    void slotToggleToolBar();
    void slotConfKeys();
    void slotConfToolbar();
};

#endif
