#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Trial.h"
#include "core/Image.h"
#include "core/Camera.h"
#include "core/Project.h"
#include "core/RigidBody.h"

#include "ui/ConsoleDockWidget.h"
#include <QApplication>
#include <QFileInfo>

#include <fstream>

#ifdef WIN32
#define OS_SEP "\\"
#else
#define OS_SEP "/"
#endif

using namespace xma;

Trial::Trial(QString trialname, std::vector<QStringList> &imageFilenames){
	filenames = imageFilenames;
	name = trialname;
	activeFrame = 0;
	referenceCalibrationImage = 0;
	activeMarkerIdx = -1; 
	activeBody = -1;

	images.clear();
	nbImages = imageFilenames[0].size();
	for (std::vector< QStringList>::iterator filenameList = filenames.begin(); filenameList != filenames.end(); ++filenameList)
	{
		Image * newImage = new Image(filenameList->at(0));
		images.push_back(newImage);
		assert(nbImages == filenameList->size());
	}
}

Trial::Trial(QString trialname, QString folder){
	name = trialname;
	activeFrame = 0;
	images.clear();
	filenames.clear();
	for (int i = 0; i < Project::getInstance()->getCameras().size(); i++){
		QStringList filenameList;
		std::vector<std::vector<double> > values;
		QString camera_filenames = folder + Project::getInstance()->getCameras()[i]->getName() + "_filenames_absolute.txt";
		std::ifstream fin(camera_filenames.toAscii().data());
		for (std::string line; std::getline(fin, line);)
		{
			filenameList << QString::fromStdString(line);
		}
		filenames.push_back(filenameList);
		
		fin.close();
	}

	nbImages = filenames[0].size();
	for (std::vector< QStringList>::iterator filenameList = filenames.begin(); filenameList != filenames.end(); ++filenameList)
	{
		Image * newImage = new Image(filenameList->at(0));
		images.push_back(newImage);
		assert(nbImages == filenameList->size());
	}
}

Trial::~Trial(){
	for (std::vector< Image* >::iterator image = images.begin(); image != images.end(); ++image)
	{
		delete *image;
	}

	for (std::vector< RigidBody* >::iterator rigidBody = rigidBodies.begin(); rigidBody != rigidBodies.end(); ++rigidBody)
	{
		delete *rigidBody;
	}

	for (std::vector< Marker* >::iterator marker = markers.begin(); marker != markers.end(); ++marker)
	{
		delete *marker;
	}
}

void Trial::bindTextures()
{
	for (std::vector<Image *>::iterator image = images.begin(); image != images.end(); ++image)
	{
		(*image)->bindTexture();
	}
}

void Trial::save(QString path)
{
	for (int i = 0; i < filenames.size(); i++)
	{
		QString camera_filenames = path + Project::getInstance()->getCameras()[i]->getName() + "_filenames_absolute.txt";
		std::ofstream outfile(camera_filenames.toAscii().data());
		for (unsigned int j = 0; j < filenames[i].size(); ++j){
			outfile << filenames[i].at(j).toAscii().data() << std::endl;;
		}

		outfile.close();
	}
}

int Trial::getActiveFrame()
{
	return activeFrame;
}

void Trial::setActiveFrame(int _activeFrame)
{
	activeFrame = _activeFrame;
	for (int i = 0; i < images.size(); i++)
	{
		images[i]->setImage(filenames[i].at(activeFrame));
	}
}

Image* Trial::getImage(int cameraID)
{
	return images[cameraID];
}

const std::vector<QStringList>& Trial::getFilenames()
{
	return filenames;
}

const std::vector<Marker*>& Trial::getMarkers()
{
	return markers;
}

const std::vector<RigidBody*>& Trial::getRigidBodies()
{
	return rigidBodies;
}

Marker* Trial::getActiveMarker()
{
	if (activeMarkerIdx >= markers.size()) return NULL;

	return markers[activeMarkerIdx];
}

bool Trial::isActiveMarkerUndefined(int camera)
{
	return ( (markers.size() != 0 ) && (getMarkers()[activeMarkerIdx]->getStatus2D()[camera][activeFrame] == UNDEFINED));
}

void Trial::addRigidBody()
{
	RigidBody *rb = new RigidBody(nbImages);
	rigidBodies.push_back(rb);
}

void Trial::removeRigidBody(int idx)
{
	delete rigidBodies[idx];
	rigidBodies.erase(std::remove(rigidBodies.begin(), rigidBodies.end(), rigidBodies[idx]), rigidBodies.end());
	if (activeBody >= rigidBodies.size())activeBody = rigidBodies.size() - 1;
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
	if (activeMarkerIdx >= markers.size())setActiveMarkerIdx(markers.size() - 1);
}

void Trial::moveMarker(int camera, double x, double y)
{
	if (markers.size() > activeMarkerIdx){
		markers[activeMarkerIdx]->movePoint(camera, activeFrame, x, y);
	}
}

int Trial::getActiveMarkerIdx()
{
	return activeMarkerIdx;
}

void Trial::setActiveMarkerIdx(int _activeMarker)
{
	activeMarkerIdx = _activeMarker;
}

int Trial::getReferenceCalibrationImage()
{
	return referenceCalibrationImage;
}

QString Trial::getActiveFilename(int camera)
{
	QFileInfo info(filenames[camera].at(activeFrame));
	return info.fileName();
}

int Trial::getNbImages()
{
	return nbImages;
}

QString Trial::getName()
{
	return name;
}

void Trial::clear(){
	for (std::vector< Image*>::iterator image_it = images.begin(); image_it != images.end(); ++image_it){
		delete *image_it;
	}

	images.clear();
}

void Trial::drawPoints(int cameraId, bool detailView)
{
	glBegin(GL_LINES);
	int idx = 0;
	if (!detailView){
		for (std::vector <Marker *>::const_iterator it = markers.begin(); it != markers.end(); ++it){
			if (((*it)->getStatus2D()[cameraId][activeFrame] > 0)){
				if (idx == activeMarkerIdx){
					glColor3f(1.0, 0.0, 0.0);
				}
				else{
					glColor3f(0.0, 1.0, 0.0);
				}

				glVertex2f((*it)->getPoints2D()[cameraId][activeFrame].x - 5, (*it)->getPoints2D()[cameraId][activeFrame].y);
				glVertex2f((*it)->getPoints2D()[cameraId][activeFrame].x + 5, (*it)->getPoints2D()[cameraId][activeFrame].y);
				glVertex2f((*it)->getPoints2D()[cameraId][activeFrame].x, (*it)->getPoints2D()[cameraId][activeFrame].y - 5);
				glVertex2f((*it)->getPoints2D()[cameraId][activeFrame].x, (*it)->getPoints2D()[cameraId][activeFrame].y + 5);

				idx++;
			}
		}
	}
	else if (activeMarkerIdx >= 0 && activeMarkerIdx < markers.size())
	{
		double x = markers[activeMarkerIdx]->getPoints2D()[cameraId][activeFrame].x;
		double y = markers[activeMarkerIdx]->getPoints2D()[cameraId][activeFrame].y;

		glColor3f(1.0, 0.0, 0.0);
		glVertex2f(x - 12, y);
		glVertex2f(x + 12, y);
		glVertex2f(x, y - 12);
		glVertex2f(x, y + 12);

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
	}
	glEnd();

	if (activeMarkerIdx >= 0 && activeMarkerIdx < markers.size() && markers[activeMarkerIdx]->getStatus3D()[activeFrame] > 0){
		glBegin(GL_LINES);
		glColor3f(0.0, 1.0, 1.0);
		glVertex2f(markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].x - 5, markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].y - 5);
		glVertex2f(markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].x + 5, markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].y + 5);
		glVertex2f(markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].x + 5, markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].y - 5);
		glVertex2f(markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].x - 5, markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].y + 5);
		glEnd();
	}

	for (int i = 0; i < Project::getInstance()->getCameras().size(); i++)
	{
		if (activeMarkerIdx >= 0 && activeMarkerIdx < markers.size() && markers[activeMarkerIdx]->getStatus2D()[i][activeFrame] > 0){
			if (cameraId != i)
			{
				std::vector < cv::Point2d > epiline = markers[activeMarkerIdx]->getEpipolarLine(i, cameraId, activeFrame);
				glBegin(GL_LINE_STRIP);
				glColor3f(0.0, 0.0, 1.0);
				for (std::vector < cv::Point2d >::const_iterator pt = epiline.begin(); pt != epiline.end(); ++pt){
					glVertex2f(pt->x,pt->y);
				}
				epiline.clear();
				glEnd();
			}
		}
	}
}