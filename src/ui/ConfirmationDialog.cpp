#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/ConfirmationDialog.h"
#include "ui_ConfirmationDialog.h"

using namespace xma;

ConfirmationDialog* ConfirmationDialog::instance = NULL;

ConfirmationDialog::ConfirmationDialog(QWidget *parent) :
												QDialog(parent),
												diag(new Ui::ConfirmationDialog){

	diag->setupUi(this);
}

ConfirmationDialog::~ConfirmationDialog(){
	delete diag;
	instance = NULL;
}

ConfirmationDialog* ConfirmationDialog::getInstance()
{
	if(!instance) 
	{
		instance = new ConfirmationDialog();
	}
	return instance;
}

bool ConfirmationDialog::showConfirmationDialog(QString message)
{
	diag->message->setText(message);
	this->exec();

	if(result()){
		return true;
	}else{
		return false;
	}
}

void ConfirmationDialog::on_pushButton_OK_clicked(){
	this->accept();
}

void ConfirmationDialog::on_pushButton_Cancel_clicked(){
	this->reject();
}