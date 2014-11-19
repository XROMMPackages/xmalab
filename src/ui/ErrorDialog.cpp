/*
 * ProgressDialog.cpp
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */


#include "ui/ErrorDialog.h"
#include "ui_ErrorDialog.h"

ErrorDialog* ErrorDialog::instance = NULL;

ErrorDialog::ErrorDialog(QWidget *parent) :
												QDialog(parent),
												diag(new Ui::ErrorDialog){

	diag->setupUi(this);
}

ErrorDialog::~ErrorDialog(){
	delete diag;
	instance = NULL;
}

ErrorDialog* ErrorDialog::getInstance()
{
	if(!instance) 
	{
		instance = new ErrorDialog();
	}
	return instance;
}

void ErrorDialog::showErrorDialog(QString message)
{
	diag->message->setText(message);
	this->exec();
}