/***************************************************************************
                          knsmtpclient.cpp  -  description
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

#include <unistd.h>

#include "knsavedarticle.h"
#include "knsmtpclient.h"

KNSmtpClient::KNSmtpClient(int NfdPipeIn, int NfdPipeOut, QObject *parent, const char *name)
: KNProtocolClient(NfdPipeIn,NfdPipeOut,parent,name)
{}


KNSmtpClient::~KNSmtpClient()
{}


// examines the job and calls the suitable handling method
void KNSmtpClient::processJob()
{
	switch (job->type()) {
		case KNJobData::JTmail :
			doMail();
			break;
		default:
			qDebug("KNSmtpClient::processJob(): mismatched job");		
	}
}
	

void KNSmtpClient::doMail()
{
	KNSavedArticle *art=(KNSavedArticle*)job->data();
	
	sendSignal(TSsendMail);	
	
	QCString cmd = "MAIL FROM:";
	//cmd += art->headerLine("From");
	cmd += art->fromEmail();
	if (!sendCommandWCheck(cmd,250))
		return;
		
	progressValue = 80;

	cmd = "RCPT TO:";
	cmd += art->headerLine("To");
	if (!sendCommandWCheck(cmd,250))
		return;
		
	progressValue = 90;

	if (!sendCommandWCheck("DATA",354))
		return;
		
	progressValue = 100;
	
	if (!sendMsg(art->encodedData()))
		return;
		
	if (!checkNextResponse(250))
		return;
}


bool KNSmtpClient::openConnection()
{
	if (!KNProtocolClient::openConnection())
		return false;
		
	progressValue = 30;
		
	if (!checkNextResponse(220))
		return false;
		
	progressValue = 50;

	char hostName[500];

	QCString cmd = "HELO ";
		
	if (gethostname(hostName,490)==0) {
	  cmd += hostName;
  	qDebug("KNSmtpClient::openConnection(): %s",cmd.data());	
	} else {
		cmd += "foo";
  	qDebug("KNSmtpClient::openConnection(): can't detect hostname, using foo");
	}
	
	if (!sendCommandWCheck(cmd,250))
		return false;
		
	progressValue = 70;
	
	return true;
}



//--------------------------------

#include "knsmtpclient.moc"
