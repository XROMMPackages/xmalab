#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/OptimizationDialog.h"
#include "ui_OptimizationDialog.h"

using namespace xma;

OptimizationDialog::OptimizationDialog(QWidget *parent) :
												QDialog(parent),
												diag(new Ui::OptimizationDialog){

	diag->setupUi(this);
}

int OptimizationDialog::getIterations()
{
	return diag->spinBox_Iterations->value();
}

int OptimizationDialog::getMethod()
{
	if (diag->radioButton_NoDist->isChecked())
	{
		return 0;
	}
	else if (diag->radioButton_NoDistXY->isChecked())
	{
		return 1;
	}
	else if (diag->radioButton_Dist->isChecked())
	{
		return 2;
	}
}

OptimizationDialog::~OptimizationDialog(){
	delete diag;
}

double OptimizationDialog::getInitial()
{
	return diag->doubleSpinBox_Initial->value();
}

void OptimizationDialog::on_pushButton_OK_clicked(){
	this->accept();
}

void OptimizationDialog::on_pushButton_Cancel_clicked(){
	this->reject();
}