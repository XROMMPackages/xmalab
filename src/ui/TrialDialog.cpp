#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/TrialDialog.h"
#include "ui_TrialDialog.h"
#include "ui/State.h"
#include "ui/PlotWindow.h"

#include "core/Trial.h"
#include "core/Project.h""
#include "core/Camera.h""
#include "core/CalibrationImage.h"

using namespace xma;

TrialDialog::TrialDialog(Trial * trial, QWidget *parent) :
												QDialog(parent),
												diag(new Ui::TrialDialog), m_trial(trial){
	diag->setupUi(this);

	this->setWindowTitle("Trial : " + m_trial->getName());

	diag->comboBoxReferenceCalibration->clear();

	for (int i = 0; i < Project::getInstance()->getCameras()[0]->getCalibrationImages().size(); i++)
	{
		bool calibrated = true;
		for (int j = 0; j < Project::getInstance()->getCameras().size(); j++)
		{
			calibrated = calibrated && Project::getInstance()->getCameras()[j]->getCalibrationImages()[i]->isCalibrated();
		}
		if (calibrated) diag->comboBoxReferenceCalibration->addItem(QString::number(i + 1));
	}

	int referenceIdx = diag->comboBoxReferenceCalibration->findText(QString::number(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getReferenceCalibrationImage() + 1));

	if (referenceIdx >= 0)
	{
		diag->comboBoxReferenceCalibration->setCurrentIndex(referenceIdx);
	}
	else if(diag->comboBoxReferenceCalibration->count() > 0)
	{
		diag->comboBoxReferenceCalibration->setCurrentIndex(0);
	}
	
	int ref = diag->comboBoxReferenceCalibration->currentText().toInt() - 1;
	m_trial->setReferenceCalibrationImage(ref);

	diag->doubleSpinBoxRecSpeedFPS->setValue(m_trial->getRecordingSpeed());
	diag->doubleSpinBoxCutoffFrq->setValue(m_trial->getCutoffFrequency());
	diag->spinBoxInterpolateNFrames->setValue(m_trial->getInterpolateMissingFrames());
}



TrialDialog::~TrialDialog(){
	delete diag;
}

bool TrialDialog::isComplete()
{
	m_trial->setRecordingSpeed(diag->doubleSpinBoxRecSpeedFPS->value());
	m_trial->setCutoffFrequency(diag->doubleSpinBoxCutoffFrq->value());
	m_trial->setInterpolateMissingFrames(diag->spinBoxInterpolateNFrames->value());

	if (diag->comboBoxReferenceCalibration->count() > 0){
		int ref = diag->comboBoxReferenceCalibration->currentText().toInt() - 1;
		m_trial->setReferenceCalibrationImage(ref);
	}
	PlotWindow::getInstance()->updateTimeCheckBox();

	return true;
}

void TrialDialog::on_pushButton_OK_clicked(){
	if (isComplete()) this->accept();
}

void TrialDialog::on_pushButton_Cancel_clicked(){
	this->reject();
}


//void WorkspaceNavigationFrame::updateCalibrationReference()
//{
//	updating = true;
//	int idx;
//	if (frame->comboBoxReferenceCalibration->count() > 0){
//		idx = frame->comboBoxReferenceCalibration->currentText().toInt() - 1;
//	}
//	else
//	{
//		idx = -1;
//	}
//	frame->comboBoxReferenceCalibration->clear();
//
//	for (int i = 0; i < Project::getInstance()->getCameras()[0]->getCalibrationImages().size(); i++)
//	{
//		bool calibrated = true;
//		for (int j = 0; j < Project::getInstance()->getCameras().size(); j++)
//		{
//			calibrated = calibrated && Project::getInstance()->getCameras()[j]->getCalibrationImages()[i]->isCalibrated();
//		}
//		if (calibrated) frame->comboBoxReferenceCalibration->addItem(QString::number(i + 1));
//	}
//
//	updating = false;
//	if (idx = -1)
//	{
//		if (frame->comboBoxReferenceCalibration->count() > 0){
//			frame->comboBoxReferenceCalibration->setCurrentIndex(0);
//		}
//	}
//	else
//	{
//		frame->comboBoxReferenceCalibration->setCurrentIndex(frame->comboBoxReferenceCalibration->findData(QString::number(idx + 1)));
//	}
//}

//if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0){
//	int referenceIdx = frame->comboBoxReferenceCalibration->findText(QString::number(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getReferenceCalibrationImage() + 1));
//	if (referenceIdx != -1) {
//		frame->comboBoxReferenceCalibration->setCurrentIndex(referenceIdx);
//	}
//}
//
//int referenceIdx = frame->comboBoxReferenceCalibration->findText(QString::number(Project::getInstance()->getTrials()[activeTrial]->getReferenceCalibrationImage() + 1));
//if (referenceIdx != -1) {
//	frame->comboBoxReferenceCalibration->setCurrentIndex(referenceIdx);
//}

//void WorkspaceNavigationFrame::on_comboBoxReferenceCalibration_currentIndexChanged(QString value)
//{
//	if (!updating){
//		int idx = value.toInt() - 1;
//		if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0){
//			Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->setReferenceCalibrationImage(idx);
//		}
//	}
//}