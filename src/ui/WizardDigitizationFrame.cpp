//  ----------------------------------
//  XMALab -- Copyright � 2015, Brown University, Providence, RI.
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
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
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
#include "core/Camera.h"
#include <processing/MarkerDetection.h>
#include <processing/MarkerTracking.h>


using namespace xma;

WizardDigitizationFrame::WizardDigitizationFrame(QWidget* parent) :
	QFrame(parent),
	frame(new Ui::WizardDigitizationFrame)
{
	frame->setupUi(this);

#ifdef __APPLE__
	frame->pushButton->setMinimumHeight(26);
                                                    
    frame->toolButton_RBBack->setMinimumHeight(26);
    frame->toolButton_RBPrev->setMinimumHeight(26);
    frame->toolButton_RBForw->setMinimumHeight(26);
    frame->toolButton_RBNext->setMinimumHeight(26);
    
    frame->toolButton_AllPrev->setMinimumHeight(26);
    frame->toolButton_AllNext->setMinimumHeight(26);
    frame->toolButton_AllBack->setMinimumHeight(26);
    frame->toolButton_AllForw->setMinimumHeight(26);
    
    frame->toolButton_PointNext->setMinimumHeight(26);
    frame->toolButton_PointPrev->setMinimumHeight(26);
    frame->toolButton_PointBack->setMinimumHeight(26);
    frame->toolButton_PointForw->setMinimumHeight(26);

	frame->toolButton_InterpolateActive->setMinimumHeight(26);
	frame->toolButton_InterpolateAll->setMinimumHeight(26);

	frame->toolButton_RBBack->setFocusPolicy(Qt::StrongFocus);
	frame->toolButton_RBPrev->setFocusPolicy(Qt::StrongFocus);
	frame->toolButton_RBForw->setFocusPolicy(Qt::StrongFocus);
	frame->toolButton_RBNext->setFocusPolicy(Qt::StrongFocus);

	frame->toolButton_AllPrev->setFocusPolicy(Qt::StrongFocus);
	frame->toolButton_AllNext->setFocusPolicy(Qt::StrongFocus);
	frame->toolButton_AllBack->setFocusPolicy(Qt::StrongFocus);
	frame->toolButton_AllForw->setFocusPolicy(Qt::StrongFocus);

	frame->toolButton_PointNext->setFocusPolicy(Qt::StrongFocus);
	frame->toolButton_PointPrev->setFocusPolicy(Qt::StrongFocus);
	frame->toolButton_PointBack->setFocusPolicy(Qt::StrongFocus);
	frame->toolButton_PointForw->setFocusPolicy(Qt::StrongFocus);
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
		if (trackType == 1) stopTracking();
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
			if (Project::getInstance()->getCameras()[i]->isVisible() &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->getStatus2D()[i][startFrame] > UNDEFINED &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->getStatus2D()[i][endFrame] != UNTRACKABLE &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->getStatus2D()[i][endFrame] <= (Settings::getInstance()->getBoolSetting("RetrackOptimizedTrackedPoints") ? TRACKED_AND_OPTIMIZED : TRACKED)
				&& !(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->getStatus2D()[i][endFrame] == INTERPOLATED && !Settings::getInstance()->getBoolSetting("TrackInterpolatedPoints")))
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
		if (Project::getInstance()->getCameras()[i]->isVisible() && 
			Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[tmptrackID]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED)
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
			if (Project::getInstance()->getCameras()[i]->isVisible() && 
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->getStatus2D()[i][startFrame] > UNDEFINED &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->getStatus2D()[i][endFrame] > UNTRACKABLE &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->getStatus2D()[i][endFrame] <= (Settings::getInstance()->getBoolSetting("RetrackOptimizedTrackedPoints") ? TRACKED_AND_OPTIMIZED : TRACKED)
				&& !(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->getStatus2D()[i][endFrame] == INTERPOLATED && !Settings::getInstance()->getBoolSetting("TrackInterpolatedPoints")))
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
			if (Project::getInstance()->getCameras()[i]->isVisible() &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED)
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
			if (Project::getInstance()->getCameras()[i]->isVisible() && 
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][startFrame] > UNDEFINED &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][endFrame] > UNTRACKABLE &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][endFrame] <= (Settings::getInstance()->getBoolSetting("RetrackOptimizedTrackedPoints") ? TRACKED_AND_OPTIMIZED : TRACKED)
				&& !(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][endFrame] == INTERPOLATED && !Settings::getInstance()->getBoolSetting("TrackInterpolatedPoints")))
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
			if (Project::getInstance()->getCameras()[i]->isVisible() && 
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED)
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
			if (Project::getInstance()->getCameras()[i]->isVisible() && 
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED &&
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
				if (Project::getInstance()->getCameras()[i]->isVisible() &&
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[*it]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED &&
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
				if (Project::getInstance()->getCameras()[i]->isVisible() && 
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED &&
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
	if (Project::getInstance()->isCalibrated() || (Project::getInstance()->getCalibration() == NO_CALIBRATION))
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
	if (Project::getInstance()->isCalibrated() || (Project::getInstance()->getCalibration() == NO_CALIBRATION))
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() > 0)
			{
#ifdef __APPLE__
				frame->label->show();
				frame->label->setText("");
#else
				frame->label->hide();
#endif
				frame->pushButton->hide();
				frame->groupBox_All->show();
				frame->groupBox_Point->show();
				frame->groupBox_RB->show();
				frame->groupBox_Interpolation->show();
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
				frame->groupBox_Interpolation->hide();
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
			frame->groupBox_Interpolation->hide();
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
		frame->groupBox_Interpolation->hide();
	}
}

void WizardDigitizationFrame::stopTracking()
{
	uncheckTrackButtons();
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
			State::getInstance()->changeActiveFrameTrial(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame());
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
			State::getInstance()->changeActiveFrameTrial(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame()-1);
		}
	}
}

void WizardDigitizationFrame::trackPointsShortcut(bool direction, bool selected, bool continous)
{
	if (direction)
	{
		if (selected)
		{
			if (continous)
			{
				std::cerr << "Clicked" << std::endl;
				if (frame->toolButton_RBForw->isEnabled())
				{
					std::cerr << "Clicked2" << std::endl;
					frame->toolButton_RBForw->click();
				}
			}
			else
			{
				if (frame->toolButton_RBNext->isEnabled())
				{
					frame->toolButton_RBNext->click();
				}
			}
		}
		else
		{
			if (continous)
			{
				std::cerr << "Clicked" << std::endl;
				if (frame->toolButton_PointForw->isEnabled())
				{
					std::cerr << "Clicked2" << std::endl;
					frame->toolButton_PointForw->click();
				}
			}
			else
			{
				if (frame->toolButton_PointNext->isEnabled())
				{
					frame->toolButton_PointNext->click();
				}
			}
		}
	} 
	else
	{
		if (selected)
		{
			if (continous)
			{
				std::cerr << "Clicked" << std::endl;
				if (frame->toolButton_RBBack->isEnabled())
				{
					std::cerr << "Clicked2" << std::endl;
					frame->toolButton_RBBack->click();
				}
			}
			else
			{
				if (frame->toolButton_RBPrev->isEnabled())
				{
					frame->toolButton_RBPrev->click();
				}
			}
		}
		else
		{
			if (continous)
			{
				std::cerr << "Clicked" << std::endl;
				if (frame->toolButton_PointBack->isEnabled())
				{
					std::cerr << "Clicked2" << std::endl;
					frame->toolButton_PointBack->click();
				}
			}
			else
			{
				if (frame->toolButton_PointPrev->isEnabled())
				{
					frame->toolButton_PointPrev->click();
				}
			}
		}
	}
}

void WizardDigitizationFrame::on_toolButton_PointNext_clicked()
{
	//if (isTracking) return;
	uncheckTrackButtons();
	//isTracking = true;

	trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
	trackType = 1;
	trackDirection = 1;

	track();
}

void WizardDigitizationFrame::on_toolButton_PointPrev_clicked()
{
	//if (isTracking) return;
	uncheckTrackButtons();
	//isTracking = true;

	trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
	trackType = 1;
	trackDirection = -1;

	track();
}

void WizardDigitizationFrame::on_toolButton_PointForw_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked)
	{
		frame->toolButton_PointForw->setChecked(true);

		trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
		trackType = 1;
		trackDirection = 2;

		track();
	}
}

void WizardDigitizationFrame::on_toolButton_PointBack_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked)
	{
		frame->toolButton_PointBack->setChecked(true);

		trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
		trackType = 1;
		trackDirection = -2;

		track();
	}
}

void WizardDigitizationFrame::on_toolButton_RBNext_clicked()
{
	uncheckTrackButtons();

	trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx();
	trackType = 2;
	trackDirection = 1;

	track();
}

void WizardDigitizationFrame::on_toolButton_RBPrev_clicked()
{
	uncheckTrackButtons();

	trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx();
	trackType = 2;
	trackDirection = -1;

	track();
}

void WizardDigitizationFrame::on_toolButton_RBForw_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked)
	{
		frame->toolButton_RBForw->setChecked(true);

		trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx();
		trackType = 2;
		trackDirection = 2;

		track();
	}
}

void WizardDigitizationFrame::on_toolButton_RBBack_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked)
	{
		frame->toolButton_RBBack->setChecked(true);

		trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx();
		trackType = 2;
		trackDirection = -2;

		track();
	}
}

void WizardDigitizationFrame::on_toolButton_AllNext_clicked()
{
	//if (isTracking) return;
	uncheckTrackButtons();
	//isTracking = true;

	trackID = -1;
	trackType = 3;
	trackDirection = 1;

	track();
}

void WizardDigitizationFrame::on_toolButton_AllPrev_clicked()
{
	//if (isTracking) return;
	uncheckTrackButtons();
	//isTracking = true;

	trackID = -1;
	trackType = 3;
	trackDirection = -1;

	track();
}

void WizardDigitizationFrame::on_toolButton_AllForw_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked)
	{
		frame->toolButton_AllForw->setChecked(true);

		trackID = -1;
		trackType = 3;
		trackDirection = 2;

		track();
	}
}

void WizardDigitizationFrame::on_toolButton_AllBack_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked)
	{
		frame->toolButton_AllBack->setChecked(true);

		trackID = -1;
		trackType = 3;
		trackDirection = -2;

		track();
	}
}

void WizardDigitizationFrame::on_toolButton_InterpolateActive_clicked(bool checked)
{
	Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker()->interpolate();

	MainWindow::getInstance()->redrawGL();
}

void WizardDigitizationFrame::on_toolButton_InterpolateAll_clicked(bool checked)
{
	for (std::vector<Marker *>::const_iterator it = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().begin();
		it < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().end(); ++it)
		(*it)->interpolate();

	MainWindow::getInstance()->redrawGL();
}

void WizardDigitizationFrame::uncheckTrackButtons()
{
	frame->toolButton_PointForw->setChecked(false);
	frame->toolButton_PointBack->setChecked(false);
	frame->toolButton_RBForw->setChecked(false);
	frame->toolButton_RBBack->setChecked(false);
	frame->toolButton_AllForw->setChecked(false);
	frame->toolButton_AllBack->setChecked(false);
	trackID = -1;
	trackType = 0;
	trackDirection = 0;
	singleTrack = false;
	//isTracking = false;
	setDialog();
}

