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
///\file Trial.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Trial.h"
#include "core/ImageSequence.h"
#include "core/CineVideo.h"
#include "core/AviVideo.h"
#include "core/Image.h"
#include "core/Camera.h"
#include "core/Project.h"
#include "core/RigidBody.h"
#include "core/Marker.h"
#include "core/Settings.h"
#include "core/UndistortionObject.h"
#include "core/HelperFunctions.h"

#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QXmlStreamReader>

#include <fstream>

#ifdef WIN32
#define OS_SEP "\\"
#else
#define OS_SEP "/"
#endif
#include <QtGui/QApplication>
#include <opencv2/highgui/highgui.hpp>


using namespace xma;

Trial::Trial(QString trialname, std::vector<QStringList>& imageFilenames)
{
	isDefault = false;
	IsCopyFromDefault = false;
	name = trialname;
	activeFrame = 0;
	referenceCalibrationImage = 0;
	recordingSpeed = 0;
	cutoffFrequency = 0;
	interpolateMissingFrames = 0;

	activeMarkerIdx = -1;
	activeBodyIdx = -1;

	interpolate3D = false;
	requiresRecomputation = true;

	for (std::vector<QStringList>::iterator filenameList = imageFilenames.begin(); filenameList != imageFilenames.end(); ++filenameList)
	{
		VideoStream* newSequence = NULL;
		if ((*filenameList).size() > 1)
		{
			newSequence = new ImageSequence(*filenameList);
		}
		else
		{
			QFileInfo info((*filenameList).at(0));
			if (info.suffix() == "cine")
			{
				newSequence = new CineVideo(*filenameList);
			}
			else if (info.suffix() == "avi")
			{
				newSequence = new AviVideo(*filenameList);
			}
			else
			{
				newSequence = new ImageSequence(*filenameList);
			}
		}
		videos.push_back(newSequence);
	}

	setNbImages();

	recordingSpeed = videos[0]->getFPS();
	hasStudyData = false;
	startFrame = 1;
	endFrame = nbImages;
	trialID = -1;
	trialNumber = -1;
	studyID = -1;
}

Trial::Trial(QString trialname, QString folder)
{
	isDefault = false;
	IsCopyFromDefault = false;
	interpolate3D = false;
	name = trialname;
	activeFrame = 0;
	referenceCalibrationImage = 0;
	recordingSpeed = 0;
	cutoffFrequency = 0;
	interpolateMissingFrames = 0;

	activeMarkerIdx = -1;
	activeBodyIdx = -1;
	requiresRecomputation = true;

	for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
	{
		QStringList filenameList;
		QString camera_filenames = folder + Project::getInstance()->getCameras()[i]->getName() + "_filenames_absolute.txt";
		std::ifstream fin(camera_filenames.toAscii().data());
		std::string line;
		while (!littleHelper::safeGetline(fin, line).eof())
		{
			filenameList << QString::fromStdString(line);
		}
		fin.close();

		VideoStream* newSequence = NULL;
		if (filenameList.size() > 1)
		{
			newSequence = new ImageSequence(filenameList);
		}
		else
		{
			QFileInfo info(filenameList.at(0));
			if (info.suffix() == "cine")
			{
				newSequence = new CineVideo(filenameList);
			}
			else if (info.suffix() == "avi")
			{
				newSequence = new AviVideo(filenameList);
			}
			else
			{
				newSequence = new ImageSequence(filenameList);
			}
		}

		videos.push_back(newSequence);
	}


	setNbImages();

	recordingSpeed = videos[0]->getFPS();
	hasStudyData = false;
	startFrame = 1;
	endFrame = nbImages;
	trialID = -1;
	trialNumber = -1;
	studyID = -1;
}

Trial::Trial()
{
	name = "Default";
	activeFrame = 0;
	isDefault = true;
	IsCopyFromDefault = false;
	nbImages = 1;
	startFrame = 1;
	endFrame = 1;
}

Trial::~Trial()
{
	if(!isDefault){
		for (std::vector<VideoStream*>::iterator video = videos.begin(); video != videos.end(); ++video)
		{
			delete *video;
		}
	}
	videos.clear();
	for (std::vector<RigidBody*>::iterator rigidBody = rigidBodies.begin(); rigidBody != rigidBodies.end(); ++rigidBody)
	{
		delete *rigidBody;
	}
	rigidBodies.clear();
	for (std::vector<Marker*>::iterator marker = markers.begin(); marker != markers.end(); ++marker)
	{
		delete *marker;
	}
	markers.clear();
	for (std::vector<EventData*>::iterator e = events.begin(); e != events.end(); ++e)
	{
		delete *e;
	}
	events.clear();
}

bool Trial::changeTrialData(QString trialname, std::vector<QStringList>& imageFilenames)
{
	if (isDefault)
		return false;

	name = trialname;
	std::vector<VideoStream*> video_tmp = videos;
	videos.clear();

	for (std::vector<QStringList>::iterator filenameList = imageFilenames.begin(); filenameList != imageFilenames.end(); ++filenameList)
	{
		VideoStream* newSequence = NULL;
		if ((*filenameList).size() > 1)
		{
			newSequence = new ImageSequence(*filenameList);
		}
		else
		{
			QFileInfo info((*filenameList).at(0));
			if (info.suffix() == "cine")
			{
				newSequence = new CineVideo(*filenameList);
			}
			else if (info.suffix() == "avi")
			{
				newSequence = new AviVideo(*filenameList);
			}
			else
			{
				newSequence = new ImageSequence(*filenameList);
			}
		}
		newSequence->setActiveFrame(activeFrame);
		videos.push_back(newSequence);
	}

	if (!checkTrialImageSizeValid())
	{
		//image size wrong we restore the old data
		for (std::vector<VideoStream*>::iterator video = videos.begin(); video != videos.end(); ++video)
		{
			delete *video;
		}
		videos.clear();
		videos = video_tmp;
		return false;
	} 
	else
	{
		//image size correct we delete the old data
		for (std::vector<VideoStream*>::iterator video = video_tmp.begin(); video != video_tmp.end(); ++video)
		{
			delete *video;
		}
		video_tmp.clear();	
	}

	setNbImages();
	startFrame = 1;
	endFrame = nbImages;
	return true;
}

void Trial::setNbImages()
{
	if (isDefault)
		return;

	nbImages = 0;
	for (unsigned int i = 0; i < videos.size(); i++)
	{
		if (videos[i]->getNbImages() >= 0)
		{
			if (i == 0)
			{
				nbImages = videos[i]->getNbImages();
			}
			else
			{
				assert(nbImages == videos[i]->getNbImages());
			}
		}
	}
}

void Trial::bindTextures()
{
	if (isDefault)
		return;
	for (std::vector<VideoStream*>::iterator video = videos.begin(); video != videos.end(); ++video)
	{
		(*video)->bindTexture();
	}
}

void Trial::save(QString path)
{
	if (isDefault)
		return;

	for (unsigned int i = 0; i < videos.size(); i++)
	{
		QString cameraPath = path + Project::getInstance()->getCameras()[i]->getName() + "_filenames_absolute.txt";
		videos[i]->save(cameraPath);
	}
}

int Trial::getActiveFrame()
{
	return activeFrame;
}

void Trial::setActiveFrame(int _activeFrame)
{
	activeFrame = _activeFrame;
	if (isDefault)
		return;
	
	for (int i = 0; i < videos.size(); i++)
	{
		if (Project::getInstance()->getCameras()[i]->isVisible())
			videos[i]->setActiveFrame(activeFrame);
	}
}

const std::vector<VideoStream*>& Trial::getVideoStreams()
{
	return videos;
}


const std::vector<Marker*>& Trial::getMarkers()
{
	return markers;
}

const std::vector<RigidBody*>& Trial::getRigidBodies()
{
	return rigidBodies;
}

const std::vector<EventData*>& Trial::getEvents()
{
	return events;
}

Marker* Trial::getActiveMarker()
{
	if (activeMarkerIdx >= (int) markers.size() || activeMarkerIdx < 0) return NULL;

	return markers[activeMarkerIdx];
}

bool Trial::isActiveMarkerUndefined(int camera)
{
	return ((markers.size() != 0) && (activeMarkerIdx < (int) markers.size()) && (activeMarkerIdx >= 0) && (getMarkers()[activeMarkerIdx]->getStatus2D()[camera][activeFrame] == UNDEFINED));
}

void Trial::setActiveToNextUndefinedMarker(int camera)
{
	if (markers.size() == 0)
	{
		addMarker();
		return;
	}
	else
	{
		if (activeMarkerIdx < 0) activeMarkerIdx = 0;

		while (activeMarkerIdx < (int) markers.size() && getMarkers()[activeMarkerIdx]->getStatus2D()[camera][activeFrame] != UNDEFINED)
		{
			activeMarkerIdx++;
		}
		if (activeMarkerIdx == markers.size()) addMarker();
	}
}

RigidBody* Trial::getActiveRB()
{
	if (activeBodyIdx >= (int) rigidBodies.size() || activeBodyIdx < 0) return NULL;

	return rigidBodies[activeBodyIdx];
}

void Trial::addRigidBody()
{
	RigidBody* rb = new RigidBody(nbImages, this);
	rigidBodies.push_back(rb);
}


void Trial::removeRigidBody(int idx)
{
	delete rigidBodies[idx];
	rigidBodies.erase(std::remove(rigidBodies.begin(), rigidBodies.end(), rigidBodies[idx]), rigidBodies.end());
	if (activeBodyIdx >= (int) rigidBodies.size())activeBodyIdx = rigidBodies.size() - 1;
}

void Trial::addMarker()
{
	markers.push_back(new Marker(Project::getInstance()->getCameras().size(), nbImages, this));
	setActiveMarkerIdx(markers.size() - 1);
}

void Trial::removeMarker(int idx)
{
	delete markers[idx];
	markers.erase(std::remove(markers.begin(), markers.end(), markers[idx]), markers.end());
	if (activeMarkerIdx >= (int) markers.size())setActiveMarkerIdx(markers.size() - 1);

	//Update RigidBodies
	for (std::vector<RigidBody*>::const_iterator it = rigidBodies.begin(); it < rigidBodies.end(); ++it)
	{
		(*it)->removePointIdx(idx);
		(*it)->updatePointIdx(idx);
	}
}

void Trial::addEvent(QString name, QColor color)
{
	events.push_back(new EventData(name, color));
	events.back()->init(nbImages);
}

void Trial::removeEvent(int idx)
{
	delete events[idx];
	events.erase(std::remove(events.begin(), events.end(), events[idx]), events.end());
}

int Trial::getActiveMarkerIdx()
{
	return activeMarkerIdx;
}

void Trial::setActiveMarkerIdx(int _activeMarker)
{
	activeMarkerIdx = _activeMarker;
}

int Trial::getActiveRBIdx()
{
	return activeBodyIdx;
}

void Trial::setActiveRBIdx(int _activeBody)
{
	activeBodyIdx = _activeBody;
}

int Trial::getReferenceCalibrationImage()
{
	return referenceCalibrationImage;
}

void Trial::setReferenceCalibrationImage(int value)
{
	referenceCalibrationImage = value;
}

double Trial::getRecordingSpeed()
{
	return recordingSpeed;
}

void Trial::setRecordingSpeed(double value)
{
	recordingSpeed = value;
}

double Trial::getCutoffFrequency()
{
	return cutoffFrequency;
}

void Trial::setCutoffFrequency(double value)
{
	cutoffFrequency = value;
}

QString Trial::getActiveFilename(int camera)
{
	if (isDefault)
		return "";

	return videos[camera]->getFrameName(activeFrame);
}

int Trial::getNbImages()
{
	return nbImages;
}

QString Trial::getName()
{
	return name;
}

void Trial::drawRigidBodies(Camera* cam)
{
	for (std::vector<RigidBody *>::const_iterator it = rigidBodies.begin(); it != rigidBodies.end(); ++it)
	{
		(*it)->draw2D(cam, activeFrame);
	}
}

void Trial::drawRigidBodiesMesh()
{
	for (std::vector<RigidBody *>::const_iterator it = rigidBodies.begin(); it != rigidBodies.end(); ++it)
	{
		if((*it)->getDrawMeshModel())
			(*it)->drawMesh(activeFrame);
	}
}

bool Trial::renderMeshes()
{
	bool render = false;
	for (std::vector<RigidBody *>::const_iterator it = rigidBodies.begin(); it != rigidBodies.end(); ++it)
	{
		render = (*it)->getDrawMeshModel() || render;
	}
	return render;
}

void Trial::drawPoints(int cameraId, bool detailView)
{
	int idx = 0;
	if (Settings::getInstance()->getBoolSetting("TrialDrawMarkers")){
		if (!detailView)
		{
			glBegin(GL_LINES);
			for (std::vector<Marker *>::const_iterator it = markers.begin(); it != markers.end(); ++it)
			{
				if (((*it)->getStatus2D()[cameraId][activeFrame] > 0))
				{
					if (Settings::getInstance()->getBoolSetting("ShowColoredMarkerCross"))
					{
						QColor color = (*it)->getStatusColor(cameraId, activeFrame);
						glColor3f(color.redF(), color.greenF(), color.blueF());
					}
					else{
						if (idx == activeMarkerIdx)
						{
							glColor3f(1.0, 0.0, 0.0);
						}
						else
						{
							glColor3f(0.0, 1.0, 0.0);
						}
					}

					glVertex2f((*it)->getPoints2D()[cameraId][activeFrame].x - 5, (*it)->getPoints2D()[cameraId][activeFrame].y);
					glVertex2f((*it)->getPoints2D()[cameraId][activeFrame].x + 5, (*it)->getPoints2D()[cameraId][activeFrame].y);
					glVertex2f((*it)->getPoints2D()[cameraId][activeFrame].x, (*it)->getPoints2D()[cameraId][activeFrame].y - 5);
					glVertex2f((*it)->getPoints2D()[cameraId][activeFrame].x, (*it)->getPoints2D()[cameraId][activeFrame].y + 5);
				}
				idx++;
			}
			glEnd();
		}
		else if (activeMarkerIdx >= 0 && activeMarkerIdx < (int)markers.size())
		{
			double x = markers[activeMarkerIdx]->getPoints2D()[cameraId][activeFrame].x;
			double y = markers[activeMarkerIdx]->getPoints2D()[cameraId][activeFrame].y;

			glBegin(GL_LINES);
			glColor3f(1.0, 0.0, 0.0);
			glVertex2f(x - 12, y);
			glVertex2f(x + 12, y);
			glVertex2f(x, y - 12);
			glVertex2f(x, y + 12);
			glEnd();

			if (Settings::getInstance()->getBoolSetting("AdvancedCrosshairDetailView"))
			{
				glBegin(GL_LINES);
				for (int i = 0; i < 6; i++)
				{
					glVertex2f(x - 1, y + i * 2);
					glVertex2f(x + 1, y + i * 2);

					glVertex2f(x - 1, y - i * 2);
					glVertex2f(x + 1, y - i * 2);

					glVertex2f(x + i * 2, y - 1);
					glVertex2f(x + i * 2, y + 1);

					glVertex2f(x - i * 2, y - 1);
					glVertex2f(x - i * 2, y + 1);
				}
				glEnd();
				double size = markers[activeMarkerIdx]->getSize();;
				if (size > 0)
				{
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glBegin(GL_LINES);
					glColor4f(1.0f, 0.0f, 0.0f, 0.3f);
					glVertex2f(x - size, y - size);
					glVertex2f(x - size, y + size);

					glVertex2f(x + size, y - size);
					glVertex2f(x + size, y + size);

					glVertex2f(x - size, y - size);
					glVertex2f(x + size, y - size);

					glVertex2f(x - size, y + size);
					glVertex2f(x + size, y + size);
					glEnd();
					glDisable(GL_BLEND);
				}
			}
		}

		if (activeMarkerIdx >= 0 && activeMarkerIdx < (int)markers.size() && markers[activeMarkerIdx]->getStatus3D()[activeFrame] > 0)
		{
			if (!detailView || Settings::getInstance()->getBoolSetting("Show3dPointDetailView"))
			{
				glBegin(GL_LINES);
				glColor3f(0.0, 1.0, 1.0);
				glVertex2f(markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].x - 5, markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].y - 5);
				glVertex2f(markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].x + 5, markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].y + 5);
				glVertex2f(markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].x + 5, markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].y - 5);
				glVertex2f(markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].x - 5, markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].y + 5);
				glEnd();
			}
		}
	}
	if (Settings::getInstance()->getBoolSetting("TrialDrawEpipolar") &&
		(!detailView || Settings::getInstance()->getBoolSetting("ShowEpiLineDetailView")))
	{
		for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
		{
		
			if (activeMarkerIdx >= 0 && activeMarkerIdx < (int) markers.size() && markers[activeMarkerIdx]->getStatus2D()[i][activeFrame] > 0)
			{
				if (cameraId != i)
				{
					std::vector<cv::Point2d> epiline = markers[activeMarkerIdx]->getEpipolarLine(i, cameraId, activeFrame);
					glBegin(GL_LINE_STRIP);
					glColor3f(0.0, 0.0, 1.0);
					for (std::vector<cv::Point2d>::const_iterator pt = epiline.begin(); pt != epiline.end(); ++pt)
					{
						glVertex2f(pt->x, pt->y);
					}
					epiline.clear();
					glEnd();
				}
			}
		}
	}
}

int Trial::getStartFrame()
{
	return startFrame;
}

void Trial::setStartFrame(int value)
{
	startFrame = value;
}

int Trial::getEndFrame()
{
	return endFrame;
}

void Trial::setEndFrame(int value)
{
	endFrame = value;
}

bool Trial::getRequiresRecomputation()
{
	return requiresRecomputation;
}

void Trial::setRequiresRecomputation(bool value)
{
	requiresRecomputation = value;

	for (unsigned int i = 0; i < getMarkers().size(); i++)
	{
		getMarkers()[i]->setRequiresRecomputation(requiresRecomputation);
	}
}

void Trial::loadMarkersFromCSV(QString filename, bool updateOnly)
{
	QString tmp_names;
	QString tmp_coords;

	std::ifstream fin;
	fin.open(filename.toAscii().data());
	std::istringstream in;
	std::string line;
	littleHelper::safeGetline(fin, line);
	tmp_names = QString::fromStdString(line);
	littleHelper::safeGetline(fin, line);
	tmp_coords = QString::fromStdString(line);
	fin.close();

	QStringList names_list = tmp_names.split(",");
	QStringList coords_list = tmp_coords.split(",");

	std::vector<cv::Point3d> points3D_tmp;
	std::vector<QString> referenceNames_tmp;

	for (int i = 0; i < names_list.size() / 3; i++)
	{
		referenceNames_tmp.push_back(names_list.at(3 * i));
		referenceNames_tmp[i].replace("_x", "");
		points3D_tmp.push_back(cv::Point3d(coords_list.at(3 * i).toDouble(), coords_list.at(3 * i + 1).toDouble(), coords_list.at(3 * i + 2).toDouble()));
	}

	for (unsigned int i = 0; i < points3D_tmp.size(); i++)
	{
		if (!updateOnly){
			addMarker();
			markers[markers.size() - 1]->setDescription(referenceNames_tmp[i]);
			markers[markers.size() - 1]->setReference3DPoint(points3D_tmp[i].x, points3D_tmp[i].y, points3D_tmp[i].z);
		}
		else
		{
			if (markers.size() <= i) addMarker();
			markers[i]->setDescription(referenceNames_tmp[i]);
			markers[i]->setReference3DPoint(points3D_tmp[i].x, points3D_tmp[i].y, points3D_tmp[i].z);
		}
	}

	if (updateOnly){
		for (unsigned int i = 0; i < getRigidBodies().size(); i++)
		{
			if (getRigidBodies()[i]->isReferencesSet() == 1){
				getRigidBodies()[i]->setReferenceMarkerReferences();
				getRigidBodies()[i]->recomputeTransformations();
				getRigidBodies()[i]->filterTransformations();
			}
		}
	}
}


void Trial::loadMarkers(QString filename)
{
	std::ifstream fin;
	fin.open(filename.toAscii().data());
	std::string line;
	unsigned int count = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		if (count >= this->getMarkers().size())
			this->addMarker();

		this->getMarkers()[count]->setDescription(QString::fromStdString(line));
		count++;
		line.clear();
	}
	fin.close();
}

void Trial::loadRigidBodies(QString filename)
{
	std::ifstream fin;
	fin.open(filename.toAscii().data());
	std::istringstream in;
	std::string line;
	std::string desc;
	std::string indices;
	int count = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		if (!line.empty())
		{
			if (count >= (int) this->getRigidBodies().size())
			{
				this->addRigidBody();
			}
			desc = line.substr(0, line.find('['));
			indices = line.substr(line.find('[') + 1, line.find(']') - 1);
			this->getRigidBodies()[count]->setDescription(QString::fromStdString(desc));

			this->getRigidBodies()[count]->clearPointIdx();
			in.clear();
			in.str(indices);
			for (int value; in >> value; littleHelper::comma(in))
			{
				this->getRigidBodies()[count]->addPointIdx(value - 1, false);
			}
			count++;
			line.clear();
			desc.clear();
			indices.clear();
		}
	}
	fin.close();
}

void Trial::saveMarkers(QString filename)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	for (unsigned int i = 0; i < this->getMarkers().size(); i++)
	{
		outfile << this->getMarkers()[i]->getDescription().toAscii().data() << std::endl;
	}
	outfile.close();
}

void Trial::saveRigidBodies(QString filename)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	for (unsigned int i = 0; i < this->getRigidBodies().size(); i++)
	{
		outfile << this->getRigidBodies()[i]->getDescription().toAscii().data() << "[";
		for (unsigned int k = 0; k < this->getRigidBodies()[i]->getPointsIdx().size(); k++)
		{
			outfile << this->getRigidBodies()[i]->getPointsIdx()[k] + 1;
			if (k != (this->getRigidBodies()[i]->getPointsIdx().size() - 1)) outfile << " , ";
		}
		outfile << "]" << std::endl;
	}
	outfile.close();
}

void Trial::changeImagePath(int camera, QString newfolder, QString oldfolder)
{
	if (isDefault)
		return;

	videos[camera]->changeImagePath(newfolder, oldfolder);
}

void Trial::updateAfterChangeImagePath()
{
	setNbImages();
}

void Trial::recomputeAndFilterRigidBodyTransformations()
{
	for (unsigned int i = 0; i < getRigidBodies().size(); i++)
	{
		getRigidBodies()[i]->recomputeTransformations();

		getRigidBodies()[i]->filterTransformations();
	}
}

void Trial::saveRigidBodyTransformations(std::vector<int> _bodies, QString outputfolder, bool onefile, bool headerRow, bool filtered, bool saveColumn, int start, int stop)
{
	for (std::vector<int>::const_iterator it = _bodies.begin(); it < _bodies.end(); ++it)
	{
		getRigidBodies()[*it]->recomputeTransformations();
		if (filtered)getRigidBodies()[*it]->filterTransformations();
	}

	if (onefile)
	{
		std::ofstream outfile(outputfolder.toAscii().data());
		outfile.precision(12);
		if (headerRow)
		{
			if (saveColumn)
			{
				outfile << "Frame" << " , ";
			}
			for (std::vector<int>::const_iterator it = _bodies.begin(); it < _bodies.end(); ++it)
			{
				QString name;
				QString filterRate = "";
				if (getRigidBodies()[*it]->getDescription().isEmpty())
				{
					name = "RigidBody" + QString().sprintf("%03d", *it + 1);
				}
				else
				{
					name = getRigidBodies()[*it]->getDescription();
				}

				if (filtered)
				{
					filterRate = "_" + QString::number(getRigidBodies()[*it]->getOverrideCutoffFrequency() ? getRigidBodies()[*it]->getCutoffFrequency() : getCutoffFrequency()) + "Hz";
				}

				outfile << name.toAscii().data() << "_R11" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R12" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R13" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_01" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R21" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R22" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R23" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_02" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R31" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R32" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R33" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_03" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_TX" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_TY" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_TY" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_1" << filterRate.toAscii().data();

				if (it != _bodies.end() - 1)
				{
					outfile << " , ";
				}
				else
				{
					outfile << std::endl;
				}
			}
		}
		double trans[16];
		if (!saveColumn)
		{
			start = 0;
			stop = nbImages - 1;
		}
		for (int f = start; f < stop + 1; f++)
		{
			if (saveColumn)
			{
				outfile << f + 1 << " , ";
			}
			for (std::vector<int>::const_iterator it = _bodies.begin(); it < _bodies.end(); ++it)
			{
				if (getRigidBodies()[*it]->getTransformationMatrix(f, filtered, &trans[0]))
				{
					outfile << trans[0] << " , " << trans[1] << " , " << trans[2] << " , " << trans[3] << " , ";
					outfile << trans[4] << " , " << trans[5] << " , " << trans[6] << " , " << trans[7] << " , ";
					outfile << trans[8] << " , " << trans[9] << " , " << trans[10] << " , " << trans[11] << " , ";
					outfile << trans[12] << " , " << trans[13] << " , " << trans[14] << " , " << trans[15];
				}
				else
				{
					outfile << "NaN , NaN , NaN , NaN , ";
					outfile << "NaN , NaN , NaN , NaN , ";
					outfile << "NaN , NaN , NaN , NaN , ";
					outfile << "NaN , NaN , NaN , NaN";
				}

				if (it != _bodies.end() - 1)
				{
					outfile << " , ";
				}
				else
				{
					outfile << std::endl;
				}
			}
		}
		
		outfile.close();
	}
	else
	{
		for (std::vector<int>::const_iterator it = _bodies.begin(); it < _bodies.end(); ++it)
		{
			QString filename;
			if (filtered)
			{
				filename = outputfolder + "RigidBody" + QString().sprintf("%03d", *it + 1) + "_" + getRigidBodies()[*it]->getDescription() + "_transformationFiltered_" + QString::number(getRigidBodies()[*it]->getOverrideCutoffFrequency() ? getRigidBodies()[*it]->getCutoffFrequency() : getCutoffFrequency()) + "Hz.csv";
			}
			else
			{
				filename = outputfolder + "RigidBody" + QString().sprintf("%03d", *it + 1) + "_" + getRigidBodies()[*it]->getDescription() + "_transformation.csv";
			}

			std::ofstream outfile(filename.toAscii().data());
			outfile.precision(12);
			if (headerRow)
			{
				if (saveColumn)
				{
					outfile << "Frame" << " , ";
				}
				QString name;
				QString filterRate = "";
				if (getRigidBodies()[*it]->getDescription().isEmpty())
				{
					name = "RigidBody" + QString().sprintf("%03d", *it + 1);
				}
				else
				{
					name = getRigidBodies()[*it]->getDescription();
				}

				if (filtered)
				{
					filterRate = "_" + QString::number(getRigidBodies()[*it]->getOverrideCutoffFrequency() ? getRigidBodies()[*it]->getCutoffFrequency() : getCutoffFrequency()) + "Hz";
				}

				outfile << name.toAscii().data() << "_R11" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R12" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R13" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_01" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R21" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R22" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R23" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_02" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R31" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R32" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_R33" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_03" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_TX" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_TY" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_TY" << filterRate.toAscii().data() << " , "
					<< name.toAscii().data() << "_1" << filterRate.toAscii().data();

				outfile << std::endl;
			}

			double trans[16];
			if (!saveColumn)
			{
				start = 0;
				stop = nbImages - 1;
			}
			for (int f = start; f < stop + 1; f++)
			{
				if (saveColumn)
				{
					outfile << f + 1 << " , ";
				}
				if (getRigidBodies()[*it]->getTransformationMatrix(f, filtered, &trans[0]))
				{
					outfile << trans[0] << " , " << trans[1] << " , " << trans[2] << " , " << trans[3] << " , ";
					outfile << trans[4] << " , " << trans[5] << " , " << trans[6] << " , " << trans[7] << " , ";
					outfile << trans[8] << " , " << trans[9] << " , " << trans[10] << " , " << trans[11] << " , ";
					outfile << trans[12] << " , " << trans[13] << " , " << trans[14] << " , " << trans[15];
				}
				else
				{
					outfile << "NaN , NaN , NaN , NaN , ";
					outfile << "NaN , NaN , NaN , NaN , ";
					outfile << "NaN , NaN , NaN , NaN , ";
					outfile << "NaN , NaN , NaN , NaN";
				}

				outfile << std::endl;
			}
			outfile.close();
		}
	}
}


void Trial::saveTrialImages(QString outputfolder, int from, int to, QString format, int id)
{
	if (isDefault)
		return;
	
	int start = (id == -1) ?  0 : id;
	int end = (id == -1) ? videos.size() : id + 1;
	for (unsigned int i = start; i < end; i++)
	{
		QFileInfo info(videos[i]->getFileBasename());
		QString foldername = (id == -1) ? outputfolder + info.completeBaseName() + "UND" : outputfolder;
		if (!QDir().mkpath(foldername))
		{
			return;
		}

		if (format == "avi")
		{
			cv::VideoWriter outputVideo;
			QString outname = foldername + OS_SEP + info.completeBaseName() + "." + format;
			outputVideo.open(outname.toAscii().data(), -1, (getRecordingSpeed() <= 0) ? 30 : getRecordingSpeed(), cv::Size(Project::getInstance()->getCameras()[i]->getWidth(), Project::getInstance()->getCameras()[i]->getHeight()), true);

			if (outputVideo.isOpened())
			{

				if (Project::getInstance()->getCameras()[i]->hasUndistortion())
				{
					for (int j = from - 1; j < to; j++)
					{
						videos[i]->setActiveFrame(j);
						Image * tmp = new Image(videos[i]->getImage());
						Project::getInstance()->getCameras()[i]->getUndistortionObject()->undistort(videos[i]->getImage(), tmp);
						cv::Mat tmpMat;
						tmp->getImage(tmpMat);
						if (tmpMat.channels() > 1)
						{
							outputVideo << tmpMat;
						} 
						else
						{
							cv::Mat tmpMat2;
							cvtColor(tmpMat, tmpMat2, CV_GRAY2RGB);
							outputVideo << tmpMat2;
							tmpMat2.release();
						}
						
						delete tmp;
						tmpMat.release();
					}
				}
				else
				{
					for (int j = from - 1; j < to; j++)
					{
						videos[i]->setActiveFrame(j);
						cv::Mat tmpMat;
						videos[i]->getImage()->getImage(tmpMat, true);
						if (tmpMat.channels() > 1)
						{
							outputVideo << tmpMat;
						}
						else
						{
							cv::Mat tmpMat2;
							cvtColor(tmpMat, tmpMat2, CV_GRAY2RGB);
							outputVideo << tmpMat2;
							tmpMat2.release();
						}
						tmpMat.release();
					}
				}
			}
		}
		else{
			if (Project::getInstance()->getCameras()[i]->hasUndistortion())
			{
				for (int j = from - 1; j < to; j++)
				{
					QString outname = foldername + OS_SEP + info.completeBaseName() + "_UND." + QString("%1").arg(j + 1, 4, 10, QChar('0')) + "." + format;
					videos[i]->setActiveFrame(j);
					Project::getInstance()->getCameras()[i]->getUndistortionObject()->undistort(videos[i]->getImage(), outname);
				}
			}
			else
			{
				for (int j = from - 1; j < to; j++)
				{
					QString outname = foldername + OS_SEP + info.completeBaseName() + "." + QString("%1").arg(j + 1, 4, 10, QChar('0')) + "." + format;
					videos[i]->setActiveFrame(j);
					videos[i]->getImage()->save(outname);
				}
			}
		}
	}
}

void Trial::saveMarkerToMarkerDistances(QString filename, int from, int to)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	outfile << "Mean";
	for (unsigned int i = 0; i < markers.size(); i++)
	{
		outfile << " , " << "Marker " << (i + 1) << " " << markers[i]->getDescription().toAscii().data() ;
	}
	outfile << std::endl;

	for (unsigned int i = 0; i < markers.size(); i++)
	{
		outfile << "Marker " << (i + 1) << " " << markers[i]->getDescription().toAscii().data() << " , ";

		for (unsigned int j = 0; j < markers.size(); j++)
		{
			double mean = 0;
			std::vector<double> value;
			for (int frame = from-1; frame < to; frame++)
			{
				if (markers[i]->getStatus3D()[frame] > UNDEFINED && markers[j]->getStatus3D()[frame] > UNDEFINED)
				{
					cv::Point3d diff = markers[i]->getPoints3D()[frame] - markers[j]->getPoints3D()[frame];
					value.push_back(cv::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z));
				}
			}

			for (unsigned int k = 0; k < value.size(); k++)
			{
				mean += value[k];
			}
			if (value.size() > 0) mean = mean / value.size();

			outfile << mean ;
			if (j != markers.size() - 1) outfile << " , ";
		}
		outfile << std::endl;
	}

	outfile << std::endl;

	outfile << "SD";
	for (unsigned int i = 0; i < markers.size(); i++)
	{
		outfile << " , " << "Marker " << (i + 1) << " " << markers[i]->getDescription().toAscii().data();
	}
	outfile << std::endl;

	for (unsigned int i = 0; i < markers.size(); i++)
	{
		outfile << "Marker " << (i + 1) << " " << markers[i]->getDescription().toAscii().data() << " , ";

		for (unsigned int j = 0; j < markers.size(); j++)
		{
			double sd = 0;
			double mean = 0;
			std::vector<double> value;
			for (int frame = from - 1; frame < to; frame++)
			{
				if (markers[i]->getStatus3D()[frame] > UNDEFINED && markers[j]->getStatus3D()[frame] > UNDEFINED)
				{
					cv::Point3d diff = markers[i]->getPoints3D()[frame] - markers[j]->getPoints3D()[frame];
					value.push_back(cv::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z));
				}
			}

			for (unsigned int k = 0; k < value.size(); k++)
			{
				mean += value[k];
			}
			if (value.size() > 0) mean = mean / value.size();

			for (unsigned int k = 0; k < value.size(); k++)
			{
				sd += pow(value[k] - mean, 2);
			}
			if (value.size() > 1)sd = sqrt(sd / (value.size() - 1));

			outfile << sd;
			if (j != markers.size() - 1) outfile << " , ";
		}
		outfile << std::endl;
	}



	for (unsigned int body = 0; body < rigidBodies.size(); body++)
	{
		outfile << std::endl;
		outfile << std::endl;

		outfile << rigidBodies[body]->getDescription().toAscii().data() << " Mean";
		
		for (unsigned int i = 0; i < rigidBodies[body]->getPointsIdx().size(); i++)
		{
			outfile << " , " << "Marker " << (rigidBodies[body]->getPointsIdx()[i] + 1) << " " << markers[rigidBodies[body]->getPointsIdx()[i]]->getDescription().toAscii().data();
		}
		outfile << std::endl;

		for (unsigned int i = 0; i < rigidBodies[body]->getPointsIdx().size(); i++)
		{
			outfile << "Marker " << (rigidBodies[body]->getPointsIdx()[i] + 1) << " " << markers[rigidBodies[body]->getPointsIdx()[i]]->getDescription().toAscii().data() << " , ";

			for (unsigned int j = 0; j < rigidBodies[body]->getPointsIdx().size(); j++)
			{
				double mean = 0;
				std::vector<double> value;
				for (int frame = from - 1; frame < to; frame++)
				{
					if (markers[rigidBodies[body]->getPointsIdx()[i]]->getStatus3D()[frame] > UNDEFINED && markers[rigidBodies[body]->getPointsIdx()[j]]->getStatus3D()[frame] > UNDEFINED)
					{
						cv::Point3d diff = markers[rigidBodies[body]->getPointsIdx()[i]]->getPoints3D()[frame] - markers[rigidBodies[body]->getPointsIdx()[j]]->getPoints3D()[frame];
						value.push_back(cv::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z));
					}
				}

				for (unsigned int k = 0; k < value.size(); k++)
				{
					mean += value[k];
				}
				if (value.size() > 0) mean = mean / value.size();

				outfile << mean;
				if (j != rigidBodies[body]->getPointsIdx().size() - 1) outfile << " , ";
			}
			outfile << std::endl;
		}

		outfile << std::endl;

		outfile << rigidBodies[body]->getDescription().toAscii().data() << " SD";
		for (unsigned int i = 0; i < rigidBodies[body]->getPointsIdx().size(); i++)
		{
			outfile << " , " << "Marker " << (rigidBodies[body]->getPointsIdx()[i] + 1) << " " << markers[rigidBodies[body]->getPointsIdx()[i]]->getDescription().toAscii().data();
		}
		outfile << std::endl;

		for (unsigned int i = 0; i < rigidBodies[body]->getPointsIdx().size(); i++)
		{
			outfile << "Marker " << (rigidBodies[body]->getPointsIdx()[i] + 1) << " " << markers[rigidBodies[body]->getPointsIdx()[i]]->getDescription().toAscii().data() << " , ";

			for (unsigned int j = 0; j < rigidBodies[body]->getPointsIdx().size(); j++)
			{
				double sd = 0;
				double mean = 0;
				std::vector<double> value;
				for (int frame = from - 1; frame < to; frame++)
				{
					if (markers[rigidBodies[body]->getPointsIdx()[i]]->getStatus3D()[frame] > UNDEFINED && markers[rigidBodies[body]->getPointsIdx()[j]]->getStatus3D()[frame] > UNDEFINED)
					{
						cv::Point3d diff = markers[rigidBodies[body]->getPointsIdx()[i]]->getPoints3D()[frame] - markers[rigidBodies[body]->getPointsIdx()[j]]->getPoints3D()[frame];
						value.push_back(cv::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z));
					}
				}

				for (unsigned int k = 0; k < value.size(); k++)
				{
					mean += value[k];
				}
				if (value.size() > 0) mean = mean / value.size();

				for (unsigned int k = 0; k < value.size(); k++)
				{
					sd += pow(value[k] - mean, 2);
				}
				if (value.size() > 1)sd = sqrt(sd / (value.size() - 1));

				outfile << sd;
				if (j != rigidBodies[body]->getPointsIdx().size() - 1) outfile << " , ";
			}
			outfile << std::endl;
		}
	}
}

void Trial::savePrecisionInfo(QString filename)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	outfile << "RigidBodies , nb tracked frames , marker to marker sd , error 3d Unfiltered , error 3D filtered" << std::endl;
	for (auto rb : rigidBodies)
	{
		double averageSD;
		int count;
		rb->getMarkerToMarkerSD(averageSD, count);
		double val1 = rb->getError3D(false);
		double val2 = rb->getError3D(true);
		outfile << rb->getDescription().toStdString() << " , " << rb->getFramesTracked() << " , " << averageSD << " , " << val1 << " , " << val2 << std::endl;
	}
	outfile << std::endl << std::endl << "Markers , nb tracked frames , reprojectionerror , sd" << std::endl;
	for (auto marker : markers)
	{
		double sd;
		double val = marker->getReprojectionError(&sd);
		outfile << marker->getDescription().toStdString() << " , " << marker->getFramesTracked() << " , " << val << " , " << sd << std::endl;
	}
}

void Trial::resetRigidBodyByMarker(Marker* marker, int frame)
{
	for (unsigned int i = 0; i < getRigidBodies().size(); i++)
	{
		for (unsigned int j = 0; j < getRigidBodies()[i]->getPointsIdx().size(); j++)
		{
			if (getRigidBodies()[i]->getPointsIdx()[j] < (int) getMarkers().size() && getMarkers()[getRigidBodies()[i]->getPointsIdx()[j]] == marker)
			{
				getRigidBodies()[i]->computePose(frame);
				return;
			}
		}
	}
}

bool Trial::save3dPoints(std::vector<int> _markers, QString outputfolder, bool onefile, bool headerRow, double filterFrequency, bool saveColumn, int start, int stop)
{
	//create TmpData
	std::vector<std::vector <cv::Point3d> > points3D;
	std::vector<std::vector <markerStatus> > status3D;
	for (std::vector<int>::const_iterator it = _markers.begin(); it < _markers.end(); ++it)
	{
		std::vector <cv::Point3d> marker;
		std::vector <markerStatus> status;
		//filter Data
		if (filterFrequency > 0.0)
		{
			if (!getMarkers()[*it]->filterMarker(filterFrequency, marker, status))
			{
				return false;
			}
		}
		//Copy Data
		else{
			for (int f = 0; f < nbImages; f++)
			{
				marker.push_back(cv::Point3d(getMarkers()[*it]->getPoints3D()[f]));
				status.push_back(getMarkers()[*it]->getStatus3D()[f]);
			}
		}
		points3D.push_back(marker);
		status3D.push_back(status);
	}

	//Write to File
	if (onefile)
	{
		std::ofstream outfile(outputfolder.toAscii().data());
		outfile.precision(12);
		if (headerRow)
		{
			if (saveColumn)
			{
				outfile << "Frame" << " , ";
			}
			for (std::vector<int>::const_iterator it = _markers.begin(); it < _markers.end(); ++it)
			{
				QString name;
				if (getMarkers()[*it]->getDescription().isEmpty())
				{
					name = "marker" + QString().sprintf("%03d", *it + 1);
				}
				else
				{
					name = getMarkers()[*it]->getDescription();
				}

				if (filterFrequency > 0.0)
				{
					name = name + "_" + QString::number(filterFrequency) + "Hz";
				}

				outfile << name.toAscii().data() << "_X" << " , " << name.toAscii().data() << "_Y" << " , " << name.toAscii().data() << "_Z";

				if (it != _markers.end() - 1)
				{
					outfile << " , ";
				}
				else
				{
					outfile << std::endl;
				}
			}
		}
		if (!saveColumn)
		{
			start = 0;
			stop = nbImages - 1;
		}
		for (int f = start; f < stop+1; f++)
		{
			if (saveColumn)
			{
				outfile << f + 1 << " , ";
			}
			for (unsigned int i = 0; i < points3D.size(); i++)
			{
				if (status3D[i][f] <= 0)
				{
					outfile << "NaN" << " , " << "NaN" << " , " << "NaN";
				}
				else
				{
					outfile << points3D[i][f].x << " , " << points3D[i][f].y << " , " << points3D[i][f].z;
				}

				if (i != points3D.size() - 1)
				{
					outfile << " , ";
				}
				else
				{
					outfile << std::endl;
				}
			}
		}
		outfile.close();
	}
	else
	{
		int count = 0;
		for (std::vector<int>::const_iterator it = _markers.begin(); it < _markers.end(); ++it)
		{
			QString filename = outputfolder + "Marker" + QString().sprintf("%03d", *it + 1) + "_" + getMarkers()[*it]->getDescription() + "_points3d";
			if (filterFrequency > 0.0)
			{
				filename = filename + "_" + QString::number(filterFrequency) + "Hz";
			}
			filename = filename + ".csv";

			std::ofstream outfile(filename.toAscii().data());
			outfile.precision(12);
			if (headerRow)
			{
				if (saveColumn)
				{
					outfile << "Frame" << " , ";
				}
				QString name;
				if (getMarkers()[*it]->getDescription().isEmpty())
				{
					name = "marker" + QString().sprintf("%03d", *it + 1);
				}
				else
				{
					name = getMarkers()[*it]->getDescription();
				}
				if (filterFrequency > 0.0)
				{
					name = name + "_" + QString::number(filterFrequency) + "Hz";
				}
				outfile << name.toAscii().data() << "_X" << " , " << name.toAscii().data() << "_Y" << " , " << name.toAscii().data() << "_Z";

				outfile << std::endl;
			}
			if (!saveColumn)
			{
				start = 0;
				stop = nbImages - 1;
			}
			for (int f = start; f < stop + 1; f++)
			{
				if (saveColumn)
				{
					outfile << f + 1 << " , ";
				}
				if (status3D[count][f] <= 0)
				{
					outfile << "NaN" << " , " << "NaN" << " , " << "NaN";
				}
				else
				{
					outfile << points3D[count][f].x << " , " << points3D[count][f].y << " , " << points3D[count][f].z;
				}

				outfile << std::endl;
			}
			count++;
			outfile.close();
		}
	}

	//clear tmpData
	for (unsigned int i = 0; i < points3D.size(); i++)
	{
		points3D[i].clear();
		status3D[i].clear();
	}
	points3D.clear();
	status3D.clear();

	return true;
}

void Trial::save2dPoints(QString outputfolder, bool onefile, bool distorted, bool offset1, bool yinvert, bool headerRow, bool offsetCols, int id)
{
	int start = (id == -1) ? 0 : id;
	int end = (id == -1) ? Project::getInstance()->getCameras().size() : id + 1;

	if (onefile)
	{
		std::ofstream outfile(outputfolder.toAscii().data());
		outfile.precision(12);
		if (headerRow)
		{
			if (offsetCols)
			{
				for (unsigned int j = start; j < end; j++)
				{
					outfile << "cam" << j << "_offset";
				}
			}

			for (unsigned int i = 0; i < getMarkers().size(); i++)
			{
				QString name;
				if (getMarkers()[i]->getDescription().isEmpty())
				{
					name = "marker" + QString().sprintf("%03d", i + 1);
				}
				else
				{
					name = getMarkers()[i]->getDescription();
				}

				for (unsigned int j = start; j < end; j++)
				{
					outfile << name.toAscii().data() << "_cam" << j + 1 << "_X" << " , " << name.toAscii().data() << "_cam" << j + 1 << "_Y";

					if (i != getMarkers().size() - 1 || j != end- 1)
					{
						outfile << " , ";
					}
					else
					{
						outfile << std::endl;
					}
				}
			}
		}

		for (int f = 0; f < nbImages; f++)
		{
			if (offsetCols)
			{
				for (unsigned int j = start; j < end; j++)
				{
					outfile << 0.0 << " , ";
				}
			}
			for (unsigned int i = 0; i < getMarkers().size(); i++)
			{
				for (unsigned int j = start; j < end; j++)
				{
					if (getMarkers()[i]->getStatus2D()[j][f] > 0)
					{
						double x;
						double y;
						if (distorted)
						{
							x = getMarkers()[i]->getPoints2D()[j][f].x;
							y = getMarkers()[i]->getPoints2D()[j][f].y;
						}
						else
						{
							cv::Point2d pt = Project::getInstance()->getCameras()[j]->undistortPoint(getMarkers()[i]->getPoints2D()[j][f], true);
							x = pt.x;
							y = pt.y;
						}

						if (yinvert)
						{
							y = Project::getInstance()->getCameras()[j]->getHeight() - y - 1;
						}

						if (offset1)
						{
							y += 1;
							x += 1;
						}
						outfile << x << " , " << y;
					}
					else
					{
						outfile << "NaN" << " , " << "NaN";
					}

					if (i != getMarkers().size() - 1 || j != end - 1)
					{
						outfile << " , ";
					}
					else
					{
						outfile << std::endl;
					}
				}
			}
		}
		outfile.close();
	}
	else
	{
		for (unsigned int i = 0; i < getMarkers().size(); i++)
		{
			QString filename = outputfolder + "Marker" + QString().sprintf("%03d", i + 1) + "_" + getMarkers()[i]->getDescription() + "_points2d.csv";
			std::ofstream outfile(filename.toAscii().data());
			outfile.precision(12);
			if (headerRow)
			{
				if (offsetCols)
				{
					for (unsigned int j = start; j < end; j++)
					{
						outfile << "cam" << j << "_offset";
					}
				}

				QString name;
				if (getMarkers()[i]->getDescription().isEmpty())
				{
					name = "marker" + QString().sprintf("%03d", i + 1);
				}
				else
				{
					name = getMarkers()[i]->getDescription();
				}

				for (unsigned int j = start; j < end; j++)
				{
					outfile << name.toAscii().data() << "_cam" << j + 1 << "_X" << " , " << name.toAscii().data() << "_cam" << j + 1 << "_Y";

					if (j != end - 1)
					{
						outfile << " , ";
					}
					else
					{
						outfile << std::endl;
					}
				}
			}

			for (int f = 0; f < nbImages; f++)
			{
				if (offsetCols)
				{
					for (unsigned int j = start; j < end; j++)
					{
						outfile << 0.0 << " , ";
					}
				}

				for (unsigned int j = start; j < end; j++)
				{
					if (getMarkers()[i]->getStatus2D()[j][f] > 0)
					{
						double x;
						double y;
						if (distorted)
						{
							x = getMarkers()[i]->getPoints2D()[j][f].x;
							y = getMarkers()[i]->getPoints2D()[j][f].y;
						}
						else
						{
							cv::Point2d pt = Project::getInstance()->getCameras()[j]->undistortPoint(getMarkers()[i]->getPoints2D()[j][f], true);
							x = pt.x;
							y = pt.y;
						}

						if (yinvert)
						{
							y = Project::getInstance()->getCameras()[j]->getHeight() - y - 1;
						}

						if (offset1)
						{
							y += 1;
							x += 1;
						}
						outfile << x << " , " << y;
					}
					else
					{
						outfile << "NaN" << " , " << "NaN";
					}

					if (j != end - 1)
					{
						outfile << " , ";
					}
					else
					{
						outfile << std::endl;
					}
				}
			}

			outfile.close();
		}
	}
}

int Trial::load2dPoints(QString input, bool distorted, bool offset1, bool yinvert, bool headerRow, bool offsetCols)
{
	std::vector<Marker *> newMarkers;

	std::ifstream fin;
	fin.open(input.toAscii().data());
	std::istringstream in;
	std::string line;
	QString tmp_names;
	QStringList list;

	bool firstRun = true;
	bool readHeader = headerRow;
	int frame = 0;
	int offset = offsetCols ? Project::getInstance()->getCameras().size() : 0;

	while (!littleHelper::safeGetline(fin, line).eof())
	{
		if (!line.empty())
		{
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			tmp_names = QString::fromStdString(line);
			list = tmp_names.split(",");

			if (firstRun)
			{
				int nbNewMarker = (list.size() - offset) / 2 / Project::getInstance()->getCameras().size();
				for (int i = 0; i < nbNewMarker; i++)
				{
					addMarker();
					newMarkers.push_back(markers[markers.size() - 1]);
				}
				firstRun = false;
			}

			if (readHeader)
			{
				for (unsigned int i = 0; i < newMarkers.size(); i++)
				{
					QString name = list.at(i * 2 * Project::getInstance()->getCameras().size() + offset);
					name.replace("_cam1_X", "");
					while (name.startsWith(" "))
					{
						name = name.right(name.length() - 1);
					}
					while (name.endsWith(" "))
					{
						name = name.left(name.length() - 1);
					}
					newMarkers[i]->setDescription(name);
				}
				readHeader = false;
			}
			else
			{
				for (unsigned int i = 0; i < newMarkers.size(); i++)
				{
					for (unsigned int j = 0; j < Project::getInstance()->getCameras().size(); j++)
					{
						if (!list.at(i * Project::getInstance()->getCameras().size() * 2 + 2 * j + offset).contains("NaN")
							&& !list.at(i * Project::getInstance()->getCameras().size() * 2 + 2 * j + 1 + offset).contains("NaN"))
						{
							double x = list.at(i * Project::getInstance()->getCameras().size() * 2 + 2 * j + offset).toDouble();
							double y = list.at(i * Project::getInstance()->getCameras().size() * 2 + 2 * j + 1 + offset).toDouble();

							if (offset1)
							{
								x -= 1;
								y -= 1;
							}

							if (yinvert)
							{
								y = Project::getInstance()->getCameras()[j]->getHeight() - y - 1;
							}

							if (!distorted)
							{
								cv::Point2d pt = Project::getInstance()->getCameras()[j]->undistortPoint(cv::Point2d(x,y), false);
								x = pt.x;
								y = pt.y;
							}

							newMarkers[i]->setPoint(j, frame, x, y, SET);
						}
					}
				}
				frame++;
			}
		}
	}

	for (unsigned int i = 0; i < newMarkers.size(); i++)
	{
		newMarkers[i]->update();
	}
	fin.close();

	return newMarkers.size();
}

void Trial::getDrawTextData(int cam, int frame, std::vector<double>& x, std::vector<double>& y, std::vector<QString>& text)
{
	x.clear();
	y.clear();
	text.clear();
	if (frame >= 0)
	{
		for (unsigned int i = 0; i < markers.size(); i++)
		{
			x.push_back(markers[i]->getPoints2D()[cam][frame].x);
			y.push_back(markers[i]->getPoints2D()[cam][frame].y);
			text.push_back(QString::number(i + 1));
		}
	}
}

void Trial::saveXMLData(QString filename)
{
	if (hasStudyData)
	{
		QFile file(filename); //
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream out(&file);

			for (int i = 0; i < xml_data.size(); i++)
			{
				out << xml_data.at(i).toAscii().data() << endl;
			}
		}
		file.close();
	}
}

void Trial::setXMLData(QString filename)
{
	if (!filename.isEmpty()){
		QFile file(filename); // this is a name of a file text1.txt sent from main method
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			hasStudyData = true;

			QTextStream in(&file);
			QString line = in.readLine();
			while (!line.isNull())
			{
				xml_data << line;
				line = in.readLine();
			}
			file.close();
		}
	}
	parseXMLData();
}

bool Trial::setFrameRateFromXML()
{
	if (hasStudyData){
		QXmlStreamReader xml(xml_data.join(""));

		std::vector<double> FrameRates;

		while (!xml.atEnd() && !xml.hasError())
		{
			QXmlStreamReader::TokenType token = xml.readNext();

			if (token == QXmlStreamReader::StartDocument)
			{
				continue;
			}

			if (token == QXmlStreamReader::StartElement)
			{
				if (xml.name() == "File")
				{
					double FrameRate_tmp = -1;

					while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "File"))
					{
						if (xml.tokenType() == QXmlStreamReader::StartElement)
						{
							if (xml.name() == "metadata")
							{
								while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "metadata"))
								{
									if (xml.tokenType() == QXmlStreamReader::StartElement)
									{
										if (xml.name() == "FrameRate")
										{
											FrameRate_tmp = xml.readElementText().toDouble();
										}

									}
									xml.readNext();
								}
							}
						}
						xml.readNext();
					}
					FrameRates.push_back(FrameRate_tmp);
				}
			}
		}
		if (FrameRates.size() <= 0) return false;

		double FrameRate = FrameRates[0];
		bool equal = FrameRate > 0;
		for (unsigned int i = 1; i < FrameRates.size(); i ++)
		{
			if (FrameRate != FrameRates[i]) equal = false;
		}

		if (equal)
		{
			recordingSpeed = FrameRate;
			return true;
		}
		else
		{
			return false;
		}
	}
	return true;
}

void Trial::parseXMLData()
{
	if (isDefault)
		return;

	if (hasStudyData){
		QXmlStreamReader xml(xml_data.join(""));

		while (!xml.atEnd() && !xml.hasError())
		{
			QXmlStreamReader::TokenType token = xml.readNext();

			if (token == QXmlStreamReader::StartDocument)
			{
				continue;
			}

			if (token == QXmlStreamReader::StartElement)
			{
				if (xml.name() == "Repository")
				{
					repository = xml.readElementText();
				}
				else if (xml.name() == "StudyName")
				{
					studyName = xml.readElementText();
				}
				else if (xml.name() == "StudyID")
				{
					studyID = xml.readElementText().toInt();
				}
				else if (xml.name() == "TrialName")
				{
					trialName = xml.readElementText();
				}
				else if (xml.name() == "TrialID")
				{
					trialID = xml.readElementText().toInt();
				}
				else if (xml.name() == "TrialNumber")
				{
					trialNumber = xml.readElementText().toInt();
				}
				else if (xml.name() == "TrialType")
				{
					trialType = xml.readElementText();
				}
				else if (xml.name() == "lab")
				{
					lab = xml.readElementText();
				}
				else if (xml.name() == "attribcomment")
				{
					attribcomment = xml.readElementText();
				}
				else if (xml.name() == "ts")
				{
					ts = xml.readElementText();
				}
				else if (xml.name() == "trialDate")
				{
					trialDate = xml.readElementText();
				}
			}
		}
		for (unsigned int i = 0; i < videos.size(); i++)
		{
			videos[i]->parseXMLData(i, xml_data.join(""));
		}
	}
}

const bool& Trial::getHasStudyData() const
{
	return hasStudyData;
}

const QString& Trial::getStudyName() const
{
	return studyName;
}

const QString& Trial::getRepository() const
{
	return repository;
}

const int& Trial::getStudyId() const
{
	return studyID;
}

const QString& Trial::getTrialName() const
{
	return trialName;
}

const int& Trial::getTrialId() const
{
	return trialID;
}

const int& Trial::getTrialNumber() const
{
	return trialNumber;
}

const QString& Trial::getTrialType() const
{
	return trialType;
}

const QString& Trial::getLab() const
{
	return lab;
}

const QString& Trial::getAttribcomment() const
{
	return attribcomment;
}

const QString& Trial::getTs() const
{
	return ts;
}

const QString& Trial::getTrialDate() const
{
	return trialDate;
}

void Trial::setInterpolate3D(bool val)
{
	interpolate3D = val;
}

bool Trial::getInterpolate3D()
{
	return interpolate3D;
}

bool Trial::getIsDefault()
{
	return isDefault;
}

void Trial::setIsDefault(bool value)
{
	isDefault = value;
}

bool Trial::getIsCopyFromDefault()
{
	return IsCopyFromDefault;
}

void Trial::setIsCopyFromDefault(bool value)
{
	IsCopyFromDefault = value;
}

void Trial::clearMarkerAndRigidBodies()
{
	for (std::vector<RigidBody*>::iterator rigidBody = rigidBodies.begin(); rigidBody != rigidBodies.end(); ++rigidBody)
	{
		delete *rigidBody;
	}
	rigidBodies.clear();
	for (std::vector<Marker*>::iterator marker = markers.begin(); marker != markers.end(); ++marker)
	{
		delete *marker;
	}
	markers.clear();
}

QString Trial::getFirstTrackedFrame()
{
	QString out_string;
	for (int i = 0; i != Project::getInstance()->getCameras().size(); i++){
		int firstFrame = -1;
		for (auto m : getMarkers())
		{
			int f = m->getFirstTrackedFrame(i);
			if (f >= 0)
			{
				if (f < firstFrame || firstFrame == -1)
					firstFrame = f;
			}
		}
		if (firstFrame != -1) {
			out_string += QString::number(firstFrame);
		}
		else
		{
			out_string += "NA";
		}
		if (i != Project::getInstance()->getCameras().size() - 1) out_string += " / ";
	}
	return out_string;
}

QString Trial::getLastTrackedFrame()
{
	QString out_string;
	for (int i = 0; i != Project::getInstance()->getCameras().size(); i++){
		int lastFrame = -1;
		for (auto m : getMarkers())
		{
			int f = m->getLastTrackedFrame(i);
			if (f >= 0)
			{
				if (f > lastFrame)
					lastFrame = f;
			}
		}
		if (lastFrame != -1){
			out_string += QString::number(lastFrame);
		}
		else
		{
			out_string += "NA";
		}
		if (i != Project::getInstance()->getCameras().size() - 1) out_string += " / ";
	}
	return out_string;
}

double Trial::getReprojectionError()
{
	int count = 0;
	double error = 0.0;
	for (auto m : getMarkers())
	{
		int c = m->getFramesTracked();
		error += c * m->getReprojectionError();
		count += c;
	}

	if (count != 0) error /= count;
	return error;
}

void Trial::setCameraSizes()
{
	for (int i = 0; i < videos.size(); i++)
	{
		Project::getInstance()->getCameras()[i]->setResolution(videos[i]->getImage()->getWidth(), videos[i]->getImage()->getHeight());
	}
}

bool Trial::checkTrialImageSizeValid()
{
	for (int i = 0; i < videos.size(); i++)
	{
		if (Project::getInstance()->getCameras()[i]->getWidth() != videos[i]->getImage()->getWidth() ||
			Project::getInstance()->getCameras()[i]->getHeight() != videos[i]->getImage()->getHeight())
			return false;
	}
	return true;
}

void Trial::saveVR(QString folder)
{
	for (auto rb : rigidBodies)
	{
		if (rb->hasMeshModel()){
			bool filter = (cutoffFrequency != 0.0 || rb->getOverrideCutoffFrequency());
			rb->saveTransformations(folder + rb->getDescription() + ".csv", false, filter);
			QFileInfo info(rb->getMeshModelname());
			QFile::copy(rb->getMeshModelname(), folder + info.fileName());
		}
	}
	for (auto m : markers)
	{
		m->save3DPoints(folder + m->getDescription() + ".csv", folder + m->getDescription() + "_status.csv");
	}
	QString outfilename = folder + "VRData.csv";
	std::ofstream outfile(outfilename.toAscii().data());
	for (auto rb : rigidBodies)
	{
		if (rb->hasMeshModel()){
			QFileInfo info(rb->getMeshModelname());
			outfile << "Obj" << "," << info.fileName().toAscii().data() << "," << (rb->getDescription() + ".csv").toAscii().data() << "," << rb->getMeshScale() << std::endl;
		}
	}
	for (auto m : markers)
	{
		outfile << "Marker" << "," << (m->getDescription() + ".csv").toAscii().data() << "," << (m->getDescription() + "_status.csv").toAscii().data() << "," << -1 << std::endl;
	}

	outfile.close();

}

double Trial::getMarkerToMarkerSD()
{
	int count = 0;
	double sd = 0.0;

	for (auto rb : getRigidBodies())
	{
		int c;
		double s;
		rb->getMarkerToMarkerSD(s,c);
		
		if (c > 0){
			sd += pow(s, 2) * (c - 1);
			count += c;
 		}
	}

	if (count != 0 )sd = sqrt(sd / (count - 1));

	return sd;
}
