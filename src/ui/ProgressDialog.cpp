/*
 * ProgressDialog.cpp
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */


#include "ui/ProgressDialog.h"
#include "ui_ProgressDialog.h"

ProgressDialog* ProgressDialog::instance = NULL;

ProgressDialog::ProgressDialog(QWidget *parent) :
												QDialog(parent),
												diag(new Ui::ProgressDialog){

	diag->setupUi(this);
	diag->progressBar->setValue(0.0);
}

ProgressDialog::~ProgressDialog(){
	delete diag;
	instance = NULL;
}

ProgressDialog* ProgressDialog::getInstance()
{
	if(!instance) 
	{
		instance = new ProgressDialog();
	}
	return instance;
}

void ProgressDialog::setProgress(double progress){
	diag->progressBar->setValue(progress);
	diag->progressBar->update();
	diag->progressBar->repaint();
	QApplication::processEvents();
}

void ProgressDialog::showProgressbar(int min, int max, const char* key){
	this->show();
	diag->progressBar->setMaximum(max);
	diag->progressBar->setMinimum(min);
	setWindowTitle(QApplication::translate("ProgressDialog", key, 0, QApplication::UnicodeUTF8));
	QApplication::processEvents();
}
void ProgressDialog::closeProgressbar(){
	this->close();
}