#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Trial.h"
#include "core/Image.h"
#include "core/Camera.h"
#include "core/Project.h"
#include "core/RigidBody.h"
#include "core/Marker.h"

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
	activeBodyIdx = -1;

	images.clear();
	nbImages = imageFilenames[0].size();
	for (std::vector< QStringList>::iterator filenameList = filenames.begin(); filenameList != filenames.end(); ++filenameList)
	{
		Image * newImage = new Image(filenameList->at(0));
		images.push_back(newImage);
		assert(nbImages == filenameList->size());
	}

	startFrame = 1;
	endFrame = nbImages;
}

Trial::Trial(QString trialname, QString folder){
	name = trialname;
	activeFrame = 0;
	referenceCalibrationImage = 0;
	images.clear();
	filenames.clear();
	for (int i = 0; i < Project::getInstance()->getCameras().size(); i++){
		QStringList filenameList;
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

	startFrame = 1;
	endFrame = nbImages;
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
	if (activeMarkerIdx >= markers.size() || activeMarkerIdx < 0) return NULL;

	return markers[activeMarkerIdx];
}

bool Trial::isActiveMarkerUndefined(int camera)
{
	return ((markers.size() != 0) && (activeMarkerIdx < markers.size()) && (activeMarkerIdx >= 0) && (getMarkers()[activeMarkerIdx]->getStatus2D()[camera][activeFrame] == UNDEFINED));
}

void Trial::setActiveToNextUndefinedMarker(int camera)
{
	if (markers.size() == 0)
	{
		addMarker();
		return;
	}
	else{
		if (activeMarkerIdx < 0) activeMarkerIdx = 0;

		while (activeMarkerIdx < markers.size() && getMarkers()[activeMarkerIdx]->getStatus2D()[camera][activeFrame] != UNDEFINED)
		{
			activeMarkerIdx++;
		}
		if (activeMarkerIdx == markers.size()) addMarker();
	}
}

RigidBody* Trial::getActiveRB()
{
	if (activeBodyIdx >= rigidBodies.size() || activeBodyIdx < 0) return NULL;

	return rigidBodies[activeBodyIdx];
}

void Trial::addRigidBody()
{
	RigidBody *rb = new RigidBody(nbImages, this);
	rigidBodies.push_back(rb);
}

void Trial::removeRigidBody(int idx)
{
	delete rigidBodies[idx];
	rigidBodies.erase(std::remove(rigidBodies.begin(), rigidBodies.end(), rigidBodies[idx]), rigidBodies.end());
	if (activeBodyIdx >= rigidBodies.size())activeBodyIdx = rigidBodies.size() - 1;
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
	if (value != referenceCalibrationImage)
	{
		referenceCalibrationImage = value;
		update();
	}
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
	
	int idx = 0;
	if (!detailView){
		glBegin(GL_LINES);
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
			}
			idx++;
		}
		glEnd();
	}
	else if (activeMarkerIdx >= 0 && activeMarkerIdx < markers.size())
	{
		double x = markers[activeMarkerIdx]->getPoints2D()[cameraId][activeFrame].x;
		double y = markers[activeMarkerIdx]->getPoints2D()[cameraId][activeFrame].y;
		glBegin(GL_LINES);
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
		glEnd();
		double size = markers[activeMarkerIdx]->getSize();;
		if (size > 0)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBegin(GL_LINES);
			glColor4f(1.0, 0.0, 0.0, 0.3);
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

std::istream& Trial::comma(std::istream& in)
{
	if ((in >> std::ws).peek() != std::char_traits<char>::to_int_type(',')) {
		in.setstate(std::ios_base::failbit);
	}
	return in.ignore();
}

std::istream &Trial::getline(std::istream &is, std::string &s) {
	char ch;

	s.clear();

	while (is.get(ch) && ch != '\n' && ch != '\r')
		s += ch;
	return is;
}

void Trial::loadMarkers(QString filename){
	std::ifstream fin;
	fin.open(filename.toAscii().data());
	std::string line;
	unsigned int count = 0;
	for (; getline(fin, line);)
	{
		if (count >= this->getMarkers().size())
			this->addMarker();

		this->getMarkers()[count]->setDescription(QString::fromStdString(line));
		count++;
		line.clear();
	}
	fin.close();
}

void Trial::loadRigidBodies(QString filename){
	std::ifstream fin;
	fin.open(filename.toAscii().data());
	std::istringstream in;
	std::string line;
	std::string desc;
	std::string indices;
	int count = 0;
	for (; getline(fin, line);)
	{
		if (count >= this->getRigidBodies().size()){
			this->addRigidBody();
		}
		desc = line.substr(0, line.find('['));
		indices = line.substr(line.find('[') + 1, line.find(']') - 1);
		this->getRigidBodies()[count]->setDescription(QString::fromStdString(desc));

		this->getRigidBodies()[count]->clearPointIdx();
		in.clear();
		in.str(indices);
		for (int value; in >> value; comma(in)) {
			this->getRigidBodies()[count]->addPointIdx(value - 1);
		}
		count++;
		line.clear();
		desc.clear();
		indices.clear();
	}
	fin.close();
}

void Trial::saveMarkers(QString filename){
	std::ofstream outfile(filename.toAscii().data());
	for (unsigned int i = 0; i < this->getMarkers().size(); i++){
		outfile << this->getMarkers()[i]->getDescription().toAscii().data() << std::endl;
	}
	outfile.close();
}

void Trial::saveRigidBodies(QString filename){
	std::ofstream outfile(filename.toAscii().data());
	for (unsigned int i = 0; i < this->getRigidBodies().size(); i++){
		outfile << this->getRigidBodies()[i]->getDescription().toAscii().data() << "[";
		for (unsigned int k = 0; k < this->getRigidBodies()[i]->getPointsIdx().size(); k++){
			outfile << this->getRigidBodies()[i]->getPointsIdx()[k] + 1;
			if (k != (this->getRigidBodies()[i]->getPointsIdx().size() - 1)) outfile << " , ";
		}
		outfile << "]" << std::endl;
	}
	outfile.close();
}

void Trial::update()
{
	for (int i = 0; i < getMarkers().size(); i++)
	{
		getMarkers()[i]->update();
	}
}

void Trial::changeImagePath(int camera, QString newfolder, QString oldfolder)
{
	fprintf(stderr, "Change from %s to %s\n", oldfolder.toAscii().data(), newfolder.toAscii().data());
	filenames[camera] = filenames[camera].replaceInStrings(oldfolder, newfolder);

	for (int i = 0; i < filenames[camera].size(); i++)
	{
		//fprintf(stderr, "%s\n", filenames[camera].at(i).toAscii().data());
	}


	images[camera]->setImage(filenames[camera].at(activeFrame));
}