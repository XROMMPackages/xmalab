#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/AboutDialog.h"
#include "ui_AboutDialog.h"

using namespace xma;

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
