/*
 * ProgressDialog.cpp
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/PlotWindow.h"
#include "ui_PlotWindow.h"
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QFileDialog>

#include <fstream>

#include "core/Trial.h"
#include "core/Project.h"
#include "core/Marker.h"
#include "core/Camera.h"
#include "core/RigidBody.h"

#include "ui/State.h"
#include "ui/MainWindow.h"
#include "ui/ConfirmationDialog.h"
#include "core/Settings.h"

#ifdef WIN32
#define OS_SEP "\\"
#else
#define OS_SEP "/"
#endif
#include "Shortcuts.h"

using namespace xma;

PlotWindow* PlotWindow::instance = NULL;

PlotWindow::PlotWindow(QWidget *parent) :QDockWidget(parent), dock(new Ui::PlotWindow)
{
	dock->setupUi(this);

	//setup Plot 1
	//plotWidget = new QCustomPlot(this);
	frameMarker = new QCPItemLine(dock->plotWidget);
	dock->plotWidget->addItem(frameMarker);
	//dock->plotWidget->installEventFilter(this);
	installEventFilterToChildren(this);


	selectionMarker = new QCPItemRect(dock->plotWidget);
	dock->plotWidget->addItem(selectionMarker);
	selectionMarker->setPen(QPen(QColor(255, 255, 0, 50)));
	selectionMarker->setBrush(QBrush(QColor(255,255,0,150)));
	startFrame = 0;
	endFrame = 0;

	dock->plotWidget->setFocusPolicy(Qt::StrongFocus);
	dock->plotWidget->setInteraction(QCP::iRangeDrag, true);
	dock->plotWidget->axisRect()->setRangeDrag(Qt::Horizontal);
	dock->plotWidget->setInteraction(QCP::iRangeZoom, true);
	dock->plotWidget->axisRect()->setRangeZoom(Qt::Horizontal);

	//Setup DockWidget
	setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
	this->resize(500,500);

	dock->comboBoxPlotType->setCurrentIndex(0);
	on_comboBoxPlotType_currentIndexChanged(0);
	plot2D(-1);

	updating = false;

	connect(State::getInstance(), SIGNAL(activeTrialChanged(int)), this, SLOT(activeTrialChanged(int)));
	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));
	connect(PointsDockWidget::getInstance(), SIGNAL(activePointChanged(int)), this, SLOT(activePointChanged(int)));
	connect(PointsDockWidget::getInstance(), SIGNAL(activeRigidBodyChanged(int)), this, SLOT(activeRigidBodyChanged(int)));
}

void PlotWindow::installEventFilterToChildren(QObject* object)
{
	QObjectList list = object->children();
	for (int i = 0; i < list.size(); i++)
	{
		installEventFilterToChildren(list.at(i));
	}

	object->installEventFilter(this);
}

PlotWindow::~PlotWindow(){
	instance = NULL;
}

PlotWindow* PlotWindow::getInstance()
{
	if (!instance)
	{
		instance = new PlotWindow(MainWindow::getInstance());
		MainWindow::getInstance()->addDockWidget(Qt::TopDockWidgetArea, instance);
	}
	return instance;
}

void PlotWindow::deleteData()
{
	int cam = -1;
	int frameStart;
	int frameEnd;
	frameStart = (endFrame > startFrame) ? startFrame : endFrame;
	frameEnd = (endFrame > startFrame) ? endFrame : startFrame;
	QString cameras;
	if (dock->comboBoxPlotType->currentIndex() == 0 || dock->comboBoxPlotType->currentIndex() == 3)
	{
		cam = dock->comboBoxCamera->currentIndex() - 1;
		cameras = dock->comboBoxCamera->currentText();
	}
	else if (dock->comboBoxPlotType->currentIndex() == 1 || dock->comboBoxPlotType->currentIndex() == 2 || dock->comboBoxPlotType->currentIndex() == 4)
	{
		cam = -1;
		cameras = "All Cameras";
	}
	
	if (dock->comboBoxPlotType->currentIndex() == 0 || dock->comboBoxPlotType->currentIndex() == 1 || dock->comboBoxPlotType->currentIndex() == 3)
	{	
		if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to delete your data for " + cameras + " for Marker " + dock->comboBoxMarker1->currentText() + " from Frame " +
			QString::number(frameStart + 1) + " to " + QString::number(frameEnd + 1))) return;
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker1->currentIndex()]->resetMultipleFrames(cam, frameStart, frameEnd);
	}
	else if (dock->comboBoxPlotType->currentIndex() == 2)
	{
		if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to delete your data for " + cameras + " for Marker " + dock->comboBoxMarker1->currentText() + " and Marker " + dock->comboBoxMarker2->currentText() + " from Frame " +
			QString::number(frameStart + 1) + " to " + QString::number(frameEnd + 1))) return;
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker1->currentIndex()]->resetMultipleFrames(cam, frameStart, frameEnd);
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker2->currentIndex()]->resetMultipleFrames(cam, frameStart, frameEnd);
	}
	else if (dock->comboBoxPlotType->currentIndex() == 4)
	{
		if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to delete your data for " + cameras + " for all marker of Rigid Body " + dock->comboBoxRigidBody->currentText() + " from Frame " + QString::number(frameStart + 1) + " to " + QString::number(frameEnd + 1))) return;
		for (int i = 0; i < Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[dock->comboBoxRigidBody->currentIndex()]->getPointsIdx().size(); i++)
		{
			int idx = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[dock->comboBoxRigidBody->currentIndex()]->getPointsIdx()[i];
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[idx]->resetMultipleFrames(cam, frameStart, frameEnd);
		}

		on_pushButtonUpdate_clicked();
	}
	MainWindow::getInstance()->redrawGL();
}

bool PlotWindow::eventFilter(QObject *target, QEvent *event)
{
	if (event->type() == QEvent::KeyPress)
	{
		if(Shortcuts::getInstance()->checkShortcut(target, event))
		{
			return true;
		}

		QKeyEvent *_keyEvent = static_cast<QKeyEvent*>(event);
		if (_keyEvent->key() == Qt::Key_Delete || _keyEvent->key() == Qt::Key_Backspace)
		{
			deleteData();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_S && _keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			if (dock->comboBoxPlotType->currentIndex() == 2)
			{
				QString text = "Save intermarker distance between Marker " + dock->comboBoxMarker1->currentText() + " and Marker " + dock->comboBoxMarker2->currentText();
				QString fileName = QFileDialog::getSaveFileName(this,
					text, Settings::getInstance()->getLastUsedDirectory() + OS_SEP + "Distance" + dock->comboBoxMarker1->currentText() + "To" + dock->comboBoxMarker2->currentText() + ".csv", tr("Comma seperated data (*.csv)"));
				if (fileName.isNull() == false)
				{
					std::ofstream outfile(fileName.toAscii().data());
					for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++, count++)
					{

						if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker1->currentIndex()]->getStatus3D()[i] > UNDEFINED && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker2->currentIndex()]->getStatus3D()[i] > UNDEFINED){
							cv::Point3d diff = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker1->currentIndex()]->getPoints3D()[i] - Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker2->currentIndex()]->getPoints3D()[i];
							outfile << cv::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z) << std::endl;
						}
						else
						{
							outfile << "NaN" << std::endl;
						}
					}
					outfile.close();
				}
				return true;
			}
		}
	}
	
	if (target == dock->plotWidget)
	{
		if (event->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *_mouseEvent = static_cast<QMouseEvent*>(event);
			if (_mouseEvent->buttons() == Qt::RightButton)
			{
				
			}
			else if (_mouseEvent->buttons() == Qt::LeftButton)
			{
				if (_mouseEvent->modifiers().testFlag(Qt::ShiftModifier)){
					double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
						? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
					int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
						? 0 : 1;

					double x = dock->plotWidget->xAxis->pixelToCoord(_mouseEvent->pos().x());
					int frame = (x - posOffset) / posMultiplier + 0.5;;

					startFrame = frame;
					endFrame = frame;

					if (frame  >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() -1 &&
						frame <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1){
						State::getInstance()->changeActiveFrameTrial(frame);
					}
				}
				else
				{
					double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
						? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
					int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
						? 0 : 1;

					double x = dock->plotWidget->xAxis->pixelToCoord(_mouseEvent->pos().x());
					int frame = (x - posOffset) / posMultiplier + 0.5;;
					if (frame >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1 &&
						frame <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1){
						State::getInstance()->changeActiveFrameTrial(frame);
					}
				}
			}
		}
		if (event->type() == QEvent::MouseMove)
		{
			QMouseEvent *_mouseEvent = static_cast<QMouseEvent*>(event);
			if (_mouseEvent->buttons() == Qt::LeftButton)
			{
				if (_mouseEvent->modifiers().testFlag(Qt::ShiftModifier)){
					double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
						? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
					int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
						? 0 : 1;

					

					double x = dock->plotWidget->xAxis->pixelToCoord(_mouseEvent->pos().x());
					int frame = (x - posOffset) / posMultiplier + 0.5;;
					endFrame = frame; 
					
					if (frame >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1 &&
						frame <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1){
						State::getInstance()->changeActiveFrameTrial(frame);
					}
				}
				else
				{
					double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
						? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
					int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
						? 0 : 1;

					double x = dock->plotWidget->xAxis->pixelToCoord(_mouseEvent->pos().x());
					int frame = (x - posOffset) / posMultiplier + 0.5;;
					if (frame >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1 &&
						frame <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1){
						State::getInstance()->changeActiveFrameTrial(frame);
					}
				}
			}
		}
	}

	return false;
}

void PlotWindow::resetRange()
{
	if (State::getInstance()->getActiveTrial() >= 0 && State::getInstance()->getActiveTrial() < Project::getInstance()->getTrials().size()){
		double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
		int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			? 0 : 1;

		dock->plotWidget->xAxis->setRange(
			(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1) * posMultiplier + posOffset,
			(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1) * posMultiplier + posOffset);
		if (this->isVisible())dock->plotWidget->replot();
	}
}

void PlotWindow::activeTrialChanged(int activeTrial){
	if (State::getInstance()->getWorkspace() == DIGITIZATION){
		if (activeTrial >= 0){
			updateMarkers(false);
			if (dock->comboBoxPlotType->currentIndex() == 4)
			{
				if (!updating && State::getInstance()->getActiveTrial() >= 0) {
					activeRigidBodyChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx());
				}
			}
			else{
				if (!updating && State::getInstance()->getActiveTrial() >= 0) {
					activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
				}
			}
		}
		updateTimeCheckBox();
	}
}

void PlotWindow::draw(){
	if (State::getInstance()->getWorkspace() == DIGITIZATION && this-isVisible()){
		if (dock->comboBoxPlotType->currentIndex() == 0)
		{
			plot2D(dock->comboBoxMarker1->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 1)
		{
			plot3D(dock->comboBoxMarker1->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 2){
			plotDistance(dock->comboBoxMarker1->currentIndex(), dock->comboBoxMarker2->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 3)
		{
			plotReprojectionError(dock->comboBoxMarker1->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 4){
			plotRigidBody(dock->comboBoxRigidBody->currentIndex());
		}
	}
}

void PlotWindow::workspaceChanged(work_state workspace){
	if (workspace == DIGITIZATION){
		updateMarkers(false);

		if (dock->comboBoxPlotType->currentIndex() == 4)
		{
			if (!updating && State::getInstance()->getActiveTrial() >= 0) {
				activeRigidBodyChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx());
			}
		}
		else{
			if (!updating && State::getInstance()->getActiveTrial() >= 0) {
				activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
			}
		}
		updateTimeCheckBox();
	}
}

void PlotWindow::updateTimeCheckBox()
{
	if (!updating && State::getInstance()->getActiveTrial() >= 0)
	{
		if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
		{
			dock->checkBoxTime->show();
		}
		else
		{
			dock->checkBoxTime->hide();
		}
	}
}

void PlotWindow::activePointChanged(int idx)
{
	if (idx >= 0){
		if (dock->comboBoxPlotType->currentIndex() == 0){
			updating = true;
			dock->comboBoxMarker1->setCurrentIndex(idx);
			updating = false;
			plot2D(dock->comboBoxMarker1->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 1){
			updating = true;
			dock->comboBoxMarker1->setCurrentIndex(idx);
			updating = false;
			plot3D(dock->comboBoxMarker1->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 2)
		{
			updating = true;
			dock->comboBoxMarker1->setCurrentIndex(idx);
			updating = false;
			plotDistance(dock->comboBoxMarker1->currentIndex(), dock->comboBoxMarker2->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 3)
		{
			updating = true;
			dock->comboBoxMarker1->setCurrentIndex(idx);
			updating = false;
			plotReprojectionError(dock->comboBoxMarker1->currentIndex());
		}
	}
}

void PlotWindow::activeRigidBodyChanged(int idx)
{
	if (idx >= 0){
		if (dock->comboBoxPlotType->currentIndex() == 4)
		{
			updating = true;
			dock->comboBoxRigidBody->setCurrentIndex(idx);
			updating = false;
			plotRigidBody(dock->comboBoxRigidBody->currentIndex());
		}
	}
}

void PlotWindow::updateMarkers(bool rememberSelection){
	if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0){
		int idx1 = dock->comboBoxMarker1->currentIndex();
		int idx2 = dock->comboBoxMarker2->currentIndex();
		int idxCam = dock->comboBoxCamera->currentIndex();
		int idxrb = dock->comboBoxRigidBody->currentIndex();

		dock->comboBoxMarker1->clear();
		dock->comboBoxMarker2->clear();
		updating = true;
		for (int i = 0; i < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); i++){
			dock->comboBoxMarker1->addItem(QString::number(i + 1));
			dock->comboBoxMarker2->addItem(QString::number(i + 1));
		}

		dock->comboBoxCamera->clear();
		dock->comboBoxCamera->addItem("All Cameras");
		for (int i = 0; i < Project::getInstance()->getCameras().size(); i++){
			dock->comboBoxCamera->addItem(Project::getInstance()->getCameras()[i]->getName());
		}

		
		dock->comboBoxRigidBody->clear();
		updating = true;
		for (int i = 0; i < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size(); i++){
			dock->comboBoxRigidBody->addItem("RB" + QString::number(i + 1) + " - " + Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[i]->getDescription());
		}

		if (idx1 >= 0 && idx1 < dock->comboBoxMarker1->count()) dock->comboBoxMarker1->setCurrentIndex(idx1);
		if (idx2 >= 0 && idx2 < dock->comboBoxMarker2->count()) dock->comboBoxMarker2->setCurrentIndex(idx1);
		if (idxCam >= 0 && idxCam < dock->comboBoxCamera->count()) dock->comboBoxCamera->setCurrentIndex(idxCam);
		if (idxrb >= 0 && idxrb < dock->comboBoxRigidBody->count()) dock->comboBoxRigidBody->setCurrentIndex(idxrb);

		updating = false;
	}
}

void PlotWindow::plot2D(int idx1)
{
	if (this->isVisible()){
		double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
		int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			? 0 : 1;

		dock->plotWidget->clearGraphs();

		double max_val_x = 0;
		double min_val_x = 10000;
		double max_val_y = 0;
		double min_val_y = 10000;

		if (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0){
			dock->plotWidget->xAxis->setLabel("Time in seconds");
		}
		else
		{
			dock->plotWidget->xAxis->setLabel("Frame");
		}
		dock->plotWidget->yAxis->setLabel("X - Position");
		dock->plotWidget->yAxis2->setVisible(true);
		dock->plotWidget->yAxis2->setLabel("Y - Position");

		if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx1 >= 0 && idx1 < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size()){
			
			for (int cam = 0; cam < Project::getInstance()->getCameras().size(); cam++){

				if (dock->comboBoxCamera->currentIndex() != 0) cam = dock->comboBoxCamera->currentIndex() - 1;
				
				dock->plotWidget->addGraph();
				dock->plotWidget->addGraph();

				QVector<double>	pos, x, y;
				
				for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
				{
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus2D()[cam][i] > UNDEFINED){
						x.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints2D()[cam][i].x);
						y.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints2D()[cam][i].y);
						pos.push_back(i * posMultiplier + posOffset);
						if (x[pos.size() - 1] > max_val_x) max_val_x = x[pos.size() - 1];
						if (x[pos.size() - 1] < min_val_x) min_val_x = x[pos.size() - 1];
						if (y[pos.size() - 1] > max_val_y) max_val_y = y[pos.size() - 1];
						if (y[pos.size() - 1] < min_val_y) min_val_y = y[pos.size() - 1];
					}
				}

				QColor color;

				switch (cam)
				{
				default:
				case 0:
					color = Qt::red;
					break;
				case 1:
					color = Qt::blue;
					break;
				case 2:
					color = Qt::green;
					break;
				case 3:
					color = Qt::yellow;
					break;
				case 4:
					color = Qt::cyan;
					break;
				}

				if (dock->comboBoxCamera->currentIndex() != 0) cam = 0;
				
				dock->plotWidget->graph(2 * cam + 0)->setData(pos, x);
				dock->plotWidget->graph(2 * cam + 0)->setLineStyle(QCPGraph::lsNone);
				dock->plotWidget->graph(2 * cam + 0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, 2));
				dock->plotWidget->graph(2 * cam + 0)->setPen(QPen(color));

				dock->plotWidget->graph(2 * cam + 1)->setData(pos, y);
				dock->plotWidget->graph(2 * cam + 1)->setLineStyle(QCPGraph::lsNone);
				dock->plotWidget->graph(2 * cam + 1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 2));
				dock->plotWidget->graph(2 * cam + 1)->setPen(QPen(color));
				dock->plotWidget->graph(2 * cam + 1)->setValueAxis(dock->plotWidget->yAxis2);

				if (dock->comboBoxCamera->currentIndex() != 0) cam = Project::getInstance()->getCameras().size();
			}

			double range = 0.2;
			double center = (max_val_x + min_val_x) * 0.5;
			range = (max_val_x - min_val_x) > range ? (max_val_x - min_val_x) : range;
			dock->plotWidget->yAxis->setRange(center - range * 0.5, center + range * 0.5);

			selectionMarker->topLeft->setCoords(startFrame * posMultiplier + posOffset, center + range * 0.5);
			selectionMarker->bottomRight->setCoords(endFrame * posMultiplier + posOffset, center - range * 0.5);
			
			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center - range * 0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center + range * 0.5);

			center = (max_val_y + min_val_y) * 0.5;
			range = (max_val_y - min_val_y) > range ? (max_val_y - min_val_y) : range;
			dock->plotWidget->yAxis2->setRange(center - range * 0.5, center + range * 0.5);
			
		}
		dock->plotWidget->replot();
		dock->plotWidget->show();
	}
}

void PlotWindow::plot3D(int idx1)
{
	if (this->isVisible()){
		double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
		int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			? 0 : 1;

		dock->plotWidget->clearGraphs();

		double max_val = 0;
		double min_val = 10000;
		
		if (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0){
			dock->plotWidget->xAxis->setLabel("Time in seconds");
		}
		else
		{
			dock->plotWidget->xAxis->setLabel("Frame");
		}
		dock->plotWidget->yAxis->setLabel("Position");
		dock->plotWidget->yAxis2->setVisible(false);

		if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx1 >= 0 && idx1 < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size()){

			dock->plotWidget->addGraph();
			dock->plotWidget->addGraph();
			dock->plotWidget->addGraph();

			QVector<double>	pos, x, y,z;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED){
					x.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints3D()[i].x);
					y.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints3D()[i].y);
					z.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints3D()[i].z);

					pos.push_back(i * posMultiplier + posOffset);
				
					if (x[pos.size() - 1] > max_val) max_val= x[pos.size() - 1];
					if (x[pos.size() - 1] < min_val) min_val = x[pos.size() - 1];
					if (y[pos.size() - 1] > max_val) max_val = y[pos.size() - 1];
					if (y[pos.size() - 1] < min_val) min_val = y[pos.size() - 1];
					if (z[pos.size() - 1] > max_val) max_val = z[pos.size() - 1];
					if (z[pos.size() - 1] < min_val) min_val = z[pos.size() - 1];
				}
			}


			dock->plotWidget->graph(0)->setData(pos, x);
			dock->plotWidget->graph(0)->setLineStyle(QCPGraph::lsNone);
			dock->plotWidget->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, 2));
			dock->plotWidget->graph(0)->setPen(QPen(QColor(Qt::red)));

			dock->plotWidget->graph(1)->setData(pos, y);
			dock->plotWidget->graph(1)->setLineStyle(QCPGraph::lsNone);
			dock->plotWidget->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 2));
			dock->plotWidget->graph(1)->setPen(QPen(QColor(Qt::green)));

			dock->plotWidget->graph(2)->setData(pos, z);
			dock->plotWidget->graph(2)->setLineStyle(QCPGraph::lsNone);
			dock->plotWidget->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 2));
			dock->plotWidget->graph(2)->setPen(QPen(QColor(Qt::blue)));

			double range = 0.2;
			double center = (max_val + min_val) * 0.5;
			range = (max_val - min_val) > range ? (max_val - min_val) : range;
			dock->plotWidget->yAxis->setRange(center - range * 0.5, center + range * 0.5);

			selectionMarker->topLeft->setCoords(startFrame * posMultiplier + posOffset, center + range * 0.5);
			selectionMarker->bottomRight->setCoords(endFrame * posMultiplier + posOffset, center - range * 0.5);

			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center - range * 0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center + range * 0.5);
		}
		dock->plotWidget->replot();
		dock->plotWidget->show();
	}
}

void PlotWindow::plotRigidBody(int idx)
{
	if (this->isVisible()){
		dock->plotWidget->clearGraphs();
		if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx >= 0 && idx < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size()){

			double cutoff = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getOverrideCutoffFrequency() ? Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getCutoffFrequency() :
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getCutoffFrequency();

			double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
				? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
			int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
				? 0 : 1;

			double max_val_trans = 0;
			double min_val_trans = 10000;

			double max_val_rot = 0;
			double min_val_rot = 10000;

		
			if (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0){
				if (dock->comboBoxRigidBodyTransType->currentIndex() > 0)
				{
					dock->plotWidget->xAxis->setLabel("Time in seconds - Cutoff Frequency " + QString::number(cutoff) + "Hz");
				}
				else
				{
					dock->plotWidget->xAxis->setLabel("Time in seconds");
				}
			}
			else
			{
				if (dock->comboBoxRigidBodyTransType->currentIndex() > 0)
				{
					dock->plotWidget->xAxis->setLabel("Frame - Cutoff Frequency "+ QString::number(cutoff)  +"Hz");
				}
				else
				{
					dock->plotWidget->xAxis->setLabel("Frame");
				}
			}

			if (dock->comboBoxRigidBodyTransPart->currentIndex() == 0)
			{
				//All
				dock->plotWidget->yAxis2->setVisible(true);
				dock->plotWidget->yAxis->setLabel("Translation");
				dock->plotWidget->yAxis2->setLabel("Rotationangle");
			}
			else if (dock->comboBoxRigidBodyTransPart->currentIndex() == 1 ||
				dock->comboBoxRigidBodyTransPart->currentIndex() == 3 || 
				dock->comboBoxRigidBodyTransPart->currentIndex() == 4 || 
				dock->comboBoxRigidBodyTransPart->currentIndex() == 5)
			{
				//Angles
				dock->plotWidget->yAxis2->setVisible(false);
				dock->plotWidget->yAxis->setLabel("Rotationangle");

			}
			else if (dock->comboBoxRigidBodyTransPart->currentIndex() == 2 ||
				dock->comboBoxRigidBodyTransPart->currentIndex() == 6 ||
				dock->comboBoxRigidBodyTransPart->currentIndex() == 7 ||
				dock->comboBoxRigidBodyTransPart->currentIndex() == 8)
			{
				//Translation
				dock->plotWidget->yAxis2->setVisible(false);
				dock->plotWidget->yAxis->setLabel("Translation");
			}

		
		
			int nbGraphs = 0;
			if (dock->comboBoxRigidBodyTransPart->currentIndex() == 0)
			{
				nbGraphs = 6;
			}
			else if (dock->comboBoxRigidBodyTransPart->currentIndex() == 1 ||
				dock->comboBoxRigidBodyTransPart->currentIndex() == 2 )
			{
				nbGraphs = 3; 
			}
			else
			{
				nbGraphs = 1;
			}

			if (dock->comboBoxRigidBodyTransType->currentIndex() == 2) nbGraphs *= 2;

			for (int i = 0; i < nbGraphs; i++){
				dock->plotWidget->addGraph();
			}

			QVector<double>	pos;
			QVector<double>	pos_filtered;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getPoseComputed()[i] > 0)
					pos.push_back(i * posMultiplier + posOffset);

				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getPoseFiltered()[i] > 0)
					pos_filtered.push_back(i * posMultiplier + posOffset);
			}

			int currentPlot = 0; 
			//Angles
			for (int z = 0; z < 3; z++)
			{
				if (dock->comboBoxRigidBodyTransPart->currentIndex() == 0 || dock->comboBoxRigidBodyTransPart->currentIndex() == 1
					|| (dock->comboBoxRigidBodyTransPart->currentIndex() == 3 && z == 0)
					|| (dock->comboBoxRigidBodyTransPart->currentIndex() == 4 && z == 1)
					|| (dock->comboBoxRigidBodyTransPart->currentIndex() == 5 && z == 2))
				{
					for (int y = 0; y < 2; y++)
					{
						bool filtered = (y == 0) ? true : false;
						if ((dock->comboBoxRigidBodyTransType->currentIndex() == 0 && !filtered) ||
							(dock->comboBoxRigidBodyTransType->currentIndex() == 1 && filtered) ||
							(dock->comboBoxRigidBodyTransType->currentIndex() == 2))
						{
							QVector<double> val;
							for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
							{
								if ((!filtered && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getPoseComputed()[i] > 0) ||
									(filtered && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getPoseFiltered()[i] > 0)){
									val.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getRotationEulerAngle(filtered, i, z));
								}
							}

							for (int i = 0; i < val.size(); i++)
							{
								if (i > 0 && fabs(val[i - 1] - val[i]) > 270)
								{
									if (val[i] < val[i - 1]) {
										val[i] = 360 + val[i];
									}
									else
									{
										val[i] = val[i] - 360;
									}
								}

								if (val[i] > max_val_rot) max_val_rot = val[i];
								if (val[i] < min_val_rot) min_val_rot = val[i];
							}
							
							if (filtered) {
								dock->plotWidget->graph(currentPlot)->setData(pos_filtered, val);
							}
							else
							{
								dock->plotWidget->graph(currentPlot)->setData(pos, val);
							}
							dock->plotWidget->graph(currentPlot)->setLineStyle(QCPGraph::lsNone);

							if (filtered){
								dock->plotWidget->graph(currentPlot)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, 2));
							}
							else
							{
								dock->plotWidget->graph(currentPlot)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 2));
							}
							dock->plotWidget->graph(currentPlot)->setPen(QPen(Qt::GlobalColor(currentPlot + 7)));
							currentPlot++;
						}
						
					}
				}
			}
			//Trans
			for (int z = 0; z < 3; z++)
			{
				if (dock->comboBoxRigidBodyTransPart->currentIndex() == 0 || dock->comboBoxRigidBodyTransPart->currentIndex() == 2
					|| (dock->comboBoxRigidBodyTransPart->currentIndex() == 6 && z == 0)
					|| (dock->comboBoxRigidBodyTransPart->currentIndex() == 7 && z == 1)
					|| (dock->comboBoxRigidBodyTransPart->currentIndex() == 8 && z == 2))
				{
					for (int y = 0; y < 2; y++)
					{
						bool filtered = (y == 0) ? true : false;
						if ((dock->comboBoxRigidBodyTransType->currentIndex() == 0 && !filtered) ||
							(dock->comboBoxRigidBodyTransType->currentIndex() == 1 && filtered) ||
							(dock->comboBoxRigidBodyTransType->currentIndex() == 2))
						{
							QVector<double> val;
							for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
							{
								if ((!filtered && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getPoseComputed()[i] > 0)||
									(filtered && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getPoseFiltered()[i] > 0)){
									val.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getTranslationVector(filtered)[i][z]);
									if (val.last() > max_val_trans) max_val_trans = val.last();
									if (val.last() < min_val_trans) min_val_trans = val.last();
								}
							}
							
							if (filtered) {
								dock->plotWidget->graph(currentPlot)->setData(pos_filtered, val);
							}
							else
							{
								dock->plotWidget->graph(currentPlot)->setData(pos, val);
							}
							dock->plotWidget->graph(currentPlot)->setLineStyle(QCPGraph::lsNone);

							if (filtered){
								dock->plotWidget->graph(currentPlot)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, 2));
								
							}
							else
							{
								dock->plotWidget->graph(currentPlot)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 2));
							}
							dock->plotWidget->graph(currentPlot)->setPen(QPen(Qt::GlobalColor(currentPlot + 7)));
							if (dock->comboBoxRigidBodyTransPart->currentIndex() == 0) dock->plotWidget->graph(currentPlot)->setValueAxis(dock->plotWidget->yAxis2);
							currentPlot++;
						}
					}
				}
			}
			double range = 0.00000000001;
			double center;
			if (dock->comboBoxRigidBodyTransPart->currentIndex() == 0)
			{
				//All
				center = (max_val_trans + min_val_trans) * 0.5;
				range = (max_val_trans - min_val_trans) > range ? (max_val_trans - min_val_trans) : range;
				dock->plotWidget->yAxis2->setRange(center - range * 0.55, center + range * 0.55);

				center = (max_val_rot + min_val_rot) * 0.5;
				range = (max_val_rot - min_val_rot) > range ? (max_val_rot - min_val_rot) : range;
				dock->plotWidget->yAxis->setRange(center - range * 0.55, center + range * 0.55);
			}
			else if (dock->comboBoxRigidBodyTransPart->currentIndex() == 1 ||
				dock->comboBoxRigidBodyTransPart->currentIndex() == 3 ||
				dock->comboBoxRigidBodyTransPart->currentIndex() == 4 ||
				dock->comboBoxRigidBodyTransPart->currentIndex() == 5)
			{
				//Translation
				center = (max_val_rot + min_val_rot) * 0.5;
				range = (max_val_rot - min_val_rot) > range ? (max_val_rot - min_val_rot) : range;
				dock->plotWidget->yAxis->setRange(center - range * 0.55, center + range * 0.55);
			}
			else if (dock->comboBoxRigidBodyTransPart->currentIndex() == 2 ||
				dock->comboBoxRigidBodyTransPart->currentIndex() == 6 ||
				dock->comboBoxRigidBodyTransPart->currentIndex() == 7 ||
				dock->comboBoxRigidBodyTransPart->currentIndex() == 8)
			{
				//Translation
				center = (max_val_trans + min_val_trans) * 0.5;
				range = (max_val_trans - min_val_trans) > range ? (max_val_trans - min_val_trans) : range;
				dock->plotWidget->yAxis->setRange(center - range * 0.55, center + range * 0.55);
			}

			selectionMarker->topLeft->setCoords(startFrame * posMultiplier + posOffset, center + range * 0.55);
			selectionMarker->bottomRight->setCoords(endFrame * posMultiplier + posOffset, center - range * 0.55);
			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center - range * 0.55);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center + range * 0.55);
		}
		dock->plotWidget->replot();
		dock->plotWidget->show();
	}
}

void PlotWindow::plotDistance(int idx1, int idx2){
	if (this->isVisible()){
		double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
		int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			? 0 : 1;

		dock->plotWidget->clearGraphs();
		
		dock->plotWidget->yAxis2->setVisible(false);

		double sd = 0;
		double mean = 0;

		if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
				idx1 >= 0 && idx1 < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() &&
				idx2 >= 0 && idx2 < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size()){
			
			dock->plotWidget->addGraph();

			
			QVector<double>
				x(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() + 1),
				y(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() + 1); // initialize with entries 0..100
			double max_val = 0;
			double min_val = 10000;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++, count++)
			{

				x[count] = i * posMultiplier + posOffset; // x goes from -1 to 1
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getStatus3D()[i] > UNDEFINED){
					cv::Point3d diff = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints3D()[i] - Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getPoints3D()[i];
					y[count] = cv::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
					if (y[count] > max_val) max_val = y[count];
					if (y[count] < min_val) min_val = y[count];

				}
				else{
					y[count] = 0; // let's plot a quadratic function
				}
			}
			int countVisible = 0;
			
			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++, count++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getStatus3D()[i] > UNDEFINED){
					mean += y[count];
					countVisible++;
				}
			}
			if (countVisible > 0)mean = mean / countVisible;
			
			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++, count++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getStatus3D()[i] > UNDEFINED){
					sd += pow(y[count] - mean,2);
				}
			}

			if (countVisible > 1)sd = sqrt(sd / (countVisible - 1));

			// create graph and assign data to it:
			dock->plotWidget->graph(0)->setData(x, y);
			dock->plotWidget->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
			dock->plotWidget->graph(0)->setPen(QPen(QColor(Qt::blue))); 

			dock->plotWidget->addGraph();
			QVector<double>x2,y2;
			x2.push_back((Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1)* posMultiplier + posOffset);
			x2.push_back((Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1)* posMultiplier + posOffset);
			y2.push_back(mean);
			y2.push_back(mean);

			QPen dotPen;
			dotPen.setColor(Qt::darkGray);
			dotPen.setStyle(Qt::DashLine);
			dock->plotWidget->graph(1)->setData(x2, y2);
			dock->plotWidget->graph(1)->setLineStyle(QCPGraph::lsLine);
			dock->plotWidget->graph(1)->setPen(dotPen);

			// set axes ranges, so we see all data:
			double range = 0.2;
			double center = (max_val + min_val) * 0.5;
			range = (max_val - min_val) > range ? (max_val - min_val) : range;

			dock->plotWidget->yAxis->setRange(center - range * 0.5, center + range * 0.5);

			selectionMarker->topLeft->setCoords(startFrame * posMultiplier + posOffset, center + range * 0.5);
			selectionMarker->bottomRight->setCoords(endFrame * posMultiplier + posOffset, center - range * 0.5);
			
			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center - range * 0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center + range * 0.5);
		}
		if (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0){
			dock->plotWidget->xAxis->setLabel("Time in seconds - " + QString::number(mean) + " +/- " + QString::number(sd));
		}
		else
		{
			dock->plotWidget->xAxis->setLabel("Frame - " + QString::number(mean) + " +/- " + QString::number(sd));
		}
		
		dock->plotWidget->yAxis->setLabel("Distance Marker " + QString::number(idx1 + 1) + " to Marker " + QString::number(idx2 + 1));

		dock->plotWidget->replot();
		dock->plotWidget->show();
	}
}

void PlotWindow::plotReprojectionError(int idx1)
{
	if (this->isVisible()){
		double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
		int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			? 0 : 1;

		dock->plotWidget->clearGraphs();

		double max_val = 0;
		double min_val = 10000;

		double sd = 0;
		double mean = 0;

		dock->plotWidget->yAxis->setLabel("Error in pixel");
		dock->plotWidget->yAxis2->setVisible(false);

		if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx1 >= 0 && idx1 < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size()){

			dock->plotWidget->addGraph();
			QVector<double>	pos, error;
			
			if (dock->comboBoxCamera->currentIndex() == 0)
			{
				int count;
				bool set;
				double error_val;
				for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
				{
					set = false;
					error_val = 0;
					count = 0;
					for (int cam = 0; cam < Project::getInstance()->getCameras().size(); cam++){
						if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus2D()[cam][i] > UNDEFINED &&
							Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED)
						{
							count++;
							error_val += Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getError2D()[cam][i];
							set = true;
						}
					}

					if (set)
					{
						error.push_back(error_val/count);
						pos.push_back(i * posMultiplier + posOffset);
						if (error[pos.size() - 1] > max_val) max_val = error[pos.size() - 1];
						if (error[pos.size() - 1] < min_val) min_val = error[pos.size() - 1];
					}
					else
					{
						error.push_back(-2);
						pos.push_back(i * posMultiplier + posOffset);
					}
				}
			}
			else
			{
				int cam = dock->comboBoxCamera->currentIndex() - 1;
				for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
				{
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus2D()[cam][i] > UNDEFINED &&
						Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED)
					{
						error.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getError2D()[cam][i]);
						pos.push_back(i * posMultiplier + posOffset);
						if (error[pos.size() - 1] > max_val) max_val = error[pos.size() - 1];
						if (error[pos.size() - 1] < min_val) min_val = error[pos.size() - 1];
					}
					else
					{
						error.push_back(-2);
						pos.push_back(i * posMultiplier + posOffset);
					}
				}
			}
			int countVisible = 0;
			for (int i = 0; i < error.size(); i++)
			{
				if (error[i] >= 0){
					mean += error[i];
					countVisible++;
				}
			}
			if (countVisible > 0)mean = mean / countVisible;

			for (int i = 0; i < error.size(); i++)
			{
				if (error[i] >= 0){
					sd += pow(error[i] - mean,2);
				}
			}

			if (countVisible > 1)sd = sqrt(sd / (countVisible - 1));

			dock->plotWidget->graph(0)->setData(pos, error);
			dock->plotWidget->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
			dock->plotWidget->graph(0)->setPen(QPen(QColor(Qt::blue)));

			double range = 0.2;
			range = (max_val) > range ? (max_val * 1.1) : range;

			dock->plotWidget->yAxis->setRange(-0.5, range);
			
			selectionMarker->topLeft->setCoords(startFrame * posMultiplier + posOffset, range);
			selectionMarker->bottomRight->setCoords(endFrame * posMultiplier + posOffset, -0.5);
			
			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, -0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, range);
		}

		if (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0){
			dock->plotWidget->xAxis->setLabel("Time in seconds - " + QString::number(mean) + " +/- " + QString::number(sd));
		}
		else
		{
			dock->plotWidget->xAxis->setLabel("Frame - " + QString::number(mean) + " +/- " + QString::number(sd));
		}

		

		dock->plotWidget->replot();
		dock->plotWidget->show();
	}
}

void PlotWindow::on_comboBoxCamera_currentIndexChanged(int idx)
{
	if (!updating){
		if (dock->comboBoxPlotType->currentIndex() == 0)
		{
			plot2D(dock->comboBoxMarker1->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 3)
		{
			plotReprojectionError(dock->comboBoxMarker1->currentIndex());
		}
	}
}

void PlotWindow::on_comboBoxPlotType_currentIndexChanged(int idx)
{
	if (idx == 0)
	{
		dock->labelCamera->show();
		dock->comboBoxCamera->show();

		dock->labelMarker1->show();
		dock->comboBoxMarker1->show();

		dock->labelMarker2->hide();
		dock->comboBoxMarker2->hide();

		dock->frameMarkerPlot->show();
		dock->frameRigidBodyPlot->hide(); 

		if (!updating && State::getInstance()->getActiveTrial() >= 0) {
			activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		}
	}
	else if (idx == 1){
		dock->labelCamera->hide();
		dock->comboBoxCamera->hide();

		dock->labelMarker1->show();
		dock->comboBoxMarker1->show();

		dock->labelMarker2->hide();
		dock->comboBoxMarker2->hide();

		dock->frameMarkerPlot->show();
		dock->frameRigidBodyPlot->hide();

		if (!updating && State::getInstance()->getActiveTrial() >= 0) {
			activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		}
	}
	else if (idx == 2){
		dock->labelCamera->hide();
		dock->comboBoxCamera->hide();

		dock->labelMarker1->show();
		dock->comboBoxMarker1->show();

		dock->labelMarker2->show();
		dock->comboBoxMarker2->show();

		dock->frameMarkerPlot->show();
		dock->frameRigidBodyPlot->hide();

		if (!updating && State::getInstance()->getActiveTrial() >= 0) {
			activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		}
	}
	else if (idx == 3)
	{
		dock->labelCamera->show();
		dock->comboBoxCamera->show();

		dock->labelMarker1->show();
		dock->comboBoxMarker1->show();

		dock->labelMarker2->hide();
		dock->comboBoxMarker2->hide();

		dock->frameMarkerPlot->show();
		dock->frameRigidBodyPlot->hide();

		if (!updating && State::getInstance()->getActiveTrial() >= 0) {
			activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		}
	}
	else if (idx == 4)
	{
		dock->frameMarkerPlot->hide();
		dock->frameRigidBodyPlot->show();

		on_pushButtonUpdate_clicked();
	}
}

void PlotWindow::on_comboBoxMarker1_currentIndexChanged(int idx){
	if (!updating){
		if (dock->comboBoxPlotType->currentIndex() == 0)
		{
			PointsDockWidget::getInstance()->selectPoint(idx + 1);
			plot2D(dock->comboBoxMarker1->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 1)
		{
			PointsDockWidget::getInstance()->selectPoint(idx + 1);
			plot3D(dock->comboBoxMarker1->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 2){
			PointsDockWidget::getInstance()->selectPoint(idx + 1);
			plotDistance(dock->comboBoxMarker1->currentIndex(), dock->comboBoxMarker2->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 3)
		{
			PointsDockWidget::getInstance()->selectPoint(idx + 1);
			plotReprojectionError(dock->comboBoxMarker1->currentIndex());
		}
	}
}

void PlotWindow::on_comboBoxMarker2_currentIndexChanged(int idx){
	if (dock->comboBoxPlotType->currentIndex() == 2){
		if (!updating)plotDistance(dock->comboBoxMarker1->currentIndex(), dock->comboBoxMarker2->currentIndex());
	}
}

void PlotWindow::on_pushButton_Reset_clicked()
{
	resetRange();
}

void PlotWindow::closeEvent(QCloseEvent *event)
{
	event->ignore();
	MainWindow::getInstance()->on_actionPlot_triggered(false);
}

void PlotWindow::on_pushButtonUpdate_clicked()
{
	if(State::getInstance()->getActiveTrial() >= 0) {
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->recomputeAndFilterRigidBodyTransformations();
	}
	plotRigidBody(dock->comboBoxRigidBody->currentIndex());
}

void PlotWindow::on_comboBoxRigidBody_currentIndexChanged(int idx)
{
	if (!updating){
		PointsDockWidget::getInstance()->selectBody(idx + 1);
		plotRigidBody(dock->comboBoxRigidBody->currentIndex());
	}
}

void PlotWindow::on_comboBoxRigidBodyTransPart_currentIndexChanged(int idx)
{
	plotRigidBody(dock->comboBoxRigidBody->currentIndex());
}

void PlotWindow::on_comboBoxRigidBodyTransType_currentIndexChanged(int idx)
{
	plotRigidBody(dock->comboBoxRigidBody->currentIndex());
}

void PlotWindow::on_checkBoxTime_clicked()
{
	resetRange();
	draw();
}