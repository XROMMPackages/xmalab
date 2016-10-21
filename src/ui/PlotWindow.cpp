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
///\file PlotWindow.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
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

PlotWindow::PlotWindow(QWidget* parent) : QDockWidget(parent), dock(new Ui::PlotWindow)
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
	selectionMarker->setBrush(QBrush(QColor(255, 255, 0, 150)));
	startFrame = 0;
	endFrame = 0;

	dock->plotWidget->setFocusPolicy(Qt::StrongFocus);
	dock->plotWidget->setInteraction(QCP::iRangeDrag, true);
	dock->plotWidget->setInteraction(QCP::iRangeZoom, true);
	dock->plotWidget->axisRect()->setRangeDrag(Qt::Horizontal);
	dock->plotWidget->axisRect()->setRangeZoom(Qt::Horizontal);

	//Setup DockWidget
	setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
	this->resize(500, 500);

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

void PlotWindow::saveData()
{
	QString text = "Save PlotData";
	QString fileName = QFileDialog::getSaveFileName(this,
		text, Settings::getInstance()->getLastUsedDirectory() + OS_SEP + "PlotData.csv", tr("Comma seperated data (*.csv)"));
	if (fileName.isNull() == false)
	{
		std::ofstream outfile(fileName.toAscii().data());
		outfile.precision(12);
		
		if (dock->comboBoxPlotType->currentIndex() == 0)
		{
			int cam_start = (dock->comboBoxCamera->currentIndex() == 0) ? 0 : dock->comboBoxCamera->currentIndex() - 1;
			int cam_end = (dock->comboBoxCamera->currentIndex() == 0) ? Project::getInstance()->getCameras().size() - 1 : dock->comboBoxCamera->currentIndex() - 1;
			int marker_id = dock->comboBoxMarker1->currentIndex();

			outfile << "Frame , ";

			//Header
			for (int c = cam_start; c <= cam_end; c++)
			{
				outfile << "Cam" + std::to_string(c + 1) + "_Marker_" + std::to_string(marker_id + 1) + "_x , Cam" + std::to_string(c+1) + "_Marker_" + std::to_string(marker_id + 1) + "_y" <<
					" , Cam" + std::to_string(c + 1) + "_Marker_" + std::to_string(marker_id + 1) + "_x_undistorted , Cam" + std::to_string(c + 1) + "_Marker_" + std::to_string(marker_id + 1) + "_y_undistorted";
				if (c != cam_end)
				{
					outfile << " , ";
				}
			}
			outfile << std::endl;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
			{
				outfile << i + 1 << " , ";
				for (int c = cam_start; c <= cam_end; c++)
				{
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[marker_id]->getStatus2D()[c][i] > UNDEFINED)
					{
						cv::Point2d undist = Project::getInstance()->getCameras()[c]->undistortPoint(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[marker_id]->getPoints2D()[c][i], true);
						outfile << Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[marker_id]->getPoints2D()[c][i].x << " , " <<
							Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[marker_id]->getPoints2D()[c][i].y << " , " << undist.x << " , " << undist.y;
					}
					else
					{
						outfile << "NaN , NaN , NaN, NaN";
					}
					if (c != cam_end)
					{
						outfile << " , ";
					}
				}
				outfile << std::endl;
			}
		}
		else if (dock->comboBoxPlotType->currentIndex() == 1)
		{
			int cam_start = (dock->comboBoxCamera->currentIndex() == 0) ? 0 : dock->comboBoxCamera->currentIndex() - 1;
			int cam_end = (dock->comboBoxCamera->currentIndex() == 0) ? Project::getInstance()->getCameras().size() - 1 : dock->comboBoxCamera->currentIndex() - 1;
			int marker_id = dock->comboBoxMarker1->currentIndex();
			
			//Header
			outfile << "Frame , ";
			outfile << "Marker_" + std::to_string(marker_id + 1) + "_x , Marker_" + std::to_string(marker_id + 1) + "_y , Marker_" + std::to_string(marker_id + 1) + "_z" << std::endl;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
			{

				outfile << i + 1 << " , ";
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[marker_id]->getStatus3D()[i] > UNDEFINED)
				{
					outfile << Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[marker_id]->getPoints3D()[i].x
						<< " , " << Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[marker_id]->getPoints3D()[i].y
						<< " , " << Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[marker_id]->getPoints3D()[i].z;
				} else
				{
					outfile << "NaN , NaN , NaN" << std::endl;
				}
				
				outfile << std::endl;
			}
		}
		else if (dock->comboBoxPlotType->currentIndex() == 2)
		{
			//Header
			outfile << "Frame , ";
			outfile << "Marker_" + std::to_string(dock->comboBoxMarker1->currentIndex() + 1) + "_to_Marker_" + std::to_string(dock->comboBoxMarker2->currentIndex() + 1) << std::endl;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++, count++)
			{
				outfile << i + 1 << " , ";
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker1->currentIndex()]->getStatus3D()[i] > UNDEFINED && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker2->currentIndex()]->getStatus3D()[i] > UNDEFINED)
				{
					cv::Point3d diff = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker1->currentIndex()]->getPoints3D()[i] - Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker2->currentIndex()]->getPoints3D()[i];
					outfile << cv::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z) << std::endl;
				}
				else
				{
					outfile << "NaN" << std::endl;
				}
			}
		}
		else if (dock->comboBoxPlotType->currentIndex() == 3)
		{
			int cam_start = (dock->comboBoxCamera->currentIndex() == 0) ? 0 : dock->comboBoxCamera->currentIndex() - 1;
			int cam_end = (dock->comboBoxCamera->currentIndex() == 0) ? Project::getInstance()->getCameras().size() - 1 : dock->comboBoxCamera->currentIndex() - 1;
			int marker_id = dock->comboBoxMarker1->currentIndex();
			
			//Header
			outfile << "Frame , ";
			outfile << "ReprojectionError_Marker_" + std::to_string(dock->comboBoxMarker1->currentIndex() + 1) << "_" << dock->comboBoxCamera->currentText().toAscii().data() << std::endl;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
			{
				bool set = false;
				double error_val = 0;
				int count = 0;
				for (int c = cam_start; c <= cam_end; c++)
				{
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[marker_id]->getStatus2D()[c][i] > UNDEFINED &&
						Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[marker_id]->getStatus3D()[i] > UNDEFINED)
					{
						count++;
						error_val += Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[marker_id]->getError2D()[c][i];
						set = true;
					}
				}

				outfile << i + 1 << " , ";
				if (set)
				{
					outfile << error_val / count;
				}
				else
				{
					outfile << "NaN" << std::endl;
				}
				outfile << std::endl;
			}
		}
		else if (dock->comboBoxPlotType->currentIndex() == 4)
		{
			int filtered = dock->comboBoxRigidBodyTransType->currentIndex();
			int body = dock->comboBoxRigidBody->currentIndex();
			int type = dock->comboBoxRigidBodyTransPart->currentIndex();

			QStringList data = QStringList();

			//Header
			outfile << "Frame , ";
			if (type == 0 || type == 1 || type == 3)
			{
				if (filtered != 1) data << "Unfiltered_Yaw_RB" + QString::number(body + 1);
				if (filtered != 0) data << "Filtered_Yaw_RB" + QString::number(body + 1);
			}
			if (type == 0 || type == 1 || type == 4)
			{
				if (filtered != 1) data << "Unfiltered_Pitch_RB" + QString::number(body + 1);
				if (filtered != 0) data << "Filtered_Pitch_RB" + QString::number(body + 1);
			}
			if (type == 0 || type == 1 || type == 5)
			{
				if (filtered != 1) data << "Unfiltered_Roll_RB" + QString::number(body + 1);
				if (filtered != 0) data << "Filtered_Roll_RB" + QString::number(body + 1);
			}
			if (type == 0 || type == 2 || type == 6)
			{
				if (filtered != 1) data << "Unfiltered_X_RB" + QString::number(body + 1);
				if (filtered != 0) data << "Filtered_X_RB" + QString::number(body + 1);
			}
			if (type == 0 || type == 2 || type == 7)
			{
				if (filtered != 1) data << "Unfiltered_Y_RB" + QString::number(body + 1);
				if (filtered != 0) data << "Filtered_Y_RB" + QString::number(body + 1);
			}
			if (type == 0 || type == 2 || type == 8)
			{
				if (filtered != 1) data << "Unfiltered_Z_RB" + QString::number(body + 1);
				if (filtered != 0) data << "Filtered_Z_RB" + QString::number(body + 1);
			}
			//write Header

			for (int i = 0; i < data.size(); i++)
			{
				outfile << data[i].toAscii().data();

				if (i != data.size() - 1)
				{
					outfile << " , ";
				}
			}
			outfile << std::endl;

			std::vector<std::vector<double>> data_vec;
			double min_value = std::numeric_limits<double>::min();
			//Set Rotations
			for (int z = 0; z < 3; z++)
			{
				for (int k = 0; k < 2; k++){
					std::vector<double> val;
					bool filt = k == 1;
					for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
					{
						if ((!filt && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[body]->getPoseComputed()[i] > 0) ||
							(filt && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[body]->getPoseFiltered()[i] > 0))
						{
							val.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[body]->getRotationEulerAngle(filt, i, z));
						} else
						{
							val.push_back(min_value);
						}
					}

					double lastval = val[0];
					for (int i = 1; i < val.size(); i++)
					{
						if (lastval != min_value && fabs(lastval - val[i]) > 270)
						{
							if (val[i] < lastval)
							{
								val[i] = 360 + val[i];
							}
							else
							{
								val[i] = val[i] - 360;
							}
						}

						if (val[i] != min_value) lastval = val[i];

					}
					data_vec.push_back(val);
				}
			}
			//Set Translations
			for (int z = 0; z < 3; z++)
			{
				for (int k = 0; k < 2; k++){
					std::vector<double> val;
					bool filt = k == 1;
					for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
					{
						if ((!filt && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[body]->getPoseComputed()[i] > 0) ||
							(filt && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[body]->getPoseFiltered()[i] > 0))
						{
							val.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[body]->getTranslationVector(filt)[i][z]);
						} 
						else
						{
							val.push_back(min_value);
						}
					}
					data_vec.push_back(val);
				}
			}


			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
			{
				data.clear();
				if (type == 0 || type == 1 || type == 3)
				{
					if (filtered != 1) data << ((data_vec[0][i] != min_value) ? QString::number(data_vec[0][i],'f', 12) : QString("NaN"));
					if (filtered != 0) data << ((data_vec[1][i] != min_value) ? QString::number(data_vec[1][i], 'f', 12) : QString("NaN"));
				}
				if (type == 0 || type == 1 || type == 4)
				{
					if (filtered != 1) data << ((data_vec[2][i] != min_value) ? QString::number(data_vec[2][i], 'f', 12) : QString("NaN"));
					if (filtered != 0) data << ((data_vec[3][i] != min_value) ? QString::number(data_vec[3][i], 'f', 12) : QString("NaN"));
				}
				if (type == 0 || type == 1 || type == 5)
				{
					if (filtered != 1) data << ((data_vec[4][i] != min_value) ? QString::number(data_vec[4][i], 'f', 12) : QString("NaN"));
					if (filtered != 0) data << ((data_vec[5][i] != min_value) ? QString::number(data_vec[5][i], 'f', 12) : QString("NaN"));
				}
				if (type == 0 || type == 2 || type == 6)
				{
					if (filtered != 1) data << ((data_vec[6][i] != min_value) ? QString::number(data_vec[6][i], 'f', 12) : QString("NaN"));
					if (filtered != 0) data << ((data_vec[7][i] != min_value) ? QString::number(data_vec[7][i], 'f', 12) : QString("NaN"));
				}
				if (type == 0 || type == 2 || type == 7)
				{
					if (filtered != 1) data << ((data_vec[8][i] != min_value) ? QString::number(data_vec[8][i], 'f', 12) : QString("NaN"));
					if (filtered != 0) data << ((data_vec[9][i] != min_value) ? QString::number(data_vec[9][i], 'f', 12) : QString("NaN"));
				}
				if (type == 0 || type == 2 || type == 8)
				{
					if (filtered != 1) data << ((data_vec[10][i] != min_value) ? QString::number(data_vec[10][i], 'f', 12) : QString("NaN"));
					if (filtered != 0) data << ((data_vec[11][i] != min_value) ? QString::number(data_vec[11][i], 'f', 12) : QString("NaN"));
				}

				//write DataOut
				outfile << i + 1 << " , ";
				for (int i = 0; i < data.size(); i++)
				{
					outfile << data[i].toAscii().data();

					if (i != data.size() - 1)
					{
						outfile << " , ";
					}
				}
				outfile << std::endl;
			}

		}
		else if (dock->comboBoxPlotType->currentIndex() == 5)
		{
			int filtered = dock->comboBoxRigidBodyTransType->currentIndex();
			int bodyIdx = dock->comboBoxRigidBody->currentIndex();
			bool draw3D = dock->comboBoxRigidBodyError->currentIndex() == 0;

			RigidBody * body = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[bodyIdx];
			
			QStringList data = QStringList();

			//header
			outfile << "Frame , ";
			if (filtered != 1){
				data << "Unfiltered_RigidBodyError" + QString(draw3D ? "3D" : "2D") + "_RB" + QString::number(bodyIdx + 1) + "_Mean";
				data << "Unfiltered_RigidBodyError" + QString(draw3D ? "3D" : "2D") + "_RB" + QString::number(bodyIdx + 1) + "_SD";
			}
			if (filtered != 0){
				data << "Filtered_RigidBodyError" + QString(draw3D ? "3D" : "2D") + "_RB" + QString::number(bodyIdx + 1) + "_Mean";
				data << "Filtered_RigidBodyError" + QString(draw3D ? "3D" : "2D") + "_RB" + QString::number(bodyIdx + 1) + "_SD";
			}
			for (int i = 0; i < data.size(); i++)
			{
				outfile << data[i].toAscii().data();

				if (i != data.size() - 1)
				{
					outfile << " , ";
				}
			}
			outfile << std::endl;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
			{
				data.clear();

				if (filtered != 1){
					if (body->getPoseComputed()[i])
					{
						if (draw3D)
						{
							data << QString::number(body->getErrorMean3D()[i], 'f', 12);
							data << QString::number(body->getErrorSd3D()[i], 'f', 12);
						}
						else
						{
							data << QString::number(body->getErrorMean2D()[i], 'f', 12);
							data << QString::number(body->getErrorSd2D()[i], 'f', 12);
						}
					} else
					{
						data << "NaN";
						data << "NaN";
					}
				}
				if (filtered != 0){
					if (body->getPoseFiltered()[i])
					{
						if (draw3D)
						{
							data << QString::number(body->getErrorMean3D_filtered()[i], 'f', 12);
							data << QString::number(body->getErrorSd3D_filtered()[i], 'f', 12);
						}
						else
						{
							data << QString::number(body->getErrorMean2D_filtered()[i], 'f', 12);
							data << QString::number(body->getErrorSd2D_filtered()[i], 'f', 12);
						}
					}
					else
					{
						data << "NaN";
						data << "NaN";
					}
				}

				outfile << i + 1 << " , ";
				for (int i = 0; i < data.size(); i++)
				{
					outfile << data[i].toAscii().data();

					if (i != data.size() - 1)
					{
						outfile << " , ";
					}
				}
				outfile << std::endl;
			}
		}
	outfile.close();
	}
}

PlotWindow::~PlotWindow()
{
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
	frameStart = (frameStart < Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getStartFrame() - 1) ? Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getStartFrame() - 1 : frameStart;
	frameEnd = (frameEnd > Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getEndFrame() - 1) ? Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getEndFrame() - 1 : frameEnd;

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
			QString::number(frameStart + 1) + " to " + QString::number(frameEnd + 1)))
			return;
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker1->currentIndex()]->resetMultipleFrames(cam, frameStart, frameEnd);
	}
	else if (dock->comboBoxPlotType->currentIndex() == 2)
	{
		if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to delete your data for " + cameras + " for Marker " + dock->comboBoxMarker1->currentText() + " and Marker " + dock->comboBoxMarker2->currentText() + " from Frame " +
			QString::number(frameStart + 1) + " to " + QString::number(frameEnd + 1)))
			return;
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker1->currentIndex()]->resetMultipleFrames(cam, frameStart, frameEnd);
		Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[dock->comboBoxMarker2->currentIndex()]->resetMultipleFrames(cam, frameStart, frameEnd);
	}
	else if (dock->comboBoxPlotType->currentIndex() == 4 || dock->comboBoxPlotType->currentIndex() == 5)
	{
		if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to delete your data for " + cameras + " for all marker of Rigid Body " + dock->comboBoxRigidBody->currentText() + " from Frame " + QString::number(frameStart + 1) + " to " + QString::number(frameEnd + 1))) return;
		for (unsigned int i = 0; i < Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[dock->comboBoxRigidBody->currentIndex()]->getPointsIdx().size(); i++)
		{
			int idx = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRigidBodies()[dock->comboBoxRigidBody->currentIndex()]->getPointsIdx()[i];
			Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getMarkers()[idx]->resetMultipleFrames(cam, frameStart, frameEnd);
		}

		on_pushButtonUpdate_clicked();
	}
	MainWindow::getInstance()->redrawGL();
}

void PlotWindow::drawStatus(int idx)
{
	if (dock->checkBoxStatus->isChecked()){
		QBrush brush_Interpolated = QBrush(QColor(Settings::getInstance()->getQStringSetting("ColorInterpolated")));
		QPen pen_Interpolated = QPen(QColor(Settings::getInstance()->getQStringSetting("ColorInterpolated")));

		QBrush brush_Manual = QBrush(QColor(Settings::getInstance()->getQStringSetting("ColorManual")));
		QPen pen_Manual = QPen(QColor(Settings::getInstance()->getQStringSetting("ColorManual")));

		QBrush brush_ManualAndOpt = QBrush(QColor(Settings::getInstance()->getQStringSetting("ColorManualAndOpt")));
		QPen pen_ManualAndOpt = QPen(QColor(Settings::getInstance()->getQStringSetting("ColorManualAndOpt")));

		QBrush brush_Set = QBrush(QColor(Settings::getInstance()->getQStringSetting("ColorSet")));
		QPen pen_Set = QPen(QColor(Settings::getInstance()->getQStringSetting("ColorSet")));

		QBrush brush_SetAndOpt = QBrush(QColor(Settings::getInstance()->getQStringSetting("ColorSetAndOpt")));
		QPen pen_SetAndOpt = QPen(QColor(Settings::getInstance()->getQStringSetting("ColorSetAndOpt")));

		QBrush brush_Tracked = QBrush(QColor(Settings::getInstance()->getQStringSetting("ColorTracked")));
		QPen pen_Tracked = QPen(QColor(Settings::getInstance()->getQStringSetting("ColorTracked")));

		QBrush brush_TrackedAndOpt = QBrush(QColor(Settings::getInstance()->getQStringSetting("ColorTrackedAndOpt")));
		QPen pen_TrackedAndOpt = QPen(QColor(Settings::getInstance()->getQStringSetting("ColorTrackedAndOpt")));

		QBrush brush_Undefined = QBrush(QColor(Settings::getInstance()->getQStringSetting("ColorUndefined")));
		QPen pen_Undefined = QPen(QColor(Settings::getInstance()->getQStringSetting("ColorUndefined")));

		double y_max = dock->plotWidget->yAxis->range().upper;
		double height = dock->plotWidget->yAxis->range().size() / (1.0 + (0.05 * (Project::getInstance()->getCameras().size() + 1))) * 0.05;

		double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
		double posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			? 1 : 0;

		Marker * marker = NULL;
		if (idx >= 0 && idx < (int) Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size())
		{
			marker = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx];
		}

		for (unsigned int c = 0; c < Project::getInstance()->getCameras().size(); c++)
		{
			int count = 0;
			for (int f = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame(); f <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame(); f++)
			{
				if (marker)
				{
					marker_status[c][count]->setVisible(true);
					switch (marker->getStatus2D()[c][f - 1])
					{
						case UNDEFINED:
							marker_status[c][count]->setBrush(brush_Undefined);
							marker_status[c][count]->setPen(pen_Undefined);
							break;
						case INTERPOLATED:
							marker_status[c][count]->setBrush(brush_Interpolated);
							marker_status[c][count]->setPen(pen_Interpolated);
							break;
						case TRACKED:
							marker_status[c][count]->setBrush(brush_Tracked);
							marker_status[c][count]->setPen(pen_Tracked);
							break;
						case TRACKED_AND_OPTIMIZED:
							marker_status[c][count]->setBrush(brush_TrackedAndOpt);
							marker_status[c][count]->setPen(pen_TrackedAndOpt);
							break;
						case SET:
							marker_status[c][count]->setBrush(brush_Set);
							marker_status[c][count]->setPen(pen_Set);
							break;
						case SET_AND_OPTIMIZED:
							marker_status[c][count]->setBrush(brush_SetAndOpt);
							marker_status[c][count]->setPen(pen_SetAndOpt);
							break;
						case MANUAL:
							marker_status[c][count]->setBrush(brush_Manual);
							marker_status[c][count]->setPen(pen_Manual);
							break;
						case MANUAL_AND_OPTIMIZED:
							marker_status[c][count]->setBrush(brush_ManualAndOpt);
							marker_status[c][count]->setPen(pen_ManualAndOpt);
							break;
                        default:
                            marker_status[c][count]->setBrush(brush_Undefined);
							marker_status[c][count]->setPen(pen_Undefined);
                            break;
					}

					marker_status[c][count]->topLeft->setCoords((((double) f) - 0.5 - posOffset) * posMultiplier, y_max - c*height);
					marker_status[c][count]->bottomRight->setCoords((((double) f) + 0.5 - posOffset) * posMultiplier, y_max - (c + 1)*height);
				}
				else
				{
					marker_status[c][count]->setVisible(false);
				}
				count++;
			}
		}
	}
}

bool PlotWindow::eventFilter(QObject* target, QEvent* event)
{
	if (event->type() == QEvent::KeyPress)
	{
		if (Shortcuts::getInstance()->checkShortcut(target, event))
		{
			return true;
		}

		QKeyEvent* _keyEvent = static_cast<QKeyEvent*>(event);
		if (_keyEvent->key() == Qt::Key_Delete || _keyEvent->key() == Qt::Key_Backspace)
		{
			deleteData();
			return true;
		}
	}

	if (target == dock->plotWidget)
	{
		if (event->type() == QEvent::Wheel)
		{
			QWheelEvent* _wheelEvent = static_cast<QWheelEvent*>(event);
			if (_wheelEvent->modifiers().testFlag(Qt::NoModifier))
			{
				dock->plotWidget->axisRect()->setRangeZoom(Qt::Horizontal);
			}
			else if (_wheelEvent->modifiers().testFlag(Qt::ShiftModifier))
			{
				dock->plotWidget->axisRect()->setRangeZoom(Qt::Vertical);
				dock->plotWidget->axisRect()->setRangeZoomAxes(dock->plotWidget->xAxis, dock->plotWidget->yAxis);
			}
			else if (_wheelEvent->modifiers().testFlag(Qt::ControlModifier))
			{
				dock->plotWidget->axisRect()->setRangeZoom(Qt::Vertical);
				dock->plotWidget->axisRect()->setRangeZoomAxes(dock->plotWidget->xAxis, dock->plotWidget->yAxis2);
			}
		}

		if (event->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent* _mouseEvent = static_cast<QMouseEvent*>(event);
			if (_mouseEvent->buttons() == Qt::RightButton)
			{
				if (_mouseEvent->modifiers().testFlag(Qt::NoModifier))
				{
					dock->plotWidget->axisRect()->setRangeDrag(Qt::Horizontal);
					QMouseEvent* pEvent = new QMouseEvent(_mouseEvent->type(), dock->plotWidget->mapFromGlobal(mapToGlobal(_mouseEvent->pos())), Qt::NoButton, Qt::LeftButton, Qt::AltModifier);
					QApplication::instance()->sendEvent(dock->plotWidget, pEvent);
				} 
				else if (_mouseEvent->modifiers().testFlag(Qt::ShiftModifier))
				{
					dock->plotWidget->axisRect()->setRangeDrag(Qt::Vertical);
					dock->plotWidget->axisRect()->setRangeDragAxes(dock->plotWidget->xAxis, dock->plotWidget->yAxis);
					QMouseEvent* pEvent	 = new QMouseEvent(_mouseEvent->type(), dock->plotWidget->mapFromGlobal(mapToGlobal(_mouseEvent->pos())), Qt::NoButton, Qt::LeftButton, Qt::AltModifier);
					QApplication::instance()->sendEvent(dock->plotWidget, pEvent);
				}
				else if (_mouseEvent->modifiers().testFlag(Qt::ControlModifier))
				{
					dock->plotWidget->axisRect()->setRangeDrag(Qt::Vertical);
					dock->plotWidget->axisRect()->setRangeDragAxes(dock->plotWidget->xAxis, dock->plotWidget->yAxis2);
					QMouseEvent* pEvent = new QMouseEvent(_mouseEvent->type(), dock->plotWidget->mapFromGlobal(mapToGlobal(_mouseEvent->pos())), Qt::NoButton, Qt::LeftButton, Qt::AltModifier);
					QApplication::instance()->sendEvent(dock->plotWidget, pEvent);
				}

				return true;
			}
			else if (_mouseEvent->buttons() == Qt::XButton1 && _mouseEvent->modifiers().testFlag(Qt::ShiftModifier))
			{
				
			}
			else if (_mouseEvent->buttons() == Qt::LeftButton)
			{
				if (_mouseEvent->modifiers().testFlag(Qt::AltModifier))
				{
					return false;
				}
				if (_mouseEvent->modifiers().testFlag(Qt::ShiftModifier))
				{
					double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
						                       ? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
					int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
						                ? 0 : 1;

					double x = dock->plotWidget->xAxis->pixelToCoord(_mouseEvent->pos().x());
					int frame = (x - posOffset) / posMultiplier + 0.5;;

					startFrame = frame;
					endFrame = frame;

					if (frame >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1 &&
						frame <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1)
					{
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
						frame <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1)
					{
						State::getInstance()->changeActiveFrameTrial(frame);
					}
				}
				return true;
			}
		}
		if (event->type() == QEvent::MouseMove)
		{
			QMouseEvent* _mouseEvent = static_cast<QMouseEvent*>(event);
			if (_mouseEvent->buttons() == Qt::RightButton)
			{
				QMouseEvent* pEvent = new QMouseEvent(_mouseEvent->type(), dock->plotWidget->mapFromGlobal(mapToGlobal(_mouseEvent->pos())), Qt::NoButton, Qt::LeftButton, Qt::AltModifier);
				QApplication::instance()->sendEvent(dock->plotWidget, pEvent);

				return true;
			}
			else if (_mouseEvent->buttons() == Qt::LeftButton)
			{
				if (_mouseEvent->modifiers().testFlag(Qt::AltModifier))
				{
					return false;
				}
				else if (_mouseEvent->modifiers().testFlag(Qt::ShiftModifier))
				{
					double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
						                       ? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
					int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
						                ? 0 : 1;


					double x = dock->plotWidget->xAxis->pixelToCoord(_mouseEvent->pos().x());
					int frame = (x - posOffset) / posMultiplier + 0.5;;
					endFrame = frame;

					if (frame >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1 &&
						frame <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1)
					{
						State::getInstance()->changeActiveFrameTrial(frame);
					}
					return true;
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
						frame <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1)
					{
						State::getInstance()->changeActiveFrameTrial(frame);
					}
					return true;
				}
			}
		}
	}

	return false;
}

void PlotWindow::resetRange(bool recreateStatus)
{
	if (recreateStatus){
		for (unsigned int c = 0; c < marker_status.size(); c++)
		{
			for (unsigned int f = 0; f < marker_status[c].size(); f++)
			{
				dock->plotWidget->removeItem(marker_status[c][f]);
			}
			marker_status[c].clear();
		}
		marker_status.clear();
	}

	if (State::getInstance()->getActiveTrial() >= 0 && State::getInstance()->getActiveTrial() < (int) Project::getInstance()->getTrials().size())
	{
		double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			                       ? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
		int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			                ? 0 : 1;

		dock->plotWidget->xAxis->setRange(
			(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1) * posMultiplier + posOffset,
			(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1) * posMultiplier + posOffset);

		if (recreateStatus){
			if (dock->checkBoxStatus->isChecked()){
				for (unsigned int c = 0; c < Project::getInstance()->getCameras().size(); c++)
				{
					std::vector<QCPItemRect *> rects;
					for (int f = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame(); f <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame(); f++)
					{
						rects.push_back(new QCPItemRect(dock->plotWidget));
						dock->plotWidget->addItem(rects.back());
					}
					marker_status.push_back(rects);
				}
			}
		}

		if (this->isVisible()) dock->plotWidget->replot();
	}
}

void PlotWindow::activeTrialChanged(int activeTrial)
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (activeTrial >= 0)
		{
			updateMarkers(false);
			if (dock->comboBoxPlotType->currentIndex() == 4)
			{
				if (!updating && State::getInstance()->getActiveTrial() >= 0)
				{
					activeRigidBodyChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx());
				}
			}
			else
			{
				if (!updating && State::getInstance()->getActiveTrial() >= 0)
				{
					activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
				}
			}
		}
		updateTimeCheckBox();
	}
}

void PlotWindow::draw()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION && this->isVisible())
	{
		if (dock->comboBoxPlotType->currentIndex() == 0)
		{
			plot2D(dock->comboBoxMarker1->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 1)
		{
			plot3D(dock->comboBoxMarker1->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 2)
		{
			plotDistance(dock->comboBoxMarker1->currentIndex(), dock->comboBoxMarker2->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 3)
		{
			plotReprojectionError(dock->comboBoxMarker1->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 4)
		{
			plotRigidBody(dock->comboBoxRigidBody->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 5)
		{
			plotRigidBodyError(dock->comboBoxRigidBody->currentIndex());
		}
	}
}

void PlotWindow::workspaceChanged(work_state workspace)
{
	if (workspace == DIGITIZATION)
	{
		updateMarkers(false);

		if (dock->comboBoxPlotType->currentIndex() == 4 || dock->comboBoxPlotType->currentIndex() == 5)
		{
			if (!updating && State::getInstance()->getActiveTrial() >= 0)
			{
				activeRigidBodyChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveRBIdx());
			}
		}
		else
		{
			if (!updating && State::getInstance()->getActiveTrial() >= 0)
			{
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
	if (idx >= 0)
	{
		if (dock->comboBoxPlotType->currentIndex() == 0)
		{
			updating = true;
			dock->comboBoxMarker1->setCurrentIndex(idx);
			updating = false;
			plot2D(dock->comboBoxMarker1->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 1)
		{
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
	if (idx >= 0)
	{
		if (dock->comboBoxPlotType->currentIndex() == 4)
		{
			updating = true;
			dock->comboBoxRigidBody->setCurrentIndex(idx);
			updating = false;
			plotRigidBody(dock->comboBoxRigidBody->currentIndex());
		}
		if (dock->comboBoxPlotType->currentIndex() == 5)
		{
			updating = true;
			dock->comboBoxRigidBody->setCurrentIndex(idx);
			updating = false;
			plotRigidBodyError(dock->comboBoxRigidBody->currentIndex());
		}
	}
}

void PlotWindow::updateMarkers(bool rememberSelection)
{
	if ((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
	{
		int idx1 = dock->comboBoxMarker1->currentIndex();
		int idx2 = dock->comboBoxMarker2->currentIndex();
		int idxCam = dock->comboBoxCamera->currentIndex();
		int idxrb = dock->comboBoxRigidBody->currentIndex();

		dock->comboBoxMarker1->clear();
		dock->comboBoxMarker2->clear();
		updating = true;
		for (unsigned int i = 0; i < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); i++)
		{
			dock->comboBoxMarker1->addItem(QString::number(i + 1));
			dock->comboBoxMarker2->addItem(QString::number(i + 1));
		}

		dock->comboBoxCamera->clear();
		dock->comboBoxCamera->addItem("All Cameras");
		for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
		{
			dock->comboBoxCamera->addItem(Project::getInstance()->getCameras()[i]->getName());
		}


		dock->comboBoxRigidBody->clear();
		updating = true;
		for (unsigned int i = 0; i < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size(); i++)
		{
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
	if (this->isVisible())
	{
		double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			                       ? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
		int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			                ? 0 : 1;

		dock->plotWidget->clearGraphs();

		double max_val_x = 0;
		double min_val_x = 10000;
		double max_val_y = 0;
		double min_val_y = 10000;

		if (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
		{
			dock->plotWidget->xAxis->setLabel("Time in seconds");
		}
		else
		{
			dock->plotWidget->xAxis->setLabel("Frame");
		}
		dock->plotWidget->yAxis->setLabel("X - Position");
		dock->plotWidget->yAxis2->setVisible(true);
		dock->plotWidget->yAxis2->setLabel("Y - Position");

		if ((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx1 >= 0 && idx1 < (int) Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size())
		{
			for (unsigned int cam = 0; cam < Project::getInstance()->getCameras().size(); cam++)
			{
				if (dock->comboBoxCamera->currentIndex() != 0) cam = dock->comboBoxCamera->currentIndex() - 1;

				dock->plotWidget->addGraph();
				dock->plotWidget->addGraph();

				QVector<double> pos, x, y;

				for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
				{
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus2D()[cam][i] > UNDEFINED)
					{
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
			double offset = (dock->checkBoxStatus->isChecked()) ? range * (0.05 * (Project::getInstance()->getCameras().size() + 1)): 0; 
			dock->plotWidget->yAxis->setRange(center - range * 0.5, center + range * 0.5 + offset);
			
			selectionMarker->topLeft->setCoords(startFrame * posMultiplier + posOffset, center + range * 0.5);
			selectionMarker->bottomRight->setCoords(endFrame * posMultiplier + posOffset, center - range * 0.5);

			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center - range * 0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center + range * 0.5);

			drawStatus(idx1);

			center = (max_val_y + min_val_y) * 0.5;
			range = (max_val_y - min_val_y) > range ? (max_val_y - min_val_y) : range;
			offset = (dock->checkBoxStatus->isChecked()) ? range * (0.05 * (Project::getInstance()->getCameras().size() + 1)) : 0;
			dock->plotWidget->yAxis2->setRange(center - range * 0.5, center + range * 0.5 + offset);
		}
		dock->plotWidget->replot();
		dock->plotWidget->show();
	}
}

void PlotWindow::plot3D(int idx1)
{
	if (this->isVisible())
	{
		double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			                       ? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
		int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			                ? 0 : 1;

		dock->plotWidget->clearGraphs();

		double max_val = 0;
		double min_val = 10000;

		if (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
		{
			dock->plotWidget->xAxis->setLabel("Time in seconds");
		}
		else
		{
			dock->plotWidget->xAxis->setLabel("Frame");
		}
		dock->plotWidget->yAxis->setLabel("Position");
		dock->plotWidget->yAxis2->setVisible(false);

		if ((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx1 >= 0 && idx1 < (int) Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size())
		{
			dock->plotWidget->addGraph();
			dock->plotWidget->addGraph();
			dock->plotWidget->addGraph();

			QVector<double> pos, x, y, z;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED)
				{
					x.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints3D()[i].x);
					y.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints3D()[i].y);
					z.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints3D()[i].z);

					pos.push_back(i * posMultiplier + posOffset);

					if (x[pos.size() - 1] > max_val) max_val = x[pos.size() - 1];
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
			double offset = (dock->checkBoxStatus->isChecked()) ? range * (0.05 * (Project::getInstance()->getCameras().size() + 1)) : 0;
			dock->plotWidget->yAxis->setRange(center - range * 0.5, center + range * 0.5 + offset);


			selectionMarker->topLeft->setCoords(startFrame * posMultiplier + posOffset, center + range * 0.5);
			selectionMarker->bottomRight->setCoords(endFrame * posMultiplier + posOffset, center - range * 0.5);

			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center - range * 0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center + range * 0.5);

			drawStatus(idx1);
		}
		dock->plotWidget->replot();
		dock->plotWidget->show();
	}
}

void PlotWindow::plotRigidBody(int idx)
{
	if (this->isVisible())
	{
		dock->plotWidget->clearGraphs();
		if ((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx >= 0 && idx < (int) Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size())
		{
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


			if (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			{
				if (dock->comboBoxRigidBodyTransType->currentIndex() > 0)
				{
					dock->plotWidget->xAxis->setLabel("Time in seconds\nCutoff Frequency: " + QString::number(cutoff) + "Hz");
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
					dock->plotWidget->xAxis->setLabel("Frame\nCutoff Frequency: " + QString::number(cutoff) + "Hz");
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
				dock->comboBoxRigidBodyTransPart->currentIndex() == 2)
			{
				nbGraphs = 3;
			}
			else
			{
				nbGraphs = 1;
			}

			if (dock->comboBoxRigidBodyTransType->currentIndex() == 2) nbGraphs *= 2;

			for (int i = 0; i < nbGraphs; i++)
			{
				dock->plotWidget->addGraph();
			}

			QVector<double> pos;
			QVector<double> pos_filtered;

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
									(filtered && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getPoseFiltered()[i] > 0))
								{
									val.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getRotationEulerAngle(filtered, i, z));
								}
							}

							for (int i = 0; i < val.size(); i++)
							{
								if (i > 0 && fabs(val[i - 1] - val[i]) > 270)
								{
									if (val[i] < val[i - 1])
									{
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

							if (filtered)
							{
								dock->plotWidget->graph(currentPlot)->setData(pos_filtered, val);
							}
							else
							{
								dock->plotWidget->graph(currentPlot)->setData(pos, val);
							}
							dock->plotWidget->graph(currentPlot)->setLineStyle(QCPGraph::lsNone);

							if (filtered)
							{
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
								if ((!filtered && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getPoseComputed()[i] > 0) ||
									(filtered && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getPoseFiltered()[i] > 0))
								{
									val.push_back(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getTranslationVector(filtered)[i][z]);
									if (val.last() > max_val_trans) max_val_trans = val.last();
									if (val.last() < min_val_trans) min_val_trans = val.last();
								}
							}

							if (filtered)
							{
								dock->plotWidget->graph(currentPlot)->setData(pos_filtered, val);
							}
							else
							{
								dock->plotWidget->graph(currentPlot)->setData(pos, val);
							}
							dock->plotWidget->graph(currentPlot)->setLineStyle(QCPGraph::lsNone);

							if (filtered)
							{
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

			drawStatus(-1);
		}
		dock->plotWidget->replot();
		dock->plotWidget->show();
	}
}

void PlotWindow::plotRigidBodyError(int idx)
{
	if (this->isVisible())
	{
		double cutoff = 0;
		QString meanString = "";

		dock->plotWidget->clearGraphs();
		if ((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx >= 0 && idx < (int) Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size())
		{
			cutoff = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getOverrideCutoffFrequency() ? Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx]->getCutoffFrequency() :
				         Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getCutoffFrequency();

			double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
				                       ? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
			int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
				                ? 0 : 1;

			RigidBody* body = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies()[idx];

			double max_val = 0;
			double min_val = 10000;

			double UnfilteredMean = 0;
			double UnfilteredSD = 0;
			int countUnfiltered = 0;
			double FilteredMean = 0;
			double FilteredSD = 0;
			int countFiltered = 0;

			dock->plotWidget->addGraph();

			if (dock->comboBoxRigidBodyTransType->currentIndex() == 2)
			{
				dock->plotWidget->addGraph();
			}

			bool drawUnfiltered = dock->comboBoxRigidBodyTransType->currentIndex() == 2 || dock->comboBoxRigidBodyTransType->currentIndex() == 0;
			bool drawFiltered = dock->comboBoxRigidBodyTransType->currentIndex() == 2 || dock->comboBoxRigidBodyTransType->currentIndex() == 1;

			bool draw3D = dock->comboBoxRigidBodyError->currentIndex() == 0;

			QVector<double> pos, errorMean, errorSD, errorMeanFiltered, errorSDFiltered;

			if (draw3D)
			{
				dock->plotWidget->yAxis->setLabel("3D Error");
			}
			else
			{
				dock->plotWidget->yAxis->setLabel("2D Error in pixel");
			}

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++)
			{
				pos.push_back(i * posMultiplier + posOffset);

				if (drawUnfiltered)
				{
					if (body->getPoseComputed()[i])
					{
						if (draw3D)
						{
							errorMean.push_back(body->getErrorMean3D()[i]);
							errorSD.push_back(body->getErrorSd3D()[i]);
						}
						else
						{
							errorMean.push_back(body->getErrorMean2D()[i]);
							errorSD.push_back(body->getErrorSd2D()[i]);
						}

						if (errorMean[pos.size() - 1] + errorSD[pos.size() - 1] > max_val) max_val = errorMean[pos.size() - 1] + errorSD[pos.size() - 1];
						if (errorMean[pos.size() - 1] - errorSD[pos.size() - 1] < min_val) min_val = errorMean[pos.size() - 1] - errorSD[pos.size() - 1];
					}
					else
					{
						errorMean.push_back(-2);
						errorSD.push_back(0);
					}
				}

				if (drawFiltered)
				{
					if (body->getPoseFiltered()[i])
					{
						if (draw3D)
						{
							errorMeanFiltered.push_back(body->getErrorMean3D_filtered()[i]);
							errorSDFiltered.push_back(body->getErrorSd3D_filtered()[i]);
						}
						else
						{
							errorMeanFiltered.push_back(body->getErrorMean2D_filtered()[i]);
							errorSDFiltered.push_back(body->getErrorSd2D_filtered()[i]);
						}

						if (errorMeanFiltered[pos.size() - 1] + errorSDFiltered[pos.size() - 1] > max_val) max_val = errorMeanFiltered[pos.size() - 1] + errorSDFiltered[pos.size() - 1];
						if (errorMeanFiltered[pos.size() - 1] - errorSDFiltered[pos.size() - 1] < min_val) min_val = errorMeanFiltered[pos.size() - 1] - errorSDFiltered[pos.size() - 1];
					}
					else
					{
						errorMeanFiltered.push_back(-2);
						errorSDFiltered.push_back(0);
					}
				}
			}

			if (drawUnfiltered)
			{
				for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++ , count++)
				{
					if (body->getPoseComputed()[i])
					{
						UnfilteredMean += errorMean[count];
						countUnfiltered++;
					}
				}
				if (countUnfiltered > 0)UnfilteredMean = UnfilteredMean / countUnfiltered;

				for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++ , count++)
				{
					if (body->getPoseFiltered()[i])
					{
						UnfilteredSD += pow(errorMean[count] - UnfilteredMean, 2);
					}
				}

				if (countUnfiltered > 1)UnfilteredSD = sqrt(UnfilteredSD / (countUnfiltered - 1));
			}

			if (drawFiltered)
			{
				for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++ , count++)
				{
					if (body->getPoseFiltered()[i])
					{
						FilteredMean += errorMeanFiltered[count];
						countFiltered++;
					}
				}
				if (countFiltered > 0)FilteredMean = FilteredMean / countFiltered;

				for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++ , count++)
				{
					if (body->getPoseFiltered()[i])
					{
						FilteredSD += pow(errorMeanFiltered[count] - FilteredMean, 2);
					}
				}

				if (countFiltered > 1)FilteredSD = sqrt(FilteredSD / (countFiltered - 1));
			}

			// create graph and assign data to it:
			if (drawFiltered && !drawUnfiltered)
			{
				dock->plotWidget->graph(0)->setDataValueError(pos, errorMeanFiltered, errorSDFiltered);
				dock->plotWidget->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
				dock->plotWidget->graph(0)->setPen(QPen(QColor(Qt::darkBlue)));
				dock->plotWidget->graph(0)->setErrorType(QCPGraph::etValue);
				dock->plotWidget->graph(0)->setErrorPen(QPen(QColor(Qt::blue)));
				dock->plotWidget->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDot, 1));
			}
			else if (!drawFiltered && drawUnfiltered)
			{
				dock->plotWidget->graph(0)->setDataValueError(pos, errorMean, errorSD);
				dock->plotWidget->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
				dock->plotWidget->graph(0)->setPen(QPen(QColor(Qt::darkRed)));
				dock->plotWidget->graph(0)->setErrorType(QCPGraph::etValue);
				dock->plotWidget->graph(0)->setErrorPen(QPen(QColor(Qt::red)));
				dock->plotWidget->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDot, 1));
			}
			else
			{
				dock->plotWidget->graph(0)->setDataValueError(pos, errorMeanFiltered, errorSDFiltered);
				dock->plotWidget->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
				dock->plotWidget->graph(0)->setPen(QPen(QColor(Qt::darkBlue)));
				dock->plotWidget->graph(0)->setErrorType(QCPGraph::etValue);
				dock->plotWidget->graph(0)->setErrorPen(QPen(QColor(Qt::blue)));
				dock->plotWidget->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDot, 1));

				dock->plotWidget->graph(1)->setDataValueError(pos, errorMean, errorSD);
				dock->plotWidget->graph(1)->setLineStyle(QCPGraph::lsStepCenter);
				dock->plotWidget->graph(1)->setPen(QPen(QColor(Qt::darkRed)));
				dock->plotWidget->graph(1)->setErrorType(QCPGraph::etValue);
				dock->plotWidget->graph(1)->setErrorPen(QPen(QColor(Qt::red)));
				dock->plotWidget->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDot, 1));
			}

			// set axes ranges, so we see all data:
			double range = 0.2;
			double center = (max_val + min_val) * 0.5;
			range = (max_val - min_val) > range ? (max_val - min_val) : range;

			dock->plotWidget->yAxis->setRange(center - range * 0.5, center + range * 0.5);

			selectionMarker->topLeft->setCoords(startFrame * posMultiplier + posOffset, center + range * 0.5);
			selectionMarker->bottomRight->setCoords(endFrame * posMultiplier + posOffset, center - range * 0.5);

			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center - range * 0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, center + range * 0.5);
			
			drawStatus(-1);

			meanString = meanString + "\n";

			if (drawUnfiltered)
			{
				meanString = meanString + " Mean error unfiltered: " + QString::number(UnfilteredMean) + " +/- " + QString::number(UnfilteredSD);
			}

			if (drawFiltered)
			{
				meanString = meanString + " Mean error filtered: " + QString::number(FilteredMean) + " +/- " + QString::number(FilteredSD);
			}
		}


		if (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
		{
			if (dock->comboBoxRigidBodyTransType->currentIndex() > 0)
			{
				dock->plotWidget->xAxis->setLabel("Time in seconds\nCutoff Frequency: " + QString::number(cutoff) + "Hz" + meanString);
			}
			else
			{
				dock->plotWidget->xAxis->setLabel("Time in seconds" + meanString);
			}
		}
		else
		{
			if (dock->comboBoxRigidBodyTransType->currentIndex() > 0)
			{
				dock->plotWidget->xAxis->setLabel("Frame\nCutoff Frequency: " + QString::number(cutoff) + "Hz" + meanString);
			}
			else
			{
				dock->plotWidget->xAxis->setLabel("Frame" + meanString);
			}
		}

		dock->plotWidget->replot();
		dock->plotWidget->show();
	}
}

void PlotWindow::plotDistance(int idx1, int idx2)
{
	if (this->isVisible())
	{
		double posMultiplier = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			                       ? 1.0 / Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() : 1.0;
		int posOffset = (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
			                ? 0 : 1;

		dock->plotWidget->clearGraphs();

		dock->plotWidget->yAxis2->setVisible(false);

		double sd = 0;
		double mean = 0;

		if ((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx1 >= 0 && idx1 < (int) Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size() &&
			idx2 >= 0 && idx2 < (int) Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size())
		{
			dock->plotWidget->addGraph();


			QVector<double>
				x(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() + 1),
				y(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() + 1); // initialize with entries 0..100
			double max_val = 0;
			double min_val = 10000;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++ , count++)
			{
				x[count] = i * posMultiplier + posOffset; // x goes from -1 to 1
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getStatus3D()[i] > UNDEFINED)
				{
					cv::Point3d diff = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getPoints3D()[i] - Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getPoints3D()[i];
					y[count] = cv::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
					if (y[count] > max_val) max_val = y[count];
					if (y[count] < min_val) min_val = y[count];
				}
				else
				{
					y[count] = 0; // let's plot a quadratic function
				}
			}
			int countVisible = 0;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++ , count++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getStatus3D()[i] > UNDEFINED)
				{
					mean += y[count];
					countVisible++;
				}
			}
			if (countVisible > 0)mean = mean / countVisible;

			for (int i = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1, count = 0; i <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1; i++ , count++)
			{
				if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx1]->getStatus3D()[i] > UNDEFINED && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[idx2]->getStatus3D()[i] > UNDEFINED)
				{
					sd += pow(y[count] - mean, 2);
				}
			}

			if (countVisible > 1)sd = sqrt(sd / (countVisible - 1));

			// create graph and assign data to it:
			dock->plotWidget->graph(0)->setData(x, y);
			dock->plotWidget->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
			dock->plotWidget->graph(0)->setPen(QPen(QColor(Qt::blue)));

			dock->plotWidget->addGraph();
			QVector<double> x2, y2;
			x2.push_back((Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1) * posMultiplier + posOffset);
			x2.push_back((Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1) * posMultiplier + posOffset);
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

			drawStatus(-1);
		}
		if (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
		{
			dock->plotWidget->xAxis->setLabel("Time in seconds\nMean intermarker distance: " + QString::number(mean) + " +/- " + QString::number(sd));
		}
		else
		{
			dock->plotWidget->xAxis->setLabel("Frame\nMean intermarker distance: " + QString::number(mean) + " +/- " + QString::number(sd));
		}

		dock->plotWidget->yAxis->setLabel("Distance Marker " + QString::number(idx1 + 1) + " to Marker " + QString::number(idx2 + 1));

		dock->plotWidget->replot();
		dock->plotWidget->show();
	}
}

void PlotWindow::plotReprojectionError(int idx1)
{
	if (this->isVisible())
	{
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

		if ((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0 &&
			idx1 >= 0 && idx1 < (int) Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size())
		{
			dock->plotWidget->addGraph();
			QVector<double> pos, error;

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
					for (unsigned int cam = 0; cam < Project::getInstance()->getCameras().size(); cam++)
					{
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
						error.push_back(error_val / count);
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
				if (error[i] >= 0)
				{
					mean += error[i];
					countVisible++;
				}
			}
			if (countVisible > 0)mean = mean / countVisible;

			for (int i = 0; i < error.size(); i++)
			{
				if (error[i] >= 0)
				{
					sd += pow(error[i] - mean, 2);
				}
			}

			if (countVisible > 1)sd = sqrt(sd / (countVisible - 1));

			dock->plotWidget->graph(0)->setData(pos, error);
			dock->plotWidget->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
			dock->plotWidget->graph(0)->setPen(QPen(QColor(Qt::blue)));

			double range = 0.2;
			range = (max_val) > range ? (max_val * 1.1) : range;
			double offset = (dock->checkBoxStatus->isChecked()) ? (range + 0.5) * (0.05 * (Project::getInstance()->getCameras().size() + 1)) : 0;
			dock->plotWidget->yAxis->setRange(-0.5, range + offset);

			selectionMarker->topLeft->setCoords(startFrame * posMultiplier + posOffset, range);
			selectionMarker->bottomRight->setCoords(endFrame * posMultiplier + posOffset, -0.5);

			frameMarker->start->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, -0.5);
			frameMarker->end->setCoords(State::getInstance()->getActiveFrameTrial() * posMultiplier + posOffset, range);

			drawStatus(idx1);
		}

		if (dock->checkBoxTime->isChecked() && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() > 0)
		{
			dock->plotWidget->xAxis->setLabel("Time in seconds\nMean reprojection error" + QString::number(mean) + " +/- " + QString::number(sd));
		}
		else
		{
			dock->plotWidget->xAxis->setLabel("Frame\nMean reprojection error: " + QString::number(mean) + " +/- " + QString::number(sd));
		}


		dock->plotWidget->replot();
		dock->plotWidget->show();
	}
}

void PlotWindow::on_comboBoxCamera_currentIndexChanged(int idx)
{
	if (!updating)
	{
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

		dock->checkBoxStatus->show();

		if (!updating && State::getInstance()->getActiveTrial() >= 0)
		{
			activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		}
	}
	else if (idx == 1)
	{
		dock->labelCamera->hide();
		dock->comboBoxCamera->hide();

		dock->labelMarker1->show();
		dock->comboBoxMarker1->show();

		dock->labelMarker2->hide();
		dock->comboBoxMarker2->hide();

		dock->frameMarkerPlot->show();
		dock->frameRigidBodyPlot->hide();

		dock->checkBoxStatus->show();

		if (!updating && State::getInstance()->getActiveTrial() >= 0)
		{
			activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		}
	}
	else if (idx == 2)
	{
		dock->labelCamera->hide();
		dock->comboBoxCamera->hide();

		dock->labelMarker1->show();
		dock->comboBoxMarker1->show();

		dock->labelMarker2->show();
		dock->comboBoxMarker2->show();

		dock->frameMarkerPlot->show();
		dock->frameRigidBodyPlot->hide();

		dock->checkBoxStatus->hide();

		if (!updating && State::getInstance()->getActiveTrial() >= 0)
		{
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

		dock->checkBoxStatus->show();

		if (!updating && State::getInstance()->getActiveTrial() >= 0)
		{
			activePointChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx());
		}
	}
	else if (idx == 4)
	{
		dock->frameMarkerPlot->hide();
		dock->frameRigidBodyPlot->show();
		dock->comboBoxRigidBodyTransPart->show();
		dock->comboBoxRigidBodyError->hide();

		dock->checkBoxStatus->hide();

		plotRigidBody(dock->comboBoxRigidBody->currentIndex());
	}
	else if (idx == 5)
	{
		dock->frameMarkerPlot->hide();
		dock->frameRigidBodyPlot->show();
		dock->comboBoxRigidBodyTransPart->hide();
		dock->comboBoxRigidBodyError->show();

		dock->checkBoxStatus->hide();

		plotRigidBodyError(dock->comboBoxRigidBody->currentIndex());
	}
}

void PlotWindow::on_comboBoxMarker1_currentIndexChanged(int idx)
{
	if (!updating)
	{
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
		else if (dock->comboBoxPlotType->currentIndex() == 2)
		{
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

void PlotWindow::on_comboBoxMarker2_currentIndexChanged(int idx)
{
	if (dock->comboBoxPlotType->currentIndex() == 2)
	{
		if (!updating)plotDistance(dock->comboBoxMarker1->currentIndex(), dock->comboBoxMarker2->currentIndex());
	}
}

void PlotWindow::on_pushButton_Reset_clicked()
{
	resetRange(false);
	draw();
}

void PlotWindow::closeEvent(QCloseEvent* event)
{
	event->ignore();
	MainWindow::getInstance()->on_actionPlot_triggered(false);
}

void PlotWindow::on_pushButtonSave_clicked()
{
	saveData();
}

void PlotWindow::on_pushButtonUpdate_clicked()
{
	if (State::getInstance()->getActiveTrial() >= 0)
	{
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->recomputeAndFilterRigidBodyTransformations();
	}

	if (dock->comboBoxPlotType->currentIndex() == 4)
	{
		plotRigidBody(dock->comboBoxRigidBody->currentIndex());
	}
	else if (dock->comboBoxPlotType->currentIndex() == 5)
	{
		plotRigidBodyError(dock->comboBoxRigidBody->currentIndex());
	}
}

void PlotWindow::on_comboBoxRigidBody_currentIndexChanged(int idx)
{
	if (!updating)
	{
		PointsDockWidget::getInstance()->selectBody(idx + 1);

		if (dock->comboBoxPlotType->currentIndex() == 4)
		{
			plotRigidBody(dock->comboBoxRigidBody->currentIndex());
		}
		else if (dock->comboBoxPlotType->currentIndex() == 5)
		{
			plotRigidBodyError(dock->comboBoxRigidBody->currentIndex());
		}
	}
}

void PlotWindow::on_comboBoxRigidBodyTransPart_currentIndexChanged(int idx)
{
	plotRigidBody(dock->comboBoxRigidBody->currentIndex());
}

void PlotWindow::on_comboBoxRigidBodyError_currentIndexChanged(int idx)
{
	plotRigidBodyError(dock->comboBoxRigidBody->currentIndex());
}

void PlotWindow::on_comboBoxRigidBodyTransType_currentIndexChanged(int idx)
{
	if (dock->comboBoxPlotType->currentIndex() == 4)
	{
		plotRigidBody(dock->comboBoxRigidBody->currentIndex());
	}
	else if (dock->comboBoxPlotType->currentIndex() == 5)
	{
		plotRigidBodyError(dock->comboBoxRigidBody->currentIndex());
	}
}

void PlotWindow::on_checkBoxTime_clicked()
{
	resetRange(false);
	draw();
}

void PlotWindow::on_checkBoxStatus_clicked()
{
	resetRange();
	draw();
}
