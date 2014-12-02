/*
 * AboutDialog.cpp
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */


#include "ui/AboutDialog.h"
#include "ui_AboutDialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
												QDialog(parent),
												diag(new Ui::AboutDialog){

	diag->setupUi(this);
	diag->version_label->setText(PROJECT_VERSION);
	diag->date_label->setText(PROJECT_BUILD_TIME);
}

AboutDialog::~AboutDialog(){
	delete diag;
}
