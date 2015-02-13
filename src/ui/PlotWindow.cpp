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
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

#include "core/Trial.h"
#include "core/Project.h"
#include "core/Marker.h"
#include "core/Camera.h"

#include "ui/State.h"
#include "ui/MainWindow.h"
#include "ui/ConfirmationDialog.h"

using namespace xma;

PlotWindow* PlotWindow::instance = NULL;

PlotWindow::PlotWindow(QWidget *parent):QDockWidget(parent)
{
    setWindowTitle(tr("Plot"));
	
	//setup Plot 1
	plotWidget = new QCustomPlot(this);
	frameMarker = new QCPItemLine(plotWidget);
	plotWidget->addItem(frameMarker);
	plotWidget->installEventFilter(this);
	selectionMarker = new QCPItemRect(plotWidget);
	plotWidget->addItem(selectionMarker);
	selectionMarker->setPen(QPen(QColor(255, 255, 0, 50)));
	selectionMarker->setBrush(QBrush(QColor(255,255,0,150)));
	startFrame = 0;
	endFrame = 0;

	plotWidget->setFocusPolicy(Qt::StrongFocus);
	plotWidget->setInteraction(QCP::iRangeDrag, true);
	plotWidget->axisRect()->setRangeDrag(Qt::Horizontal);
	plotWidget->setInteraction(QCP::iRangeZoom, true);
	plotWidget->axisRect()->setRangeZoom(Qt::Horizontal);

	//Setup DockWidget
	setFeatures(QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetMovable);
	setAllowedAreas(Qt::AllDockWidgetAreas);
	setMinimumSize(0,0);
	setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
	this->resize(500,500);

	//create ui-objects
	frame = new QFrame(this);
	frame->setObjectName(QString::fromUtf8("frame"));
    frame->setMinimumSize(QSize(0, 0));
    frame->setMaximumSize(QSize(16777215, 16777215));
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Raised);

	QSizePolicy sizePolicy1(QSizePolicy::Maximum, QSizePolicy::Preferred);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);

	labelPlotType = new QLabel(this);
	labelPlotType->setObjectName(QString::fromUtf8("labelPlotType"));
	labelPlotType->setMinimumSize(QSize(0, 0));
	labelPlotType->setMaximumSize(QSize(16777215, 16777215));
	labelPlotType->setText("Plot type : ");
	sizePolicy1.setHeightForWidth(labelPlotType->sizePolicy().hasHeightForWidth());
	labelPlotType->setSizePolicy(sizePolicy1);

	comboBoxPlotType = new QComboBox(this);
	comboBoxPlotType->setObjectName(QString::fromUtf8("comboBoxPlotType"));
	comboBoxPlotType->setMinimumSize(QSize(0, 0));
	comboBoxPlotType->setMaximumSize(QSize(16777215, 16777215));
	comboBoxPlotType->addItem("2D positions");
	comboBoxPlotType->addItem("3D positions");
	comboBoxPlotType->addItem("Marker to Marker distance");
	comboBoxPlotType->addItem("Backprojection Error");

	labelCamera = new QLabel(this);
	labelCamera->setObjectName(QString::fromUtf8("labelCamera"));
	labelCamera->setMinimumSize(QSize(0, 0));
	labelCamera->setMaximumSize(QSize(16777215, 16777215));
	labelCamera->setText("Camera : ");
	sizePolicy1.setHeightForWidth(labelCamera->sizePolicy().hasHeightForWidth());
	labelCamera->setSizePolicy(sizePolicy1);

	comboBoxCamera = new QComboBox(this);
	comboBoxCamera->setObjectName(QString::fromUtf8("comboBoxCamera"));
	comboBoxCamera->setMinimumSize(QSize(0, 0));
	comboBoxCamera->setMaximumSize(QSize(16777215, 16777215));

	labelMarker1 = new QLabel(this);
	labelMarker1->setObjectName(QString::fromUtf8("labelMarker1"));
	labelMarker1->setMinimumSize(QSize(0, 0));
	labelMarker1->setMaximumSize(QSize(16777215, 16777215));
	labelMarker1->setText("Marker : ");
	sizePolicy1.setHeightForWidth(labelMarker1->sizePolicy().hasHeightForWidth());
	labelMarker1->setSizePolicy(sizePolicy1);

	comboBoxMarker1 = new QComboBox(this);
	comboBoxMarker1->setObjectName(QString::fromUtf8("comboBoxMarker1"));
	comboBoxMarker1->setMinimumSize(QSize(0, 0));
	comboBoxMarker1->setMaximumSize(QSize(16777215, 16777215));

	labelMarker2 = new QLabel(this);
	labelMarker2->setObjectName(QString::fromUtf8("labelMarker2"));
	labelMarker2->setMinimumSize(QSize(0, 0));
	labelMarker2->setMaximumSize(QSize(16777215, 16777215));
	labelMarker2->setText("Marker : ");
	sizePolicy1.setHeightForWidth(labelMarker2->sizePolicy().hasHeightForWidth());
	labelMarker2->setSizePolicy(sizePolicy1);

	comboBoxMarker2 = new QComboBox(this);
	comboBoxMarker2->setObjectName(QString::fromUtf8("comboBoxMarker2"));
	comboBoxMarker2->setMinimumSize(QSize(0, 0));
	comboBoxMarker2->setMaximumSize(QSize(16777215, 16777215));

	pushButton_Reset = new QPushButton(this);
	pushButton_Reset->setObjectName(QString::fromUtf8("pushButton_Reset"));
	pushButton_Reset->setMinimumSize(QSize(0, 0));
	pushButton_Reset->setMaximumSize(QSize(16777215, 16777215));
	pushButton_Reset->setText("Fit plot");

	//add ui-objects to layout
	layout = new QGridLayout(frame);
	layout->addWidget(plotWidget,		0, 0, 1, 9);
	layout->addWidget(labelPlotType,	1, 0, 1, 1);
	layout->addWidget(comboBoxPlotType, 1, 1, 1, 1);
	layout->addWidget(labelCamera,		1, 2, 1, 1);
	layout->addWidget(comboBoxCamera,	1, 3, 1, 1);
	layout->addWidget(labelMarker1,		1, 4, 1, 1);
	layout->addWidget(comboBoxMarker1,	1, 5, 1, 1);
	layout->addWidget(labelMarker2,		1, 6, 1, 1);
	layout->addWidget(comboBoxMarker2,	1, 7, 1, 1);
	layout->addWidget(pushButton_Reset, 1, 8, 1, 1);

	comboBoxPlotType->setCurrentIndex(0);
	on_comboBoxPlotType_currentIndexChanged(0);
	plot2D(-1);

	setWidget(frame);
	updating = false;

	connect(State::getInstance(), SIGNAL(activeTrialChanged(int)), this, SLOT(activeTrialChanged(int)));
	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));
	connect(PointsDockWidget::getInstance(), SIGNAL(activePointChanged(int)), this, SLOT(activePointChanged(int)));

	QMetaObject::connectSlotsByName(this);
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
	int cam;
	int frameStart;
	int frameEnd;
	frameStart = (endFrame > startFrame) ? startFrame : endFrame;
	frameEnd = (endFrame > startFrame) ? endFrame : startFrame;
	QString cameras;
	if (comboBoxPlotType->currentIndex() == 0 || comboBoxPlotType->currentIndex() == 3)
	{
		cam = comboBoxCamera->currentIndex() - 1;
		cameras = comboBoxCamera->currentText();
	}
	else if (comboBoxPlotType->currentIndex() == 1 || comboBoxPlotType->currentIndex() == 2)
	{
		cam = -1;
		cameras = "All Cameras";
	}
	
	if (comboBoxPlotType->currentIndex() == 0 || comboBoxPlotType->currentIndex() == 1 || comboBoxPlotType->currentIndex() == 3)
	{	
		if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to delete your data for " + cameras + " for Marker " + comboBoxMarker1->currentText() + " from Frame " +
			QString::number(frameStart + 1) + " to " + QString::number(frameEnd + 1))) return;
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[comboBoxMarker1->currentIndex()]->resetMultipleFrames(cam,frameStart, frameEnd);
	}
	else
	{
		if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to delete your data for " + cameras + " for Marker " + comboBoxMarker1->currentText() + " and Marker " + comboBoxMarker2->currentText() + " from Frame " +
			QString::number(frameStart + 1) + " to " + QString::number(frameEnd + 1))) return;
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[comboBoxMarker1->currentIndex()]->resetMultipleFrames(cam, frameStart, frameEnd);
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[comboBoxMarker2->currentIndex()]->resetMultipleFrames(cam, frameStart, frameEnd);
	}
	MainWindow::getInstance()->redrawGL();
}

bool PlotWindow::eventFilter(QObject *target, QEvent *event)
{
	if (target == plotWidget)
	{
		if (event->type() == QEvent::KeyPress)
		{
			QKeyEvent *_keyEvent = static_cast<QKeyEvent*>(event);
			if (_keyEvent->key() == Qt::Key_Delete)
			{
				deleteData();
			}
		}

		if (event->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *_mouseEvent = static_cast<QMouseEvent*>(event);
			if (_mouseEvent->buttons() == Qt::RightButton)
			{
				
			}
			else if (_mouseEvent->buttons() == Qt::LeftButton)
			{
				if (_mouseEvent->modifiers().testFlag(Qt::ShiftModifier)){
					int x = plotWidget->xAxis->pixelToCoord(_mouseEvent->pos().x()) + 0.5;		
					startFrame = x - 1;
					endFrame = x - 1;
					State::getInstance()->changeActiveFrameTrial(x - 1);
				}
				else
				{
					int x = plotWidget->xAxis->pixelToCoord(_mouseEvent->pos().x()) + 0.5;
					State::getInstance()->changeActiveFrameTrial(x - 1);
				}
			}
		}
		if (event->type() == QEvent::MouseMove)
		{
			QMouseEvent *_mouseEvent = static_cast<QMouseEvent*>(event);
			if (_mouseEvent->buttons() == Qt::LeftButton)
			{
				if (_mouseEvent->modifiers().testFlag(Qt::ShiftModifier)){
					int x = plotWidget->xAxis->pixelToCoord(_mouseEvent->pos().x()) + 0.5;
					endFrame = x - 1;
					State::getInstance()->changeActiveFrameTrial(x - 1);			
				}
				else
				{
					int x = plotWidget->xAxis->pixelToCoord(_mouseEvent->pos().x()) + 0.5;
					State::getInstance()->changeActiveFrameTrial(x - 1);
				}
			}
		}
	}
	return false;
}



void PlotWindow::resetRange()
{
	plotWidget->xAxis->setRange(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() + 1, Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() + 1);
	plotWidget->replot();
}

void PlotWindow::activeTrialChanged(int activeTrial){
	if (State::getInstance()->getWorkspace() == DIGITIZATION){
		if (activeTrial >= 0){
			updateMarkers(false);

			if (!updating && State::getInstance()->getActiveTrial() >= 0) {
				activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
			}
		}
	}
}



void PlotWindow::draw(){
	if (State::getInstance()->getWorkspace() == DIGITIZATION && this-isVisible()){
		if (comboBoxPlotType->currentIndex() == 0)
		{
			plot2D(comboBoxMarker1->currentIndex());
		}
		else if (comboBoxPlotType->currentIndex() == 1)
		{
			plot3D(comboBoxMarker1->currentIndex());
		}
		else if (comboBoxPlotType->currentIndex() == 2){
			plotDistance(comboBoxMarker1->currentIndex(), comboBoxMarker2->currentIndex());
		}
		else if (comboBoxPlotType->currentIndex() == 3)
		{
			plotBackProjectionError(comboBoxMarker1->currentIndex());
		}
	}
}


void PlotWindow::workspaceChanged(work_state workspace){
	if (workspace == DIGITIZATION){
		updateMarkers(false);

		if (!updating && State::getInstance()->getActiveTrial() >= 0) {
			activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		}
	}
}

void PlotWindow::activePointChanged(int idx)
{
	if (comboBoxPlotType->currentIndex() == 0){
		updating = true;
		comboBoxMarker1->setCurrentIndex(idx);
		updating = false;
		plot2D(comboBoxMarker1->currentIndex());
	}else if (comboBoxPlotType->currentIndex() == 1){
		updating = true;
		comboBoxMarker1->setCurrentIndex(idx);
		updating = false;
		plot3D(comboBoxMarker1->currentIndex());
	}
	else if (comboBoxPlotType->currentIndex() == 3)
	{
		updating = true;
		comboBoxMarker1->setCurrentIndex(idx);
		updating = false;
		plotBackProjectionError(comboBoxMarker1->currentIndex());
	}
}

void PlotWindow::updateMarkers(bool rememberSelection){
	if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0){
		int idx1 = comboBoxMarker1->currentIndex();
		int idx2 = comboBoxMarker2->currentIndex();
		int idxCam = comboBoxCamera->currentIndex();

		comboBoxMarker1->clear();
		comboBoxMarker2->clear();
		updating = true;
		for (int i = 0; i < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); i++){
			comboBoxMarker1->addItem(QString::number(i + 1));
			comboBoxMarker2->addItem(QString::number(i + 1));
		}

		comboBoxCamera->clear();
		comboBoxCamera->addItem("All Cameras");
		for (int i = 0; i < Project::getInstance()->getCameras().size(); i++){
			comboBoxCamera->addItem(Project::getInstance()->getCameras()[i]->getName());
		}

		
		if (idx1 >= 0 && idx1 < comboBoxMarker1->count()) comboBoxMarker1->setCurrentIndex(idx1);
		if (idx2 >= 0 && idx2 < comboBoxMarker2->count()) comboBoxMarker2->setCurrentIndex(idx1);
		if (idxCam >= 0 && idxCam < comboBoxCamera->count()) comboBoxCamera->setCurrentIndex(idxCam);

		updating = false;
	}
}

void PlotWindow::plot2D(int idx1)
{
	if (this->isVisible()){
		plotWidget->clearGraphs();

		double max_val_x = 0;
		double min_val_x = 10000;
		double max_val_y = 0;
		double min_val_y = 10000;

		plotWidget->xAxis->setLabel("Frame");
		plotWidget->yAxis->setLabel("X - Position");
		plotWidget->yAxis2->setVisible(true);
		plotWidget->yAxis2->setLabel("Y - Position");

		if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx1 >= 0 && idx1 < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size()){
			
			for (int cam = 0; cam < Project::getInstance()->getCameras().size(); cam++){

				if (comboBoxCamera->currentIndex() != 0) cam = comboBoxCamera->currentIndex() - 1;
				
				plotWidget->addGraph();
				plotWidget->addGraph();

				QVector<double>	pos, x, y;

				for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
				{
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus2D()[cam][i] > UNDEFINED){
						x.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints2D()[cam][i].x);
						y.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints2D()[cam][i].y);
						pos.push_back(i+1);
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

				if (comboBoxCamera->currentIndex() != 0) cam = 0;
				
				plotWidget->graph(2 * cam + 0)->setData(pos, x);
				plotWidget->graph(2 * cam + 0)->setLineStyle(QCPGraph::lsNone);
				plotWidget->graph(2 * cam + 0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, 2));
				plotWidget->graph(2 * cam + 0)->setPen(QPen(color));

				plotWidget->graph(2 * cam + 1)->setData(pos, y);
				plotWidget->graph(2 * cam + 1)->setLineStyle(QCPGraph::lsNone);
				plotWidget->graph(2 * cam + 1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 2));
				plotWidget->graph(2 * cam + 1)->setPen(QPen(color));
				plotWidget->graph(2 * cam + 1)->setValueAxis(plotWidget->yAxis2);

				if (comboBoxCamera->currentIndex() != 0) cam = Project::getInstance()->getCameras().size();
			}

			double range = 0.2;
			double center = (max_val_x + min_val_x) * 0.5;
			range = (max_val_x - min_val_x) > range ? (max_val_x - min_val_x) : range;
			plotWidget->yAxis->setRange(center - range * 0.5, center + range * 0.5);

			selectionMarker->topLeft->setCoords(startFrame + 1, center + range * 0.5);
			selectionMarker->bottomRight->setCoords(endFrame + 1, center - range * 0.5);

			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() + 1, center - range * 0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() + 1, center + range * 0.5);

			center = (max_val_y + min_val_y) * 0.5;
			range = (max_val_y - min_val_y) > range ? (max_val_y - min_val_y) : range;
			plotWidget->yAxis2->setRange(center - range * 0.5, center + range * 0.5);
			
		}
		plotWidget->replot();
		plotWidget->show();
	}
}

void PlotWindow::plot3D(int idx1)
{
	if (this->isVisible()){
		plotWidget->clearGraphs();

		double max_val = 0;
		double min_val = 10000;
		
		plotWidget->xAxis->setLabel("Frame");
		plotWidget->yAxis->setLabel("Position");
		plotWidget->yAxis2->setVisible(false);

		if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx1 >= 0 && idx1 < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size()){

			plotWidget->addGraph();
			plotWidget->addGraph();
			plotWidget->addGraph();

			QVector<double>	pos, x, y,z;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED){
					x.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints3D()[i].x);
					y.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints3D()[i].y);
					z.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints3D()[i].z);
					pos.push_back(i+1);
					if (x[pos.size() - 1] > max_val) max_val= x[pos.size() - 1];
					if (x[pos.size() - 1] < min_val) min_val = x[pos.size() - 1];
					if (y[pos.size() - 1] > max_val) max_val = y[pos.size() - 1];
					if (y[pos.size() - 1] < min_val) min_val = y[pos.size() - 1];
					if (z[pos.size() - 1] > max_val) max_val = z[pos.size() - 1];
					if (z[pos.size() - 1] < min_val) min_val = z[pos.size() - 1];
				}
			}


			plotWidget->graph(0)->setData(pos, x);
			plotWidget->graph(0)->setLineStyle(QCPGraph::lsNone);
			plotWidget->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, 2));
			plotWidget->graph(0)->setPen(QPen(QColor(Qt::red)));

			plotWidget->graph(1)->setData(pos, y);
			plotWidget->graph(1)->setLineStyle(QCPGraph::lsNone);
			plotWidget->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 2));
			plotWidget->graph(1)->setPen(QPen(QColor(Qt::green)));

			plotWidget->graph(2)->setData(pos, z);
			plotWidget->graph(2)->setLineStyle(QCPGraph::lsNone);
			plotWidget->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 2));
			plotWidget->graph(2)->setPen(QPen(QColor(Qt::blue)));

			double range = 0.2;
			double center = (max_val + min_val) * 0.5;
			range = (max_val - min_val) > range ? (max_val - min_val) : range;
			plotWidget->yAxis->setRange(center - range * 0.5, center + range * 0.5);

			selectionMarker->topLeft->setCoords(startFrame + 1, center + range * 0.5);
			selectionMarker->bottomRight->setCoords(endFrame + 1, center - range * 0.5);

			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() + 1, center - range * 0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() + 1, center + range * 0.5);
		}
		plotWidget->replot();
		plotWidget->show();
	}
}

void PlotWindow::plotDistance(int idx1, int idx2){
	if (this->isVisible()){
		plotWidget->clearGraphs();
		
		plotWidget->yAxis2->setVisible(false);

		double sd = 0;
		double mean = 0;

		if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
				idx1 >= 0 && idx1 < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() &&
				idx2 >= 0 && idx2 < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size()){
			
			plotWidget->addGraph();

			
			QVector<double>
				x(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() + 1),
				y(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() + 1); // initialize with entries 0..100
			double max_val = 0;
			double min_val = 10000;

			
			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++, count++)
			{

				x[count] = i + 1; // x goes from -1 to 1
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
			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame(), count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame(); i++, count++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getStatus3D()[i] > UNDEFINED){
					mean += y[count];
					countVisible++;
				}
			}
			if (countVisible > 0)mean = mean / countVisible;
			
			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame(), count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame(); i++, count++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getStatus3D()[i] > UNDEFINED){
					sd += fabs(y[count] - mean);
				}
			}

			if (countVisible > 0)sd = sd / countVisible;

			// create graph and assign data to it:
			plotWidget->graph(0)->setData(x, y);
			plotWidget->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
			plotWidget->graph(0)->setPen(QPen(QColor(Qt::blue)));

			plotWidget->addGraph();
			QVector<double>x2,y2;
			x2.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame());
			x2.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame());
			y2.push_back(mean);
			y2.push_back(mean);

			QPen dotPen;
			dotPen.setColor(Qt::darkGray);
			dotPen.setStyle(Qt::DashLine);
			plotWidget->graph(1)->setData(x2, y2);
			plotWidget->graph(1)->setLineStyle(QCPGraph::lsLine);
			plotWidget->graph(1)->setPen(dotPen);

			// set axes ranges, so we see all data:
			double range = 0.2;
			double center = (max_val + min_val) * 0.5;
			range = (max_val - min_val) > range ? (max_val - min_val) : range;

			plotWidget->yAxis->setRange(center - range * 0.5, center + range * 0.5);

			selectionMarker->topLeft->setCoords(startFrame + 1, center + range * 0.5);
			selectionMarker->bottomRight->setCoords(endFrame + 1, center - range * 0.5);

			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() + 1, center - range * 0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() + 1, center + range * 0.5);
		}
		
		plotWidget->xAxis->setLabel("Frame - " + QString::number(mean) + " +/- " + QString::number(sd));
		plotWidget->yAxis->setLabel("Distance Marker " + QString::number(idx1 + 1) + " to Marker " + QString::number(idx2 + 1));

		plotWidget->replot();
		plotWidget->show();
	}
}

void PlotWindow::plotBackProjectionError(int idx1)
{

	if (this->isVisible()){
		plotWidget->clearGraphs();

		double max_val = 0;
		double min_val = 10000;

		double sd = 0;
		double mean = 0;

		plotWidget->yAxis->setLabel("Error in pixel");
		plotWidget->yAxis2->setVisible(false);

		if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx1 >= 0 && idx1 < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size()){

			plotWidget->addGraph();
			QVector<double>	pos, error;

			if (comboBoxCamera->currentIndex() == 0)
			{
				for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
				{
					bool set = false;
					double error_val = 0;
					for (int cam = 0; cam < Project::getInstance()->getCameras().size(); cam++){
						if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus2D()[cam][i] > UNDEFINED &&
							Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED)
						{
							error_val += Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getError2D()[cam][i];
							set = true;
						}
					}

					if (set)
					{
						error.push_back(error_val);
						pos.push_back(i + 1);
						if (error[pos.size() - 1] > max_val) max_val = error[pos.size() - 1];
						if (error[pos.size() - 1] < min_val) min_val = error[pos.size() - 1];
					}
					else
					{
						error.push_back(-2);
						pos.push_back(i + 1);
					}
				}
			}
			else
			{
				int cam = comboBoxCamera->currentIndex() - 1;
				for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
				{
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus2D()[cam][i] > UNDEFINED &&
						Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED)
					{
						error.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getError2D()[cam][i]);
						pos.push_back(i + 1);
						if (error[pos.size() - 1] > max_val) max_val = error[pos.size() - 1];
						if (error[pos.size() - 1] < min_val) min_val = error[pos.size() - 1];
					}
					else
					{
						error.push_back(-2);
						pos.push_back(i + 1);
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
					sd += fabs(error[i] - mean);
				}
			}

			if (countVisible > 0)sd = sd / countVisible;

			plotWidget->graph(0)->setData(pos, error);
			plotWidget->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
			plotWidget->graph(0)->setPen(QPen(QColor(Qt::blue)));

			double range = 0.2;
			range = (max_val) > range ? (max_val * 1.1) : range;

			plotWidget->yAxis->setRange(-0.5, range);

			selectionMarker->topLeft->setCoords(startFrame + 1, range);
			selectionMarker->bottomRight->setCoords(endFrame + 1, -0.5);

			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() + 1, -0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() + 1, range);
		}

		plotWidget->xAxis->setLabel("Frame - " + QString::number(mean) + " +/- " + QString::number(sd));

		plotWidget->replot();
		plotWidget->show();
	}
}

void PlotWindow::on_comboBoxCamera_currentIndexChanged(int idx)
{
	if (!updating){
		if (comboBoxPlotType->currentIndex() == 0)
		{
			plot2D(comboBoxMarker1->currentIndex());
		}
		else if (comboBoxPlotType->currentIndex() == 3)
		{
			plotBackProjectionError(comboBoxMarker1->currentIndex());
		}
	}
}

void PlotWindow::on_comboBoxPlotType_currentIndexChanged(int idx)
{
	if (idx == 0)
	{
		labelCamera->show();
		comboBoxCamera->show();

		labelMarker1->show();
		comboBoxMarker1->show();

		labelMarker2->hide();
		comboBoxMarker2->hide();

		if (!updating && State::getInstance()->getActiveTrial() >= 0) {
			activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		}
	}
	else if (idx == 1){
		labelCamera->hide();
		comboBoxCamera->hide();

		labelMarker1->show();
		comboBoxMarker1->show();

		labelMarker2->hide();
		comboBoxMarker2->hide();

		if (!updating && State::getInstance()->getActiveTrial() >= 0) {
			activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		}
	}
	else if (idx == 2){
		labelCamera->hide();
		comboBoxCamera->hide();

		labelMarker1->show();
		comboBoxMarker1->show();

		labelMarker2->show();
		comboBoxMarker2->show();
		if (!updating) plotDistance(comboBoxMarker1->currentIndex(), comboBoxMarker2->currentIndex());
	}
	else if (idx == 3)
	{
		labelCamera->show();
		comboBoxCamera->show();

		labelMarker1->show();
		comboBoxMarker1->show();

		labelMarker2->hide();
		comboBoxMarker2->hide();

		if (!updating && State::getInstance()->getActiveTrial() >= 0) {
			activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		}
	}
}

void PlotWindow::on_comboBoxMarker1_currentIndexChanged(int idx){
	if (!updating){
		if (comboBoxPlotType->currentIndex() == 0)
		{
			PointsDockWidget::getInstance()->selectPoint(idx + 1);
			plot2D(comboBoxMarker1->currentIndex());
		}
		else if (comboBoxPlotType->currentIndex() == 1)
		{
			PointsDockWidget::getInstance()->selectPoint(idx + 1);
			plot3D(comboBoxMarker1->currentIndex());
		}
		else if (comboBoxPlotType->currentIndex() == 2){
			plotDistance(comboBoxMarker1->currentIndex(), comboBoxMarker2->currentIndex());
		}
		else if (comboBoxPlotType->currentIndex() == 3)
		{
			PointsDockWidget::getInstance()->selectPoint(idx + 1);
			plotBackProjectionError(comboBoxMarker1->currentIndex());
		}
	}
}

void PlotWindow::on_comboBoxMarker2_currentIndexChanged(int idx){
	if (comboBoxPlotType->currentIndex() == 2){
		if (!updating)plotDistance(comboBoxMarker1->currentIndex(), comboBoxMarker2->currentIndex());
	}
}

void PlotWindow::on_pushButton_Reset_clicked()
{
	resetRange();
}