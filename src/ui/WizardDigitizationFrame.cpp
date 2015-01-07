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
}

void WizardDigitizationFrame::addDigitizationPoint(int camera, double x, double y)
{
	if (!Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->isActiveMarkerUndefined(camera)){
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->addMarker();
	}
	Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->moveMarker(camera, x, y);
	PointsDockWidget::getInstance()->reloadListFromObject();
	MainWindow::getInstance()->redrawGL();
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