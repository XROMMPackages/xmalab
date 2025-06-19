//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
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
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file ProjectOverview.cpp
///\author Benjamin Knorlein
///\date 7/26/2017

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/ProjectOverview.h"
#include "ui_ProjectOverview.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/UndistortionObject.h"
#include "core/CalibrationImage.h"
#include "core/Marker.h"
#include "core/RigidBody.h"
#include "core/CalibrationObject.h"
#include "core/Trial.h"
#include <QFileInfo>

using namespace xma;


ProjectOverview::ProjectOverview(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::ProjectOverview)
{
	diag->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	update();
}

ProjectOverview::~ProjectOverview()
{
	delete diag;
}

void ProjectOverview::update()
{
	diag->treeWidget->clear();
	diag->treeWidget->addTopLevelItem(new QTreeWidgetItem(QStringList() << "Repository" << Project::getInstance()->getRepository()));
	diag->treeWidget->addTopLevelItem(new QTreeWidgetItem(QStringList() << "Created" << Project::getInstance()->get_date_created()));


	if (Project::getInstance()->getCalibration() != NO_CALIBRATION){
		QTreeWidgetItem * item;
		if (Project::getInstance()->getHasStudyData()){
			item = new QTreeWidgetItem(QStringList() << "Calibration Trial" << "name: " + Project::getInstance()->getStudyName());
		}
		else
		{
			item = new QTreeWidgetItem(QStringList() << "Calibration Trial" << "XMALab_name: " +  Project::getInstance()->getStudyName());		
		}
		diag->treeWidget->addTopLevelItem(item);

		bool hasUndist = false;
		for (auto cams : Project::getInstance()->getCameras())
		{
			if (cams->hasUndistortion()) {
				hasUndist = true;
				break;
			}
		}

		if (hasUndist){
			QTreeWidgetItem * undistortion_item = new QTreeWidgetItem(QStringList() << "Undistortion" << "");
			item->addChild(undistortion_item);

			for (auto cams : Project::getInstance()->getCameras())
			{
				if (cams->hasUndistortion()) {
					QTreeWidgetItem * cam_item = new QTreeWidgetItem(QStringList() << "Camera " + QString::number(cams->getID() + 1) << ((cams->getPortalId() > 0) ? "(cam" + QString::number(cams->getPortalId()) + ")" : ""));
					undistortion_item->addChild(cam_item);
					//if (cams->getPortalId() > 0) cam_item->addChild(new QTreeWidgetItem(QStringList() << "Portal camera label:" << "cam" + QString::number(cams->getPortalId())));
					cam_item->addChild(new QTreeWidgetItem(QStringList() << "Undistortion File" << cams->getUndistortionObject()->getFilename()));
				}
			}
		}

		QTreeWidgetItem * calib_item = new QTreeWidgetItem(QStringList() << "Calibration" << "");
		item->addChild(calib_item);

		for (auto cams : Project::getInstance()->getCameras())
		{
			if (cams->hasUndistortion()) {
				QTreeWidgetItem * cam_item = new QTreeWidgetItem(QStringList() << "Camera " + QString::number(cams->getID() + 1) << ((cams->getPortalId() > 0) ? "(cam" + QString::number(cams->getPortalId()) + ")" : ""));
				calib_item->addChild(cam_item);
				//if (cams->getPortalId() > 0) calib_item->addChild(new QTreeWidgetItem(QStringList() << "Portal camera label:" << "cam" + QString::number(cams->getPortalId())));
				cam_item->addChild(new QTreeWidgetItem(QStringList() << "Camera points" << QString::number(cams->getCalibrationNbInlier())));
				cam_item->addChild(new QTreeWidgetItem(QStringList() << "Camera error" << QString::number(cams->getCalibrationError())));
				cam_item->addChild(new QTreeWidgetItem(QStringList() << "Undistortion File" << cams->getUndistortionObject()->getFilename()));

				for (auto f : cams->getCalibrationImages()){			
					QTreeWidgetItem * camFile_item = new QTreeWidgetItem(QStringList() << "Calib. file:" << f->getFilename());
					cam_item->addChild(camFile_item);
					camFile_item->addChild(new QTreeWidgetItem(QStringList() << "Camera file points" << QString::number(f->getCalibrationNbInlier())));
					camFile_item->addChild(new QTreeWidgetItem(QStringList() << "Camera file error" << QString::number(f->getCalibrationError())));
				}
			}
		}

		QTreeWidgetItem * object_item;
		if (CalibrationObject::getInstance()->isCheckerboard())
		{
			object_item = new QTreeWidgetItem(QStringList() << "Calibration Object" << "Checkerboard");
			object_item->addChild(new QTreeWidgetItem(QStringList() << "Width" << QString::number(CalibrationObject::getInstance()->getNbHorizontalSquares())));
			object_item->addChild(new QTreeWidgetItem(QStringList() << "Height" << QString::number(CalibrationObject::getInstance()->getNbVerticalSquares())));
			object_item->addChild(new QTreeWidgetItem(QStringList() << "Size" << QString::number(CalibrationObject::getInstance()->getSquareSize())));
		}
		else
		{
			object_item = new QTreeWidgetItem(QStringList() << "Calibration Object" << "Framespec");
			QFileInfo info(CalibrationObject::getInstance()->getFrameSpecificationsFilename());
			object_item->addChild(new QTreeWidgetItem(QStringList() << "Calib. framespec" << info.fileName()));
			info = QFileInfo(CalibrationObject::getInstance()->getReferencesFilename());
			object_item->addChild(new QTreeWidgetItem(QStringList() << "Calib. references" << info.fileName()));
		}
		item->addChild(object_item);
	}

	for (auto t : Project::getInstance()->getTrials())
	{
		if (!t->getIsDefault()){
			QTreeWidgetItem * trial_item;
			if (t->getHasStudyData()){
				trial_item = new QTreeWidgetItem(QStringList() << "Trial" << "name: " + t->getTrialName());
			}
			else
			{
				trial_item = new QTreeWidgetItem(QStringList() << "Trial" << "XMALab_name: " + t->getName());
			}
			diag->treeWidget->addTopLevelItem(trial_item);
			
			trial_item->addChild(new QTreeWidgetItem(QStringList() << "first tracked frame" << t->getFirstTrackedFrame()));
			trial_item->addChild(new QTreeWidgetItem(QStringList() << "last tracked frame" << t->getLastTrackedFrame()));
			trial_item->addChild(new QTreeWidgetItem(QStringList() << "trial filter freq." << QString::number(t->getCutoffFrequency())));
			trial_item->addChild(new QTreeWidgetItem(QStringList() << "frame rate"  << QString::number(t->getRecordingSpeed())));
			double val2 = t->getReprojectionError();
			trial_item->addChild(new QTreeWidgetItem(QStringList() << "mean reprojection error"<< (val2 <= 0 ? "NA" : QString::number(val2))));
			val2 = t->getMarkerToMarkerSD();
			trial_item->addChild(new QTreeWidgetItem(QStringList() << "intermarker dist. mean s.d." << (val2 <= 0 ? "NA" : QString::number(val2))));

			QTreeWidgetItem * movies_item = new QTreeWidgetItem(QStringList() << "Movies" << "");
			trial_item->addChild(movies_item);
			for (int m = 0; m < t->getVideoStreams().size(); m++)
			{
				if (t->getHasStudyData()){
					movies_item->addChild(new QTreeWidgetItem(QStringList() << "Camera " + QString::number(m + 1) << t->getVideoStreams()[m]->getFilename()));
				}
				else
				{
					QFileInfo info(t->getVideoStreams()[m]->getFilenames()[0]);
					movies_item->addChild(new QTreeWidgetItem(QStringList() << "Camera " + QString::number(m + 1) << info.fileName()));
				}
			}

			std::vector <bool> pointsWritten(t->getMarkers().size(), false);

			for (auto rb : t->getRigidBodies())
			{
				QTreeWidgetItem * rb_item = new QTreeWidgetItem(QStringList() << "Rigid Body name" << rb->getDescription());
				trial_item->addChild(rb_item);
				int val = rb->getFirstTrackedFrame();
				rb_item->addChild(new QTreeWidgetItem(QStringList() << "first tracked" <<( val < 0 ? "NA" : QString::number(val))));
				val = rb->getLastTrackedFrame();
				rb_item->addChild(new QTreeWidgetItem(QStringList() << "last tracked frame" << (val < 0 ? "NA" : QString::number(val))));
				rb_item->addChild(new QTreeWidgetItem(QStringList() << "number tracked" << QString::number(rb->getFramesTracked())));
				double averageSD;
				int count;
				rb->getMarkerToMarkerSD(averageSD, count);
				rb_item->addChild(new QTreeWidgetItem(QStringList() << "intermarker dist. mean s.d." << (averageSD <= 0 ? "NA" : QString::number(averageSD))));
				double val2 = rb->getError3D(false);
				rb_item->addChild(new QTreeWidgetItem(QStringList() << "rigid body error 3D unfilt." << (val2 <= 0 ? "NA" : QString::number(val2))));
				val2 = rb->getError3D(true);
				rb_item->addChild(new QTreeWidgetItem(QStringList() << "rigid body error 3D filt." << (val2 <= 0 ? "NA" : QString::number(val2))));
				if (rb->getOverrideCutoffFrequency()){
					rb_item->addChild(new QTreeWidgetItem(QStringList() << "rigid body filter freq." << QString::number(rb->getCutoffFrequency())));
				}
				else
				{
					rb_item->addChild(new QTreeWidgetItem(QStringList() << "rigid body filter freq." << QString::number(t->getCutoffFrequency())));
				}


				for (auto idx : rb->getPointsIdx())
				{
					QTreeWidgetItem * marker_item = new QTreeWidgetItem(QStringList() << "Marker" << t->getMarkers()[idx]->getDescription());
					rb_item->addChild(marker_item);
					pointsWritten[idx] = true;
					
					marker_item->addChild(new QTreeWidgetItem(QStringList() << "marker id" <<  QString::number(idx + 1)));
					QString firstTracked;
					QString lastTracked;
					QString numberTracked;
					for (int i = 0; i != Project::getInstance()->getCameras().size(); i++)
					{
						int f = t->getMarkers()[idx]->getFirstTrackedFrame(i);
						if (f >= 0) { firstTracked += QString::number(f); }
						else{ firstTracked += "NA"; }

						f = t->getMarkers()[idx]->getLastTrackedFrame(i);
						if (f >= 0) { lastTracked += QString::number(f); }
						else{ lastTracked += "NA"; }

						f = t->getMarkers()[idx]->getFramesTracked(i);
						numberTracked += QString::number(f);

						if (i != Project::getInstance()->getCameras().size() - 1)
						{
							firstTracked += " / ";
							lastTracked += " / ";
							numberTracked += " / ";
						}
					}

					marker_item->addChild(new QTreeWidgetItem(QStringList() << "first tracked frame" << firstTracked));
					marker_item->addChild(new QTreeWidgetItem(QStringList() << "last tracked frame" << lastTracked));
					marker_item->addChild(new QTreeWidgetItem(QStringList() << "number tracked" << numberTracked));
					double val2 = t->getMarkers()[idx]->getReprojectionError();
					marker_item->addChild(new QTreeWidgetItem(QStringList() << "mean reprojection error" << (val2 <= 0 ? "NA" : QString::number(val2))));
				}

			}

			for (int idx = 0; idx < pointsWritten.size(); idx++)
			{
				if (!pointsWritten[idx])
				{
					QTreeWidgetItem * marker_item = new QTreeWidgetItem(QStringList() << "Marker" << t->getMarkers()[idx]->getDescription());
					trial_item->addChild(marker_item);
					pointsWritten[idx] = true;

					marker_item->addChild(new QTreeWidgetItem(QStringList() << "marker id" << QString::number(idx + 1)));
					QString firstTracked;
					QString lastTracked;
					QString numberTracked;
					for (int i = 0; i != Project::getInstance()->getCameras().size(); i++)
					{
						int f = t->getMarkers()[idx]->getFirstTrackedFrame(i);
						if (f >= 0) { firstTracked += QString::number(f); }
						else{ firstTracked += "NA"; }

						f = t->getMarkers()[idx]->getLastTrackedFrame(i);
						if (f >= 0) { lastTracked += QString::number(f); }
						else{ lastTracked += "NA"; }

						f = t->getMarkers()[idx]->getFramesTracked(i);
						numberTracked += QString::number(f);

						if (i != Project::getInstance()->getCameras().size() - 1)
						{
							firstTracked += " / ";
							lastTracked += " / ";
							numberTracked += " / ";
						}
					}

					marker_item->addChild(new QTreeWidgetItem(QStringList() << "first tracked frame" << firstTracked));
					marker_item->addChild(new QTreeWidgetItem(QStringList() << "last tracked frame" << lastTracked));
					marker_item->addChild(new QTreeWidgetItem(QStringList() << "number tracked" << numberTracked));
					double val2 = t->getMarkers()[idx]->getReprojectionError();
					marker_item->addChild(new QTreeWidgetItem(QStringList() << "mean reprojection error" << (val2 <= 0 ? "NA" : QString::number(val2))));
				}
			}
		}
	}




	diag->treeWidget->expandAll();
	diag->treeWidget->resizeColumnToContents(0);
	diag->treeWidget->collapseAll();
}


void ProjectOverview::on_pushButton_clicked()
{
	update();
}
