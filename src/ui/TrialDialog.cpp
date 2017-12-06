//  ----------------------------------
//  XMALab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMALab software is provided under the terms of the GNU General Public License version 3 
//  as published by the Free Software Foundation at http://www.gnu.org/licenses/gpl-3.0.html, provided 
//  that this copyright notice appear in all copies and that the name of Brown University not be used in 
//  advertising or publicity pertaining to the use or distribution of the software without specific written 
//  prior permission from Brown University.
//  
//  See license.txt for further information.
//  
//  BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS 
//  PROVIDED “AS IS”, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file TrialDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/TrialDialog.h"
#include "ui_TrialDialog.h"
#include "ui/State.h"
#include "ui/PlotWindow.h"
#include "ui/ConfirmationDialog.h"
#include "ui/NewTrialDialog.h"

#include "core/Trial.h"
#include "core/Project.h"
#include "core/Camera.h"
#include "core/CalibrationImage.h"

#include "processing/ThreadScheduler.h"

using namespace xma;

TrialDialog::TrialDialog(Trial* trial, QWidget* parent) :
	QDialog(parent),
	diag(new Ui::TrialDialog), m_trial(trial), returnValue(TRIALDAILOGDEFAULT)
{
	diag->setupUi(this);
	returnValue = TRIALDAILOGDEFAULT;

	this->setWindowTitle("Trial : " + m_trial->getName());

	diag->comboBoxReferenceCalibration->clear();

	for (unsigned int i = 0; i < Project::getInstance()->getCameras()[0]->getCalibrationImages().size(); i++)
	{
		bool calibrated = true;
		for (unsigned int j = 0; j < Project::getInstance()->getCameras().size(); j++)
		{
			calibrated = calibrated && (Project::getInstance()->getCameras()[j]->getCalibrationImages()[i]->isCalibrated() > 0) ;
		}
		if (calibrated) diag->comboBoxReferenceCalibration->addItem(QString::number(i + 1));
	}

	int referenceIdx = diag->comboBoxReferenceCalibration->findText(QString::number(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getReferenceCalibrationImage() + 1));

	if (referenceIdx >= 0)
	{
		diag->comboBoxReferenceCalibration->setCurrentIndex(referenceIdx);
	}
	else if (diag->comboBoxReferenceCalibration->count() > 0)
	{
		diag->comboBoxReferenceCalibration->setCurrentIndex(0);
	}

	int ref = diag->comboBoxReferenceCalibration->currentText().toInt() - 1;

	if (ref != m_trial->getReferenceCalibrationImage())
	{
		m_trial->setReferenceCalibrationImage(ref);
		m_trial->setRequiresRecomputation(true);
		ThreadScheduler::getInstance()->updateTrialData(m_trial);
	}

	diag->doubleSpinBoxRecSpeedFPS->setValue(m_trial->getRecordingSpeed());
	diag->doubleSpinBoxCutoffFrq->setValue(m_trial->getCutoffFrequency());
	diag->checkBoxInterpolate->setChecked(m_trial->getInterpolate3D());

	if (!Project::getInstance()->hasCalibration())
	{
		diag->comboBoxReferenceCalibration->setEnabled(false);
		diag->checkBoxInterpolate->setEnabled(false);
		diag->doubleSpinBoxCutoffFrq->setEnabled(false);
	}
}


TrialDialog::~TrialDialog()
{
	delete diag;
}

trialDialogReturn TrialDialog::getDialogReturn()
{
	return returnValue;
}


bool TrialDialog::isComplete()
{
	returnValue = TRIALDAILOGDEFAULT;
	if (diag->doubleSpinBoxRecSpeedFPS->value() != m_trial->getRecordingSpeed()){
		returnValue = TRIALDIALOGUPDATEFILTER;
		m_trial->setRecordingSpeed(diag->doubleSpinBoxRecSpeedFPS->value());
		PlotWindow::getInstance()->updateTimeCheckBox();
	}
	if (diag->doubleSpinBoxCutoffFrq->value() != m_trial->getCutoffFrequency()){
		returnValue = TRIALDIALOGUPDATEFILTER;
		m_trial->setCutoffFrequency(diag->doubleSpinBoxCutoffFrq->value());
	}

	m_trial->setInterpolate3D(diag->checkBoxInterpolate->isChecked());

	if (diag->comboBoxReferenceCalibration->count() > 0)
	{
		int ref = diag->comboBoxReferenceCalibration->currentText().toInt() - 1;
		if (ref != m_trial->getReferenceCalibrationImage())
		{
			m_trial->setReferenceCalibrationImage(ref);
			returnValue = TRIALDIALOGUPDATE;
		}
	}
	
	return true;
}

void TrialDialog::on_pushButton_OK_clicked()
{
	if (isComplete()) this->accept();
}

void TrialDialog::on_pushButton_Cancel_clicked()
{
	this->reject();
}

void TrialDialog::on_pushButton_DeleteTrial_clicked()
{
	if (ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you really want to delete the trial?"))
	{
		returnValue = TRIALDIALOGDELETE;
		this->reject();
	}
}

void TrialDialog::on_pushButton_ChangeTrialData_clicked()
{
	NewTrialDialog* newTriaLdialog = new NewTrialDialog(m_trial);
	newTriaLdialog->exec();

	if (newTriaLdialog->result())
	{
		newTriaLdialog->createTrial();
	}
	delete newTriaLdialog;
	returnValue = TRIALDIALOGCHANGE;
	this->reject();
}

void TrialDialog::on_pushButton_Update_clicked()
{
	returnValue = TRIALDIALOGUPDATE;
	this->reject();
}


