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
///\file WizardDigitizationFrame.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/WizardDigitizationFrame.h"
#include "ui/MainWindow.h"
#include "ui_WizardDigitizationFrame.h"
#include "ui/PointsDockWidget.h"
#include "ui/PlotWindow.h"
#include "ui/DetectionSettings.h"

#include "core/Project.h"
#include "core/Trial.h"
#include "core/Settings.h"
#include "core/Marker.h"
#include <processing/MarkerDetection.h>
#include <processing/MarkerTracking.h>


using namespace xma;

WizardDigitizationFrame::WizardDigitizationFrame(QWidget* parent) :
	QFrame(parent),
	frame(new Ui::WizardDigitizationFrame)
{
	frame->setupUi(this);

	frame->checkBoxMarkerIds->setChecked(Settings::getInstance()->getBoolSetting("TrialDrawMarkerIds"));
	frame->checkBoxRigidBodyConstellation->setChecked(Settings::getInstance()->getBoolSetting("TrialDrawRigidBodyConstellation"));
	frame->checkBoxRigidBodyMeshmodels->setChecked(Settings::getInstance()->getBoolSetting("TrialDrawRigidBodyMeshmodels"));
	frame->checkBox_DrawFiltered->setChecked(Settings::getInstance()->getBoolSetting("TrialDrawFiltered"));

#ifdef __APPLE__
	frame->pushButton->setMinimumHeight(26);
                                                    
    frame->pushButton_RBBack->setMinimumHeight(26);
    frame->pushButton_RBPrev->setMinimumHeight(26);
    frame->pushButton_RBForw->setMinimumHeight(26);
    frame->pushButton_RBNext->setMinimumHeight(26);
    
    frame->pushButton_AllPrev->setMinimumHeight(26);
    frame->pushButton_AllNext->setMinimumHeight(26);
    frame->pushButton_AllBack->setMinimumHeight(26);
    frame->pushButton_AllForw->setMinimumHeight(26);
    
    frame->pushButton_PointNext->setMinimumHeight(26);
    frame->pushButton_PointPrev->setMinimumHeight(26);
    frame->pushButton_PointBack->setMinimumHeight(26);
    frame->pushButton_PointForw->setMinimumHeight(26);

	frame->pushButton_InterpolateActive->setMinimumHeight(26);
	frame->pushButton_InterpolateAll->setMinimumHeight(26);

	frame->checkBoxMarkerIds->setMinimumHeight(26);
	frame->checkBoxRigidBodyConstellation->setMinimumHeight(26);
	frame->checkBoxRigidBodyMeshmodels->setMinimumHeight(26);
	frame->checkBox_DrawFiltered->setMinimumHeight(26);

	frame->pushButton_RBBack->setFocusPolicy(Qt::StrongFocus);
	frame->pushButton_RBPrev->setFocusPolicy(Qt::StrongFocus);
	frame->pushButton_RBForw->setFocusPolicy(Qt::StrongFocus);
	frame->pushButton_RBNext->setFocusPolicy(Qt::StrongFocus);

	frame->pushButton_AllPrev->setFocusPolicy(Qt::StrongFocus);
	frame->pushButton_AllNext->setFocusPolicy(Qt::StrongFocus);
	frame->pushButton_AllBack->setFocusPolicy(Qt::StrongFocus);
	frame->pushButton_AllForw->setFocusPolicy(Qt::StrongFocus);

	frame->pushButton_PointNext->setFocusPolicy(Qt::StrongFocus);
	frame->pushButton_PointPrev->setFocusPolicy(Qt::StrongFocus);
	frame->pushButton_PointBack->setFocusPolicy(Qt::StrongFocus);
	frame->pushButton_PointForw->setFocusPolicy(Qt::StrongFocus);

	frame->checkBoxMarkerIds->setFocusPolicy(Qt::StrongFocus);
	frame->checkBoxRigidBodyConstellation->setFocusPolicy(Qt::StrongFocus);
	frame->checkBoxRigidBodyMeshmodels->setFocusPolicy(Qt::StrongFocus);
	frame->checkBox_DrawFiltered->setFocusPolicy(Qt::StrongFocus);

#endif
	connect(PointsDockWidget::getInstance(), SIGNAL(activePointChanged(int)), this, SLOT(activePointChanged(int)));
	connect(State::getInstance(), SIGNAL(activeTrialChanged(int)), this, SLOT(activeTrialChanged(int)));
	connect(State::getInstance(), SIGNAL(activeCameraChanged(int)), this, SLOT(activeCameraChanged(int)));
	connect(State::getInstance(), SIGNAL(activeFrameTrialChanged(int)), this, SLOT(activeFrameTrialChanged(int)));
	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));

	trackID = 0;
	trackType = 0;
	trackDirection = 0;
	singleTrack = false;
}

void WizardDigitizationFrame::addDigitizationPoint(int camera, double x, double y)
{
	Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->setActiveToNextUndefinedMarker(camera);
	PlotWindow::getInstance()->updateMarkers(true);

	Marker* marker = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker();

	if (marker != NULL)
	{
		canUndo = true;
		MainWindow::getInstance()->setUndo(canUndo);
		lastPoint = marker->getPoints2D()[camera][State::getInstance()->getActiveFrameTrial()];
		lastStatus = marker->getStatus2D()[camera][State::getInstance()->getActiveFrameTrial()];


		marker->setPoint(camera, State::getInstance()->getActiveFrameTrial(), x, y, SET);

		PointsDockWidget::getInstance()->reloadListFromObject();
		MainWindow::getInstance()->redrawGL();

		MarkerDetection* markerdetection = new MarkerDetection(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveTrial(), State::getInstance()->getActiveFrameTrial(), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		connect(markerdetection, SIGNAL(detectMarker_finished()), MainWindow::getInstance(), SLOT(redrawGL()));
		markerdetection->detectMarker();
	}
}

void WizardDigitizationFrame::selectDigitizationPoint(int camera, double x, double y)
{
	double dist = 20;
	int idx = -1;
	for (unsigned int i = 0; i < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); i++)
	{
		double x1 = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->getPoints2D()[camera][State::getInstance()->getActiveFrameTrial()].x;
		double y1 = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->getPoints2D()[camera][State::getInstance()->getActiveFrameTrial()].y;
		double tmpdist = sqrt((x - x1) * (x - x1) + (y - y1) * (y - y1));

		if (tmpdist < dist)
		{
			dist = tmpdist;
			idx = i;
		}
	}

	if (idx >= 0)
	{
		PointsDockWidget::getInstance()->selectPoint(idx + 1);
		MainWindow::getInstance()->redrawGL();
	}
}

void WizardDigitizationFrame::moveDigitizationPoint(int camera, double x, double y, bool noDetection)
{
	Marker* marker = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker();

	if (marker != NULL)
	{
		canUndo = true;
		MainWindow::getInstance()->setUndo(canUndo);
		lastPoint = marker->getPoints2D()[camera][State::getInstance()->getActiveFrameTrial()];
		lastStatus = marker->getStatus2D()[camera][State::getInstance()->getActiveFrameTrial()];

		marker->setPoint(camera, State::getInstance()->getActiveFrameTrial(), x, y, noDetection ? MANUAL : SET);

		if (DetectionSettings::getInstance()->isVisible() && (marker->getMethod() == 0 || marker->getMethod() == 2 || marker->getMethod() == 5))
		{
			DetectionSettings::getInstance()->setMarker(marker);
			DetectionSettings::getInstance()->update(camera, cv::Point2d(x, y));
		}

		if (!noDetection)
		{
			MarkerDetection* markerdetection = new MarkerDetection(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveTrial(), State::getInstance()->getActiveFrameTrial(), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
			connect(markerdetection, SIGNAL(detectMarker_finished()), MainWindow::getInstance(), SLOT(redrawGL()));
			markerdetection->detectMarker();
		}
		else
		{
			MainWindow::getInstance()->redrawGL();
			setDialog();
		}
	}
}

void WizardDigitizationFrame::undoLastPoint()
{
	if (canUndo)
	{
		Marker* marker = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker();
		marker->setPoint(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveFrameTrial(), lastPoint.x, lastPoint.y, markerStatus(lastStatus));
		canUndo = false;
		MainWindow::getInstance()->setUndo(canUndo);
		MainWindow::getInstance()->redrawGL();
		setDialog();
	}
}

WizardDigitizationFrame::~WizardDigitizationFrame()
{
	delete frame;
}

void WizardDigitizationFrame::activePointChanged(int idx){
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		canUndo = false;
		MainWindow::getInstance()->setUndo(canUndo);
	}
}


void WizardDigitizationFrame::activeTrialChanged(int activeTrial)
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		canUndo = false;
		MainWindow::getInstance()->setUndo(canUndo);
	}
}

void WizardDigitizationFrame::activeCameraChanged(int activeCamera)
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		canUndo = false;
		MainWindow::getInstance()->setUndo(canUndo);
		setDialog();
	}
}

void WizardDigitizationFrame::activeFrameTrialChanged(int activeFrame)
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		canUndo = false;
		MainWindow::getInstance()->setUndo(canUndo);
		setDialog();
	}
}

void WizardDigitizationFrame::workspaceChanged(work_state workspace)
{
	if (workspace == DIGITIZATION)
	{
		canUndo = false;
		MainWindow::getInstance()->setUndo(canUndo);
		setDialog();
	}
}

void WizardDigitizationFrame::trackSinglePoint()
{
	int startFrame = State::getInstance()->getActiveFrameTrial();
	int endFrame;
	tmptrackID = trackID;
	if (trackID >= 0 && trackID <= (int) Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size())
	{
		if (trackDirection > 0)
		{
			endFrame = startFrame + 1;
		}
		else
		{
			endFrame = startFrame - 1;
		}
		std::vector<MarkerTracking *> trackers;
		for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
		{
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->getStatus2D()[i][startFrame] > UNDEFINED &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->getStatus2D()[i][endFrame] <= (Settings::getInstance()->getBoolSetting("RetrackOptimizedTrackedPoints") ? TRACKED_AND_OPTIMIZED : TRACKED))
			{
				MarkerTracking* markertracking = new MarkerTracking(i, State::getInstance()->getActiveTrial(), startFrame, endFrame, trackID, trackDirection > 0);
				connect(markertracking, SIGNAL(trackMarker_finished()), this, SLOT(trackSinglePointFinished()));
				trackers.push_back(markertracking);
			}
		}

		State::getInstance()->setDisableDraw(true);
		State::getInstance()->changeActiveFrameTrial(endFrame);

		if (trackers.size() > 0)
		{
			for (unsigned int i = 0; i < trackers.size(); i++)
			{
				trackers[i]->trackMarker();
			}
			trackers.clear();
		}
		else
		{
			checkIfValid();
		}
	}
	else
	{
		uncheckTrackButtons();
	}
}

void WizardDigitizationFrame::trackSinglePointFinished()
{
	std::vector<MarkerDetection *> detectors;
	for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
	{
		if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[tmptrackID]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED)
		{
			MarkerDetection* markerdetection = new MarkerDetection(i, State::getInstance()->getActiveTrial(), State::getInstance()->getActiveFrameTrial(), tmptrackID,
			                                                       Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[tmptrackID]->getSize() * 2, true);
			detectors.push_back(markerdetection);
			connect(markerdetection, SIGNAL(detectMarker_finished()), this, SLOT(checkIfValid()));
		}
	}

	if (detectors.size() > 0)
	{
		for (unsigned int i = 0; i < detectors.size(); i++)
		{
			detectors[i]->detectMarker();
		}
		detectors.clear();
	}
	else
	{
		checkIfValid();
	}
	tmptrackID = -1;
}


void WizardDigitizationFrame::trackRB()
{
	int startFrame = State::getInstance()->getActiveFrameTrial();
	int endFrame;

	if (trackDirection > 0)
	{
		endFrame = startFrame + 1;
	}
	else
	{
		endFrame = startFrame - 1;
	}
	std::vector<int> markers = PointsDockWidget::getInstance()->getSelectedPoints();
	std::vector<MarkerTracking *> trackers;
	for (std::vector<int>::const_iterator it = markers.begin(); it < markers.end();++it)
	{
		for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
		{
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->getStatus2D()[i][startFrame] > UNDEFINED &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->getStatus2D()[i][endFrame] <= (Settings::getInstance()->getBoolSetting("RetrackOptimizedTrackedPoints") ? TRACKED_AND_OPTIMIZED : TRACKED))
			{
				MarkerTracking* markertracking = new MarkerTracking(i, State::getInstance()->getActiveTrial(), startFrame, endFrame, *it, trackDirection > 0);
				connect(markertracking, SIGNAL(trackMarker_finished()), this, SLOT(trackAllFinished()));
				trackers.push_back(markertracking);
			}
		}
	}
	State::getInstance()->setDisableDraw(true);
	State::getInstance()->changeActiveFrameTrial(endFrame);

	if (trackers.size() > 0)
	{
		for (unsigned int i = 0; i < trackers.size(); i++)
		{
			trackers[i]->trackMarker();
		}
		trackers.clear();
	}
	else
	{
		checkIfValid();
	}
}

void WizardDigitizationFrame::trackRBFinished()
{
	std::vector<MarkerDetection *> detectors;
	std::vector<int> markers = PointsDockWidget::getInstance()->getSelectedPoints();
	for (std::vector<int>::const_iterator it = markers.begin(); it < markers.end(); ++it)
	{
		for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
		{
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED)
			{
				MarkerDetection* markerdetection = new MarkerDetection(i, State::getInstance()->getActiveTrial(), State::getInstance()->getActiveFrameTrial(), *it,
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->getSize() * 2, true);
				detectors.push_back(markerdetection);
				connect(markerdetection, SIGNAL(detectMarker_finished()), this, SLOT(checkIfValid()));
			}
		}
	}

	if (detectors.size() > 0)
	{
		for (unsigned int i = 0; i < detectors.size(); i++)
		{
			detectors[i]->detectMarker();
		}
		detectors.clear();
	}
	else
	{
		checkIfValid();
	}
}

void WizardDigitizationFrame::trackAll()
{
	int startFrame = State::getInstance()->getActiveFrameTrial();
	int endFrame;

	if (trackDirection > 0)
	{
		endFrame = startFrame + 1;
	}
	else
	{
		endFrame = startFrame - 1;
	}
	std::vector<MarkerTracking *> trackers;
	for (unsigned int j = 0; j < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); j++)
	{
		for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
		{
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][startFrame] > UNDEFINED &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][endFrame] <= (Settings::getInstance()->getBoolSetting("RetrackOptimizedTrackedPoints") ? TRACKED_AND_OPTIMIZED : TRACKED))
			{
				MarkerTracking* markertracking = new MarkerTracking(i, State::getInstance()->getActiveTrial(), startFrame, endFrame, j, trackDirection > 0);
				connect(markertracking, SIGNAL(trackMarker_finished()), this, SLOT(trackAllFinished()));
				trackers.push_back(markertracking);
			}
		}
	}
	State::getInstance()->setDisableDraw(true);
	State::getInstance()->changeActiveFrameTrial(endFrame);

	if (trackers.size() > 0)
	{
		for (unsigned int i = 0; i < trackers.size(); i++)
		{
			trackers[i]->trackMarker();
		}
		trackers.clear();
	}
	else
	{
		checkIfValid();
	}
}

void WizardDigitizationFrame::trackAllFinished()
{
	std::vector<MarkerDetection *> detectors;
	for (unsigned int j = 0; j < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); j++)
	{
		for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
		{
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED)
			{
				MarkerDetection* markerdetection = new MarkerDetection(i, State::getInstance()->getActiveTrial(), State::getInstance()->getActiveFrameTrial(), j,
				                                                       Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getSize() * 2, true);
				detectors.push_back(markerdetection);
				connect(markerdetection, SIGNAL(detectMarker_finished()), this, SLOT(checkIfValid()));
			}
		}
	}

	if (detectors.size() > 0)
	{
		for (unsigned int i = 0; i < detectors.size(); i++)
		{
			detectors[i]->detectMarker();
		}
		detectors.clear();
	}
	else
	{
		checkIfValid();
	}
}


void WizardDigitizationFrame::checkIfValid()
{
	bool valid = true;
	if (trackType == 1)
	{
		for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
		{
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->getMethod() != 4)
			{
				valid = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->isValid(i, State::getInstance()->getActiveFrameTrial()) && valid;
			}
		}
	}
	else if (trackType == 2)
	{
		std::vector<int> markers = PointsDockWidget::getInstance()->getSelectedPoints();
		for (std::vector<int>::const_iterator it = markers.begin(); it < markers.end(); ++it)
		{
			for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED &&
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->getMethod() != 4)
				{
					valid = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->isValid(i, State::getInstance()->getActiveFrameTrial()) && valid;
				}
			}
		}
	}
	else if (trackType == 3)
	{
		for (unsigned int j = 0; j < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); j++)
		{
			for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED &&
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getMethod() != 4)
				{
					valid = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->isValid(i, State::getInstance()->getActiveFrameTrial()) && valid;
				}
			}
		}
	}

	if (!valid && trackType == 1)
	{
		uncheckTrackButtons();
	}

	State::getInstance()->setDisableDraw(false);
	State::getInstance()->changeActiveFrameTrial(State::getInstance()->getActiveFrameTrial(), true);

	track();
}

void WizardDigitizationFrame::track()
{
	setDialog();
	QApplication::processEvents();

	if (singleTrack)
	{
		uncheckTrackButtons();
		return;
	}
	else if (abs(trackDirection) <= 1)
	{
		singleTrack = true;
	}

	if (trackDirection > 0 && State::getInstance()->getActiveFrameTrial() == Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1)
	{
		uncheckTrackButtons();
		return;
	}
	else if (trackDirection < 0 && State::getInstance()->getActiveFrameTrial() == Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1)
	{
		uncheckTrackButtons();
		return;
	}

	switch (trackType)
	{
	default:
	case 0:
		uncheckTrackButtons();
		break;
	case 1:
		trackSinglePoint();
		break;
	case 2:
		trackRB();
		break;
	case 3:
		trackAll();
		break;
	}
}


void WizardDigitizationFrame::on_pushButton_clicked()
{
	if (Project::getInstance()->isCalibrated())
	{
		if (Project::getInstance()->getTrials().size() == 0)
		{
			MainWindow::getInstance()->on_pushButtonNewTrial_clicked();
		}
		else
		{
		}
	}
}

void WizardDigitizationFrame::setDialog()
{
	if (Project::getInstance()->isCalibrated())
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() > 0)
			{
				frame->label->hide();
				frame->pushButton->hide();
				frame->groupBox_All->show();
				frame->groupBox_Point->show();

				if (trackDirection == 0)
				{
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx() >= 0)
					{
						bool visible = false;
						for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
						{
							if (State::getInstance()->getActiveFrameTrial() >= 0 && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker() != NULL && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker()->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] > UNDEFINED)
							{
								visible = true;
							}
						}

						frame->groupBox_Point->setEnabled(visible);
					}

					//if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size() > 0){
					//	frame->groupBox_RB->show();
					//	if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx() >= 0)
					//	{
					//		bool visible = false;

					//		if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRB()->getPoseComputed()[State::getInstance()->getActiveFrameTrial()])
					//		{
					//			visible = true;
					//		}

					//		frame->groupBox_RB->setEnabled(visible);
					//	}
					//}
					//else
					//{
					//frame->groupBox_RB->hide();
					//}
				}
			}
			else
			{
				frame->label->show();
#ifdef WIN32
				frame->label->setText("You first have to add points by using CTRL + left Click in the image or by using the point widget");
#else
				frame->label->setText("You first have to add points by using COMMAND + left Click in the image or by using the point widget");
#endif
				frame->pushButton->hide();
				frame->groupBox_All->hide();
				frame->groupBox_Point->hide();
				frame->groupBox_RB->hide();
			}
		}
		else
		{
			frame->label->show();
			frame->label->setText("You first have to add a trial");
			frame->pushButton->show();
			frame->pushButton->setText("Add Trial");
			frame->groupBox_All->hide();
			frame->groupBox_Point->hide();
			frame->groupBox_RB->hide();
		}
	}
	else
	{
		frame->label->show();
		frame->label->setText("You first have to calibrate your cameras");
		frame->pushButton->hide();
		frame->groupBox_All->hide();
		frame->groupBox_Point->hide();
		frame->groupBox_RB->hide();
	}
}

void WizardDigitizationFrame::stopTracking()
{
	uncheckTrackButtons();
}

void WizardDigitizationFrame::trackSelectedPointToNextFrame()
{
	if (frame->pushButton_PointNext->isEnabled())
	{
		on_pushButton_PointNext_clicked();
	}
}

void WizardDigitizationFrame::goToLastTrackedFrame()
{
	if ((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
	{
		int trackidx = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
		if (trackidx >= 0)
		{
			for (int i = State::getInstance()->getActiveFrameTrial(); i < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame(); i++)
			{
				for (unsigned int j = 0; j < Project::getInstance()->getCameras().size(); j++)
				{
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackidx]->getStatus2D()[j][i] <= 0)
					{
						if (i - 1 > State::getInstance()->getActiveFrameTrial())
						{
							State::getInstance()->changeActiveFrameTrial(i - 1);
						}

						return;
					}
				}
			}
		}
	}
}

void WizardDigitizationFrame::goToFirstTrackedFrame()
{
	if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
	{
		int trackidx = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
		if (trackidx >= 0)
		{
			for (int i = State::getInstance()->getActiveFrameTrial(); i >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i--)
			{
				for (unsigned int j = 0; j < Project::getInstance()->getCameras().size(); j++)
				{
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackidx]->getStatus2D()[j][i] <= 0)
					{
						if (i + 1 < State::getInstance()->getActiveFrameTrial())
						{
							State::getInstance()->changeActiveFrameTrial(i + 1);
						}

						return;
					}
				}
			}
		}
	}
}

void WizardDigitizationFrame::trackSelectedPointToPrevFrame()
{
	if (frame->pushButton_PointPrev->isEnabled())
	{
		on_pushButton_PointPrev_clicked();
	}
}

void WizardDigitizationFrame::on_pushButton_PointNext_clicked()
{
	//if (isTracking) return;
	uncheckTrackButtons();
	//isTracking = true;

	trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
	trackType = 1;
	trackDirection = 1;

	track();
}

void WizardDigitizationFrame::on_pushButton_PointPrev_clicked()
{
	//if (isTracking) return;
	uncheckTrackButtons();
	//isTracking = true;

	trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
	trackType = 1;
	trackDirection = -1;

	track();
}

void WizardDigitizationFrame::on_pushButton_PointForw_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked)
	{
		frame->pushButton_PointForw->setChecked(true);

		trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
		trackType = 1;
		trackDirection = 2;

		track();
	}
}

void WizardDigitizationFrame::on_pushButton_PointBack_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked)
	{
		frame->pushButton_PointBack->setChecked(true);

		trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
		trackType = 1;
		trackDirection = -2;

		track();
	}
}

void WizardDigitizationFrame::on_pushButton_RBNext_clicked()
{
	uncheckTrackButtons();

	trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx();
	trackType = 2;
	trackDirection = 1;

	track();
}

void WizardDigitizationFrame::on_pushButton_RBPrev_clicked()
{
	uncheckTrackButtons();

	trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx();
	trackType = 2;
	trackDirection = -1;

	track();
}

void WizardDigitizationFrame::on_pushButton_RBForw_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked)
	{
		frame->pushButton_RBForw->setChecked(true);

		trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx();
		trackType = 2;
		trackDirection = 2;

		track();
	}
}

void WizardDigitizationFrame::on_pushButton_RBBack_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked)
	{
		frame->pushButton_RBBack->setChecked(true);

		trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx();
		trackType = 2;
		trackDirection = -2;

		track();
	}
}

void WizardDigitizationFrame::on_pushButton_AllNext_clicked()
{
	//if (isTracking) return;
	uncheckTrackButtons();
	//isTracking = true;

	trackID = -1;
	trackType = 3;
	trackDirection = 1;

	track();
}

void WizardDigitizationFrame::on_pushButton_AllPrev_clicked()
{
	//if (isTracking) return;
	uncheckTrackButtons();
	//isTracking = true;

	trackID = -1;
	trackType = 3;
	trackDirection = -1;

	track();
}

void WizardDigitizationFrame::on_pushButton_AllForw_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked)
	{
		frame->pushButton_AllForw->setChecked(true);

		trackID = -1;
		trackType = 3;
		trackDirection = 2;

		track();
	}
}

void WizardDigitizationFrame::on_pushButton_AllBack_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked)
	{
		frame->pushButton_AllBack->setChecked(true);

		trackID = -1;
		trackType = 3;
		trackDirection = -2;

		track();
	}
}

void WizardDigitizationFrame::on_pushButton_InterpolateActive_clicked(bool checked)
{
	Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker()->interpolate();

	MainWindow::getInstance()->redrawGL();
}

void WizardDigitizationFrame::on_pushButton_InterpolateAll_clicked(bool checked)
{
	for (std::vector<Marker *>::const_iterator it = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().begin();
		it < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().end(); ++it)
		(*it)->interpolate();

	MainWindow::getInstance()->redrawGL();
}

void WizardDigitizationFrame::uncheckTrackButtons()
{
	frame->pushButton_PointForw->setChecked(false);
	frame->pushButton_PointBack->setChecked(false);
	frame->pushButton_RBForw->setChecked(false);
	frame->pushButton_RBBack->setChecked(false);
	frame->pushButton_AllForw->setChecked(false);
	frame->pushButton_AllBack->setChecked(false);
	trackID = -1;
	trackType = 0;
	trackDirection = 0;
	singleTrack = false;
	//isTracking = false;
	setDialog();
}

void WizardDigitizationFrame::on_checkBoxMarkerIds_clicked()
{
	Settings::getInstance()->set("TrialDrawMarkerIds", frame->checkBoxMarkerIds->isChecked());
	MainWindow::getInstance()->redrawGL();
}

void WizardDigitizationFrame::on_checkBoxRigidBodyConstellation_clicked()
{
	Settings::getInstance()->set("TrialDrawRigidBodyConstellation", frame->checkBoxRigidBodyConstellation->isChecked());
	MainWindow::getInstance()->redrawGL();
}

void WizardDigitizationFrame::on_checkBoxRigidBodyMeshmodels_clicked()
{
	Settings::getInstance()->set("TrialDrawRigidBodyMeshmodels", frame->checkBoxRigidBodyMeshmodels->isChecked());
	MainWindow::getInstance()->redrawGL();
}

void WizardDigitizationFrame::on_checkBox_DrawFiltered_clicked()
{
	Settings::getInstance()->set("TrialDrawFiltered", frame->checkBox_DrawFiltered->isChecked());
	MainWindow::getInstance()->redrawGL();
}
