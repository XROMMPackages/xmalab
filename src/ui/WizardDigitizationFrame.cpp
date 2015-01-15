#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/WizardDigitizationFrame.h"
#include "ui/MainWindow.h"
#include "ui_WizardDigitizationFrame.h"
#include "ui/PointsDockWidget.h"

#include "core/Project.h"
#include "core/Trial.h"
#include "core/Camera.h"
#include "core/Settings.h"
#include "core/Marker.h"
#include <processing/MarkerDetection.h>
#include <processing/MarkerTracking.h>


using namespace xma;

WizardDigitizationFrame::WizardDigitizationFrame(QWidget *parent) :
												QFrame(parent),
												frame(new Ui::WizardDigitizationFrame){

	frame->setupUi(this);
#ifdef __APPLE__
	frame->pushButton->setMinimumHeight(26);
#endif
	connect(State::getInstance(), SIGNAL(activeCameraChanged(int)), this, SLOT(activeCameraChanged(int)));
	connect(State::getInstance(), SIGNAL(activeFrameCalibrationChanged(int)), this, SLOT(activeFrameCalibrationChanged(int)));
	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));

	track_timer = new QTimer(this);
	track_timer->setSingleShot(true);
	connect(track_timer, SIGNAL(timeout()), this, SLOT(on_pushButton_trackPoint_clicked()));
}

void WizardDigitizationFrame::addDigitizationPoint(int camera, double x, double y)
{
	if (!Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->isActiveMarkerUndefined(camera)){
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->addMarker();
	}
	Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->moveMarker(camera, x, y);
	PointsDockWidget::getInstance()->reloadListFromObject();
	MainWindow::getInstance()->redrawGL();

	MarkerDetection * markerdetection = new MarkerDetection(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveTrial(), State::getInstance()->getActiveFrameTrial(), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
	markerdetection->detectMarker();
}

void WizardDigitizationFrame::moveDigitizationPoint(int camera, double x, double y)
{
	Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->moveMarker(camera, x, y);
	MainWindow::getInstance()->redrawGL();
}

WizardDigitizationFrame::~WizardDigitizationFrame(){
	delete frame;
}

void WizardDigitizationFrame::activeCameraChanged(int activeCamera){
	setDialog();
}

void WizardDigitizationFrame::activeFrameCalibrationChanged(int activeFrame){
	setDialog();
}

void WizardDigitizationFrame::workspaceChanged(work_state workspace){
	if (workspace == DIGITIZATION){
		setDialog();
	}
}

void WizardDigitizationFrame::on_pushButton_trackPoint_clicked()
{
	MarkerTracking * markertracking = new MarkerTracking(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveTrial(), State::getInstance()->getActiveFrameTrial(), State::getInstance()->getActiveFrameTrial()+1, Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
	State::getInstance()->changeActiveFrameTrial(State::getInstance()->getActiveFrameTrial() + 1);
	connect(markertracking, SIGNAL(trackMarker_finished()), this, SLOT(runTrackMarkerFinished()));
	markertracking->trackMarker();
}

void WizardDigitizationFrame::runTrackMarkerFinished(){
	MarkerDetection * markerdetection = new MarkerDetection(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveTrial(), State::getInstance()->getActiveFrameTrial(), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx(),
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx()]->getSize() + 3);
	connect(markerdetection, SIGNAL(detectMarker_finished()), this, SLOT(startTimer()));
	markerdetection->detectMarker();
}

void WizardDigitizationFrame::startTimer(){
	/*if (State::getInstance()->getActiveFrameTrial() < 500){
		track_timer->start(10);
	}*/
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
		if (Project::getInstance()->getTrials().size() > 0)
		{
			
		}
		else
		{
			frame->label->setText("You first have to add a trial");	
			frame->pushButton->setText("Add Trial");
		}
	}
	else
	{
		frame->label->setText("You first have to calibrate your cameras");
		frame->pushButton->setVisible(false);
	}
}