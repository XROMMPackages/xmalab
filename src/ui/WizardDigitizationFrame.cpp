#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/WizardDigitizationFrame.h"
#include "ui/MainWindow.h"
#include "ui_WizardDigitizationFrame.h"
#include "ui/PointsDockWidget.h"
#include "ui/PlotWindow.h"

#include "core/Project.h"
#include "core/Trial.h"
#include "core/Camera.h"
#include "core/Settings.h"
#include "core/Marker.h"
#include "core/RigidBody.h"
#include <processing/MarkerDetection.h>
#include <processing/MarkerTracking.h>


using namespace xma;

WizardDigitizationFrame::WizardDigitizationFrame(QWidget *parent) :
												QFrame(parent),
												frame(new Ui::WizardDigitizationFrame){

	frame->setupUi(this);
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

#endif
	connect(State::getInstance(), SIGNAL(activeCameraChanged(int)), this, SLOT(activeCameraChanged(int)));
	connect(State::getInstance(), SIGNAL(activeFrameTrialChanged(int)), this, SLOT(activeFrameTrialChanged(int)));
	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));

	trackID = 0; 
	trackType = 0; 
	trackDirection = 0; 
	singleTrack = false;
	//track_timer = new QTimer(this);
	//track_timer->setSingleShot(true);
	//connect(track_timer, SIGNAL(timeout()), this, SLOT(on_pushButton_trackPoint_clicked()));
}

void WizardDigitizationFrame::addDigitizationPoint(int camera, double x, double y)
{
	Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->setActiveToNextUndefinedMarker(camera);
	PlotWindow::getInstance()->updateMarkers(true);

	Marker * marker = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker();

	if (marker != NULL){
		marker->setPoint(camera, State::getInstance()->getActiveFrameTrial(), x, y, MANUAL_REFINED);

		PointsDockWidget::getInstance()->reloadListFromObject();
		MainWindow::getInstance()->redrawGL();

		MarkerDetection * markerdetection = new MarkerDetection(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveTrial(), State::getInstance()->getActiveFrameTrial(), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		markerdetection->detectMarker();
	}
}

void WizardDigitizationFrame::selectDigitizationPoint(int camera, double x, double y)
{
	double dist = 20;
	int idx = -1;
	for (int i = 0; i < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); i++)
	{
		double x1 = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->getPoints2D()[camera][State::getInstance()->getActiveFrameTrial()].x;
		double y1 = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->getPoints2D()[camera][State::getInstance()->getActiveFrameTrial()].y;
		double tmpdist = sqrt((x - x1)*(x - x1) + (y - y1)*(y - y1));

		if (tmpdist < dist)
		{
			dist = tmpdist;
			idx = i;
		}
	}

	if (idx >= 0)
	{
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->setActiveMarkerIdx(idx);
		MainWindow::getInstance()->redrawGL();
	}
}

void WizardDigitizationFrame::moveDigitizationPoint(int camera, double x, double y, bool noDetection)
{
	Marker * marker = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker();

	if (marker != NULL){
		marker->setPoint(camera, State::getInstance()->getActiveFrameTrial(), x, y, MANUAL_REFINED);

		if (!noDetection)
		{
			MarkerDetection * markerdetection = new MarkerDetection(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveTrial(), State::getInstance()->getActiveFrameTrial(), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
			markerdetection->detectMarker();
		}
		else{
			MainWindow::getInstance()->redrawGL();
			setDialog();
		}
	}
}

WizardDigitizationFrame::~WizardDigitizationFrame(){
	delete frame;
}

void WizardDigitizationFrame::activeCameraChanged(int activeCamera){
	if (State::getInstance()->getWorkspace() == DIGITIZATION){
		setDialog();
	}
}

void WizardDigitizationFrame::activeFrameTrialChanged(int activeFrame){
	if (State::getInstance()->getWorkspace() == DIGITIZATION){
		setDialog();
	}
}

void WizardDigitizationFrame::workspaceChanged(work_state workspace){
	if (workspace == DIGITIZATION){
		setDialog();
	}
}

void WizardDigitizationFrame::trackSinglePoint()
{
	int startFrame = State::getInstance()->getActiveFrameTrial();
	int endFrame;
	tmptrackID = trackID;
	if (trackID >= 0 && trackID <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size()){
		if (trackDirection > 0){
			endFrame = startFrame + 1;
		}
		else
		{
			endFrame = startFrame - 1;
		}
		std::vector< MarkerTracking * > trackers;
		for (int i = 0; i < Project::getInstance()->getCameras().size(); i++){
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->getStatus2D()[i][startFrame] > UNDEFINED &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->getStatus2D()[i][endFrame] <= TRACKED){
				MarkerTracking * markertracking = new MarkerTracking(i, State::getInstance()->getActiveTrial(), startFrame, endFrame, trackID, trackDirection > 0);
				connect(markertracking, SIGNAL(trackMarker_finished()), this, SLOT(trackSinglePointFinished()));
				trackers.push_back(markertracking);
			}
		}

		State::getInstance()->setDisableDraw(true);
		State::getInstance()->changeActiveFrameTrial(endFrame);

		if (trackers.size() > 0){
			for (int i = 0; i < trackers.size(); i++){
				trackers[i]->trackMarker();
			}
			trackers.clear();
		}
		else if (abs(trackDirection) > 1){
			checkIfValid();
		}
	}
	else
	{
		uncheckTrackButtons();
	}
}

void WizardDigitizationFrame::trackSinglePointFinished(){
	std::vector< MarkerDetection * > detectors;
	for (int i = 0; i < Project::getInstance()->getCameras().size(); i++){
		if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[tmptrackID]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED){
			MarkerDetection * markerdetection = new MarkerDetection(i, State::getInstance()->getActiveTrial(), State::getInstance()->getActiveFrameTrial(), tmptrackID,
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[tmptrackID]->getSize() * 2, true);
			detectors.push_back(markerdetection);
			connect(markerdetection, SIGNAL(detectMarker_finished()), this, SLOT(checkIfValid()));
		}
	}
	
	if (detectors.size() > 0){
		for (int i = 0; i < detectors.size(); i++){
			detectors[i]->detectMarker();
		}
		detectors.clear();
	}
	else{
		checkIfValid();
	} 
	tmptrackID = -1;
}


void WizardDigitizationFrame::trackRB()
{
	
}

void WizardDigitizationFrame::trackRBFinished()
{
	
}

void WizardDigitizationFrame::trackAll()
{
	int startFrame = State::getInstance()->getActiveFrameTrial();
	int endFrame;

	if (trackDirection > 0){
		endFrame = startFrame + 1;
	}
	else
	{
		endFrame = startFrame - 1;
	}
	std::vector< MarkerTracking * > trackers;
	for (int j = 0; j < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); j++){
		for (int i = 0; i < Project::getInstance()->getCameras().size(); i++){
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][startFrame] > UNDEFINED &&
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][endFrame] <= TRACKED){
				MarkerTracking * markertracking = new MarkerTracking(i, State::getInstance()->getActiveTrial(), startFrame, endFrame, j, trackDirection > 0);
				connect(markertracking, SIGNAL(trackMarker_finished()), this, SLOT(trackAllFinished()));
				trackers.push_back(markertracking);
			}
		}
	}
	State::getInstance()->setDisableDraw(true);
	State::getInstance()->changeActiveFrameTrial(endFrame);

	if (trackers.size() > 0){
		for (int i = 0; i < trackers.size(); i++){
			trackers[i]->trackMarker();
		}
		trackers.clear();
	}
	else if (abs(trackDirection) > 1){
		checkIfValid();
	}
	
}

void WizardDigitizationFrame::trackAllFinished()
{
	std::vector< MarkerDetection * > detectors;
	for (int j = 0; j < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); j++){
		for (int i = 0; i < Project::getInstance()->getCameras().size(); i++){
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED){
				MarkerDetection * markerdetection = new MarkerDetection(i, State::getInstance()->getActiveTrial(), State::getInstance()->getActiveFrameTrial(), j,
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getSize() * 2, true);
				detectors.push_back(markerdetection);
				connect(markerdetection, SIGNAL(detectMarker_finished()), this, SLOT(checkIfValid()));
			}
		}
	}

	if (detectors.size() > 0){
		for (int i = 0; i < detectors.size(); i++){
			detectors[i]->detectMarker();
		}
		detectors.clear();
	}
	else{
		checkIfValid();
	}
}


void WizardDigitizationFrame::checkIfValid()
{

	bool valid = true;
	if (trackType == 1)
	{
		for (int i = 0; i < Project::getInstance()->getCameras().size(); i++){
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED){
				valid = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackID]->isValid(i, State::getInstance()->getActiveFrameTrial()) && valid;
			}
		}

	}
	else if (trackType == 3)
	{
		for (int j = 0; j < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); j++){
			for (int i = 0; i < Project::getInstance()->getCameras().size(); i++){
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] == TRACKED){
					valid = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[j]->isValid(i, State::getInstance()->getActiveFrameTrial()) && valid;
				}
			}
		}
	}

	if (!valid)
	{
		uncheckTrackButtons();
	}

	State::getInstance()->setDisableDraw(false);
	State::getInstance()->changeActiveFrameTrial(State::getInstance()->getActiveFrameTrial(), true);

	track();
}

void WizardDigitizationFrame::track(){
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
		frame->pushButton->setVisible(true);
		if ((Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)){
			if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() > 0){
				frame->label->hide();
				frame->pushButton->hide();
				frame->groupBox_All->show();
				frame->groupBox_Point->show();
				
				if (trackDirection == 0){
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx() >= 0)
					{
						bool visible = false;
						for (int i = 0; i < Project::getInstance()->getCameras().size(); i++)
						{
							if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker()->getStatus2D()[i][State::getInstance()->getActiveFrameTrial()] > UNDEFINED)
							{
								visible = true;
							}
						}

						frame->groupBox_Point->setEnabled(visible);
					}

					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size() > 0){
						frame->groupBox_RB->show();
						if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx() >= 0)
						{
							bool visible = false;

							if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRB()->getPoseComputed()[State::getInstance()->getActiveFrameTrial()])
							{
								visible = true;
							}

							frame->groupBox_RB->setEnabled(visible);
						}
					}
					else
					{
						frame->groupBox_RB->hide();
					}
				}
			}else
			{
				frame->label->show();
				frame->label->setText("You first have to add points by using CTRL + left Click in the image or by using the point widget");
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
	if ((Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0))
	{
		int trackidx = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
		for (int i = State::getInstance()->getActiveFrameTrial(); i < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame(); i++)
		{
			for (int j = 0; j < Project::getInstance()->getCameras().size(); j++){
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackidx]->getStatus2D()[j][i] <= 0){
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

void WizardDigitizationFrame::goToFirstTrackedFrame()
{
	if ((Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0))
	{
		int trackidx = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
		for (int i = State::getInstance()->getActiveFrameTrial(); i >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i--)
		{
			for (int j = 0; j < Project::getInstance()->getCameras().size(); j++){
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[trackidx]->getStatus2D()[j][i] <= 0){
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

void WizardDigitizationFrame::trackSelectedPointToPrevFrame()
{
	if (frame->pushButton_PointPrev->isEnabled())
	{
		on_pushButton_PointPrev_clicked();
	}
}

void WizardDigitizationFrame::on_pushButton_PointNext_clicked()
{
	uncheckTrackButtons();

	trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
	trackType = 1;
	trackDirection = 1;

	track();
}

void WizardDigitizationFrame::on_pushButton_PointPrev_clicked()
{
	uncheckTrackButtons();

	trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
	trackType = 1;
	trackDirection = -1;

	track();
}

void WizardDigitizationFrame::on_pushButton_PointForw_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked){
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
	if (checked){
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
	if (checked){
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
	if (checked){
		frame->pushButton_RBBack->setChecked(true);

		trackID = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx();
		trackType = 2;
		trackDirection = -2;

		track();
	}
}

void WizardDigitizationFrame::on_pushButton_AllNext_clicked()
{
	uncheckTrackButtons();

	trackID = -1;
	trackType = 3;
	trackDirection = 1;

	track();
}

void WizardDigitizationFrame::on_pushButton_AllPrev_clicked()
{
	uncheckTrackButtons();

	trackID = -1;
	trackType = 3;
	trackDirection = -1;

	track();
}

void WizardDigitizationFrame::on_pushButton_AllForw_clicked(bool checked)
{
	uncheckTrackButtons();
	if (checked){
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
	if (checked){
		frame->pushButton_AllBack->setChecked(true);

		trackID = -1;
		trackType = 3;
		trackDirection = -2;

		track();
	}
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
	setDialog();
}