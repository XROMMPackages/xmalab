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
#include "core/Trial.h"
#include "core/Project.h"
#include "core/Marker.h"

#include "ui/State.h"
#include "ui/MainWindow.h"

using namespace xma;

PlotWindow* PlotWindow::instance = NULL;

PlotWindow::PlotWindow(QWidget *parent):QDockWidget(parent)
{
    setWindowTitle(tr("Plot"));
	
	plotWidget = new QCustomPlot(this);
	plotWidget->addGraph();
	frameMarker = new QCPItemLine(plotWidget);
	plotWidget->addItem(frameMarker);
	frameMarker->setSelectable(true);
	plotWidget->installEventFilter(this);

	setFeatures(QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetMovable);
	setAllowedAreas(Qt::AllDockWidgetAreas);
	setMinimumSize(0,0);
	setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
	this->resize(500,500);

	frame = new QFrame(this);
	frame->setObjectName(QString::fromUtf8("frame"));
    frame->setMinimumSize(QSize(0, 0));
    frame->setMaximumSize(QSize(16777215, 16777215));
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Raised);

    layout = new QGridLayout(frame);
	layout->addWidget(plotWidget, 0, 0, 1, 2);

	comboBoxMarker1 = new QComboBox(this);
    comboBoxMarker1->setObjectName(QString::fromUtf8("comboBoxMarker1"));
    comboBoxMarker1->setMinimumSize(QSize(0, 0));
    comboBoxMarker1->setMaximumSize(QSize(16777215, 16777215));
	layout->addWidget(comboBoxMarker1, 1, 0, 1 , 1);

	comboBoxMarker2 = new QComboBox(this);
    comboBoxMarker2->setObjectName(QString::fromUtf8("comboBoxMarker2"));
    comboBoxMarker2->setMinimumSize(QSize(0, 0));
    comboBoxMarker2->setMaximumSize(QSize(16777215, 16777215));
	layout->addWidget(comboBoxMarker2, 1, 1, 1 , 1);
	setWidget(frame);
	updating = false;

	connect(State::getInstance(), SIGNAL(activeTrialChanged(int)), this, SLOT(activeTrialChanged(int)));
	connect(State::getInstance(), SIGNAL(activeFrameTrialChanged(int)), this, SLOT(activeFrameTrialChanged(int)));
	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));

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

bool PlotWindow::eventFilter(QObject *target, QEvent *event)
{
	if (target == plotWidget){
		if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *_mouseEvent = static_cast<QMouseEvent*>(event);
			if (_mouseEvent->buttons() == Qt::LeftButton){
				int x = plotWidget->xAxis->pixelToCoord(_mouseEvent->pos().x());
				State::getInstance()->changeActiveFrameTrial(x - 1);
			}
		}
	}
	return false;
}

void PlotWindow::activeTrialChanged(int activeTrial){
	if (State::getInstance()->getWorkspace() == DIGITIZATION){
		updateMarkers(false);
	}
}

void PlotWindow::activeFrameTrialChanged(int activeFrame){
	if (State::getInstance()->getWorkspace() == DIGITIZATION){
		if(this->isVisible())
			plot(comboBoxMarker1->currentIndex(), comboBoxMarker2->currentIndex());
	}
}


void PlotWindow::workspaceChanged(work_state workspace){
	if (workspace == DIGITIZATION){
		updateMarkers(false);
	}
}

void PlotWindow::updateMarkers(bool rememberSelection){
	if (Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0){
		int idx1 = comboBoxMarker1->currentIndex();
		int idx2 = comboBoxMarker2->currentIndex();

		comboBoxMarker1->clear();
		comboBoxMarker2->clear();
		updating = true;
		for (int i = 0; i < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); i++){
			comboBoxMarker1->addItem(QString::number(i + 1));
			comboBoxMarker2->addItem(QString::number(i + 1));
		}

		if (rememberSelection)
		{
			if (idx1 >= 0 && idx1 < comboBoxMarker1->count()) comboBoxMarker1->setCurrentIndex(idx1);
			if (idx2 >= 0 && idx2 < comboBoxMarker2->count()) comboBoxMarker2->setCurrentIndex(idx1);
		}

		updating = false;
	}
}



void PlotWindow::plot(int idx1, int idx2){
	if (this->isVisible()){
		if (idx1 >= 0 && idx1 < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() &&
			idx2 >= 0 && idx2 < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size()){
			QVector<double>
				x(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() + 1),
				y(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() + 1); // initialize with entries 0..100
			double max_val = 0;
			double min_val = 10000;


			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame(), count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame(); i++, count++)
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
			double mean = 0;
			int countVisible = 0;
			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame(), count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame(); i++, count++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getStatus3D()[i] > UNDEFINED){
					mean += y[count];
					countVisible++;
				}
			}
			if (countVisible > 0)mean = mean / countVisible;
			double sd = 0;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame(), count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame(); i++, count++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getStatus3D()[i] > UNDEFINED){
					sd += fabs(y[count] - mean);
				}
			}

			if (countVisible > 0)sd = sd / countVisible;

			// create graph and assign data to it:
			plotWidget->graph(0)->setData(x, y);
			// give the axes some labels:
			plotWidget->xAxis->setLabel("Frame - " + QString::number(mean) + " +/- " + QString::number(sd));
			plotWidget->yAxis->setLabel("Distance Marker " + QString::number(idx1 + 1) + " to Marker " + QString::number(idx2 + 1));
			// set axes ranges, so we see all data:
			double range = 0.2;
			double center = (max_val + min_val) * 0.5;
			range = (max_val - min_val) > range ? (max_val - min_val) : range;

			plotWidget->xAxis->setRange(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() + 1, Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() + 1);
			plotWidget->yAxis->setRange(center - range * 0.5, center + range * 0.5);
			
			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() + 1, center - range * 0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() + 1, center + range * 0.5);

			plotWidget->replot();
			plotWidget->show();
		}
	}
}


void PlotWindow::on_comboBoxMarker1_currentIndexChanged(int idx){
	if(!updating)plot(comboBoxMarker1->currentIndex(), comboBoxMarker2->currentIndex());
}

void PlotWindow::on_comboBoxMarker2_currentIndexChanged(int idx){
	if (!updating)plot(comboBoxMarker1->currentIndex(), comboBoxMarker2->currentIndex());
}