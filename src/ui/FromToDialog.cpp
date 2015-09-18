#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/FromToDialog.h"
#include "ui_FromToDialog.h"

using namespace xma;

FromToDialog::FromToDialog(int from, int to, int max, bool withFormat, QWidget *parent ) :
												QDialog(parent),
												diag(new Ui::FromToDialog){

	diag->setupUi(this);

	if (!withFormat)diag->frameFormat->hide();
	diag->spinBoxFrom->setMinimum(1);
	diag->spinBoxFrom->setMaximum(max);
	diag->spinBoxFrom->setValue(from);

	diag->spinBoxTo->setMinimum(1);
	diag->spinBoxTo->setMaximum(max);
	diag->spinBoxTo->setValue(to);
}

FromToDialog::~FromToDialog(){
	delete diag;
}

int FromToDialog::getFrom()
{
	return diag->spinBoxFrom->value();
}

int FromToDialog::getTo()
{
	return diag->spinBoxTo->value();
}

QString FromToDialog::getFormat()
{
	if (diag->radioButtonJPG->isChecked())
	{
		return "jpg";
	}
	else
	{
		return "tif";
	}
}

void FromToDialog::on_pushButton_OK_clicked(){
	this->accept();
}

void FromToDialog::on_pushButton_Cancel_clicked(){
	this->reject();
}