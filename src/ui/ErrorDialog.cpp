#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/ErrorDialog.h"
#include "ui_ErrorDialog.h"

using namespace xma;

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