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
///\file WizardDockWidget.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/MainWindow.h"
#include "ui/WizardDockWidget.h"
#include "ui_WizardDockWidget.h"
#include "ui/WizardUndistortionFrame.h"
#include "ui/WizardCalibrationCubeFrame.h"
#include "ui/WizardDigitizationFrame.h"

#include "ui/Shortcuts.h"


using namespace xma;

WizardDockWidget* WizardDockWidget::instance = NULL;

WizardDockWidget::WizardDockWidget(QWidget* parent) :
	QDockWidget(parent),
	dock(new Ui::WizardDockWidget)
{
	dock->setupUi(this);

	undistortionFrame = new WizardUndistortionFrame(this);
	calibrationFrame = new WizardCalibrationCubeFrame(this);
	digitizationFrame = new WizardDigitizationFrame(this);

	dock->gridLayout->addWidget(undistortionFrame, 0, 0, 1, 1);
	dock->gridLayout->addWidget(calibrationFrame, 1, 0, 1, 1);
	dock->gridLayout->addWidget(digitizationFrame, 2, 0, 1, 1);

	dock->gridLayout->setSizeConstraint(QLayout::SetFixedSize);
	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));

	Shortcuts::getInstance()->installEventFilterToChildren(this);
}

bool WizardDockWidget::checkForPendingChanges()
{
	if (State::getInstance()->getWorkspace() == UNDISTORTION)
	{
		return undistortionFrame->checkForPendingChanges();
	}
	else if (State::getInstance()->getWorkspace() == CALIBRATION)
	{
		return calibrationFrame->checkForPendingChanges();
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		return true;
	}
	return true;
}

void WizardDockWidget::updateWizard()
{
	calibrationFrame->runCalibrationCameraAllFrames();
}

WizardDockWidget::~WizardDockWidget()
{
	delete dock;
	instance = NULL;
}

WizardDockWidget* WizardDockWidget::getInstance()
{
	if (!instance)
	{
		instance = new WizardDockWidget(MainWindow::getInstance());
		MainWindow::getInstance()->addDockWidget(Qt::LeftDockWidgetArea, instance);
	}
	return instance;
}

bool WizardDockWidget::manualCalibrationRunning()
{
	return calibrationFrame->manualCalibrationRunning();
}

void WizardDockWidget::addCalibrationReference(double x, double y)
{
	calibrationFrame->addCalibrationReference(x, y);
}

void WizardDockWidget::addDigitizationPoint(int camera, double x, double y)
{
	digitizationFrame->addDigitizationPoint(camera, x, y);
}

void WizardDockWidget::selectDigitizationPoint(int camera, double x, double y)
{
	digitizationFrame->selectDigitizationPoint(camera, x, y);
}

void WizardDockWidget::moveDigitizationPoint(int camera, double x, double y, bool noDetection)
{
	digitizationFrame->moveDigitizationPoint(camera, x, y, noDetection);
}

void WizardDockWidget::draw()
{
	if (State::getInstance()->getWorkspace() == UNDISTORTION)
	{
	}
	else if (State::getInstance()->getWorkspace() == CALIBRATION)
	{
		calibrationFrame->draw();
	}
}

void WizardDockWidget::updateDialog()
{
	if (State::getInstance()->getWorkspace() == CALIBRATION)
	{
		calibrationFrame->loadCalibrationSettings();
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->setDialog();
	}
}

void WizardDockWidget::stop()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->stopTracking();
	}
}

void WizardDockWidget::undoLastPoint()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->undoLastPoint();
	}
}

void WizardDockWidget::workspaceChanged(work_state workspace)
{
	if (workspace == UNDISTORTION)
	{
		undistortionFrame->show();
		calibrationFrame->hide();
		digitizationFrame->hide();
	}
	else if (workspace == CALIBRATION)
	{
		undistortionFrame->hide();
		calibrationFrame->show();
		digitizationFrame->hide();
	}
	else if (workspace == DIGITIZATION)
	{
		undistortionFrame->hide();
		calibrationFrame->hide();
		digitizationFrame->show();
	}
}

void WizardDockWidget::trackSelectedPointForward()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->trackSelectedPointToNextFrame();
	}
}

void WizardDockWidget::trackSelectedPointBackward()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->trackSelectedPointToPrevFrame();
	}
}

void WizardDockWidget::goToLastTrackedFrame()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->goToLastTrackedFrame();
	}
}

void WizardDockWidget::goToFirstTrackedFrame()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->goToFirstTrackedFrame();
	}
}

void WizardDockWidget::interpolateActive()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->on_pushButton_InterpolateActive_clicked(true);
	}
}

void WizardDockWidget::interpolateAll()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->on_pushButton_InterpolateAll_clicked(true);
	}
}