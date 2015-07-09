#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/MarkerDialog.h"
#include "ui_MarkerDialog.h"
#include "ui/State.h"

#include "core/Marker.h"

using namespace xma;

MarkerDialog::MarkerDialog(Marker * marker, QWidget *parent) :
												QDialog(parent),
												diag(new Ui::MarkerDialog), m_marker(marker){
	diag->setupUi(this);

	diag->spinBox_MarkerRadius->setValue(m_marker->getSizeOverride());
	diag->spinBox_ThresholdOffset->setValue(m_marker->getThresholdOffset());
	diag->spinBox_Penalty->setValue(m_marker->getMaxPenalty());
	diag->comboBox_Method->setCurrentIndex(m_marker->getMethod());
}



MarkerDialog::~MarkerDialog(){
	delete diag;
}

bool MarkerDialog::isComplete()
{
	m_marker->setMaxPenalty(diag->spinBox_Penalty->value());
	m_marker->setMethod(diag->comboBox_Method->currentIndex());
	m_marker->setSizeOverride(diag->spinBox_MarkerRadius->value());
	m_marker->setThresholdOffset(diag->spinBox_ThresholdOffset->value());

	return true;
}

void MarkerDialog::on_pushButton_OK_clicked(){
	if (isComplete()) this->accept();
}

void MarkerDialog::on_pushButton_Cancel_clicked(){
	this->reject();
}
