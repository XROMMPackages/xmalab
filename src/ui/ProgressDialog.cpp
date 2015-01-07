#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/ProgressDialog.h"
#include "ui_ProgressDockWidget.h"
#include "ui/MainWindow.h"

using namespace xma;

ProgressDialog* ProgressDialog::instance = NULL;

ProgressDialog::ProgressDialog(QWidget *parent) :
												QDockWidget(parent),
												diag(new Ui::ProgressDockWidget){

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
		instance = new ProgressDialog(MainWindow::getInstance());
		MainWindow::getInstance()->addDockWidget(Qt::BottomDockWidgetArea, instance);
	}
	return instance;
}

void ProgressDialog::setProgress(double progress){
	diag->progressBar->setTextVisible(true);
	diag->progressBar->setValue(progress);
	diag->progressBar->update();
	diag->progressBar->repaint();
	QApplication::processEvents();
}

void ProgressDialog::showProgressbar(int min, int max, const char* key){
	MainWindow::getInstance()->setEnabled(false);
	this->show();
	diag->progressBar->setMaximum(max);
	diag->progressBar->setMinimum(min);
	diag->progressBar->setTextVisible(false);
	setWindowTitle(QApplication::translate("ProgressDialog", key, 0, QApplication::UnicodeUTF8));
	QApplication::processEvents();
}
void ProgressDialog::closeProgressbar(){
	MainWindow::getInstance()->setEnabled(true);
	this->close();
}