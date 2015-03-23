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
#include <QFileInfo>
#include <QDir>

#include <fstream>

#ifdef WIN32
#define OS_SEP "\\"
#else
#define OS_SEP "/"
#endif



using namespace xma;

Trial::Trial(QString trialname, std::vector<QStringList> &imageFilenames){
	name = trialname;
	activeFrame = 0;
	referenceCalibrationImage = 0;
	recordingSpeed = 0;
	cutoffFrequency = 0;
	interpolateMissingFrames = 0;

	activeMarkerIdx = -1; 
	activeBodyIdx = -1;

	for (std::vector< QStringList>::iterator filenameList = imageFilenames.begin(); filenameList != imageFilenames.end(); ++filenameList)
	{
		VideoStream * newSequence = NULL;
		if ((*filenameList).size() > 1){
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

	startFrame = 1;
	endFrame = nbImages;
}

Trial::Trial(QString trialname, QString folder){
	name = trialname;
	activeFrame = 0;
	referenceCalibrationImage = 0;
	recordingSpeed = 0;
	cutoffFrequency = 0;
	interpolateMissingFrames = 0;

	activeMarkerIdx = -1;
	activeBodyIdx = -1;

	for (int i = 0; i < Project::getInstance()->getCameras().size(); i++){
		QStringList filenameList;
		QString camera_filenames = folder + Project::getInstance()->getCameras()[i]->getName() + "_filenames_absolute.txt";
		std::ifstream fin(camera_filenames.toAscii().data());
		for (std::string line; std::getline(fin, line);)
		{
			filenameList << QString::fromStdString(line);
		}
		fin.close();

		VideoStream * newSequence = NULL;
		if (filenameList.size() > 1){
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

	startFrame = 1;
	endFrame = nbImages;
}



Trial::~Trial(){
	for (std::vector< VideoStream* >::iterator video = videos.begin(); video != videos.end(); ++video)
	{
		delete *video;
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

void Trial::setNbImages()
{
	nbImages = 0;
	for (int i = 0; i < videos.size(); i++)
	{
		if (videos[i]->getNbImages() >= 0){
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
	for (std::vector< VideoStream* >::iterator video = videos.begin(); video != videos.end(); ++video)
	{
		(*video)->bindTexture();
	}
}

void Trial::save(QString path)
{
	for (int i = 0; i < videos.size(); i++)
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
	for (std::vector< VideoStream* >::iterator video = videos.begin(); video != videos.end(); ++video)
	{
		(*video)->setActiveFrame(activeFrame);
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

int Trial::getInterpolateMissingFrames()
{
	return interpolateMissingFrames;
}

void Trial::setInterpolateMissingFrames(int value)
{
	interpolateMissingFrames = value;
}

QString Trial::getActiveFilename(int camera)
{
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

void Trial::drawRigidBodies(Camera * cam)
{
	for (std::vector <RigidBody *>::const_iterator it = rigidBodies.begin(); it != rigidBodies.end(); ++it){
		(*it)->draw2D(cam, activeFrame);
	}
}

void Trial::drawPoints(int cameraId, bool detailView)
{
	
	int idx = 0;
	if (!detailView ){
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
		glEnd();

		if (Settings::getInstance()->getBoolSetting("AdvancedCrosshairDetailView")){
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
	}

	if (activeMarkerIdx >= 0 && activeMarkerIdx < markers.size() && markers[activeMarkerIdx]->getStatus3D()[activeFrame] > 0){
		if (!detailView || Settings::getInstance()->getBoolSetting("Show3dPointDetailView")){
			glBegin(GL_LINES);
			glColor3f(0.0, 1.0, 1.0);
			glVertex2f(markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].x - 5, markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].y - 5);
			glVertex2f(markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].x + 5, markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].y + 5);
			glVertex2f(markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].x + 5, markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].y - 5);
			glVertex2f(markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].x - 5, markers[activeMarkerIdx]->getPoints2D_projected()[cameraId][activeFrame].y + 5);
			glEnd();
		}
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
	videos[camera]->changeImagePath(newfolder, oldfolder); 	
}

void Trial::updateAfterChangeImagePath()
{
	setNbImages();

	recordingSpeed = videos[0]->getFPS();
}

void Trial::save3dPoints(QString outputfolder)
{
	for (int i = 0; i < getMarkers().size(); i++)
	{
		getMarkers()[i]->save("", "", "", outputfolder + "Marker" + QString().sprintf("%03d", i) + "_" + getMarkers()[i]->getDescription() + "_points3d.csv");
	}
}

void Trial::recomputeAndFilterRigidBodyTransformations()
{
	for (int i = 0; i < getRigidBodies().size(); i++)
	{
		getRigidBodies()[i]->recomputeTransformations();
		
		getRigidBodies()[i]->filterTransformations();
	}
}

void Trial::saveRigidBodyTransformations(QString outputfolder)
{
	for (int i = 0; i < getRigidBodies().size(); i++)
	{
		getRigidBodies()[i]->recomputeTransformations();
		
		getRigidBodies()[i]->saveTransformations(outputfolder + "RigidBody" + QString().sprintf("%03d", i + 1) + "_" + getRigidBodies()[i]->getDescription() + "_transformation.csv", true, false);
	}
}

void Trial::saveRigidBodyTransformationsFiltered(QString outputfolder)
{
	for (int i = 0; i < getRigidBodies().size(); i++)
	{
		getRigidBodies()[i]->recomputeTransformations();
		getRigidBodies()[i]->filterTransformations();
		getRigidBodies()[i]->saveTransformations(outputfolder + "RigidBody" + QString().sprintf("%03d", i + 1) + "_" + getRigidBodies()[i]->getDescription() + "_transformationFiltered_" + QString::number(getRigidBodies()[i]->getOverrideCutoffFrequency() ? getRigidBodies()[i]->getCutoffFrequency() : getCutoffFrequency()) + "Hz.csv", true, true);
	}
}

void Trial::saveTrialImages(QString outputfolder)
{
	for (int i = 0; i < videos.size(); i++)
	{
		QFileInfo info(videos[i]->getFileBasename());
		QString foldername = outputfolder + info.baseName();
		if (!QDir().mkpath(foldername)){
			return;
		}
		if (Project::getInstance()->getCameras()[i]->hasUndistortion()){
			for (int j = 0; j < videos[i]->getNbImages(); j++)
			{
				QString outname = foldername + OS_SEP + info.baseName() + "_" + QString("%1").arg(j + 1, 6, 10, QChar('0')) + ".tif";
				videos[i]->setActiveFrame(j);
				Project::getInstance()->getCameras()[i]->getUndistortionObject()->undistort(videos[i]->getImage(), outname);
			}
		}
		else
		{
			for (int j = 0; j < videos[i]->getNbImages(); j++)
			{
				QString outname = foldername + OS_SEP + info.baseName() + "_" + QString("%1").arg(j + 1, 6, 10, QChar('0')) + ".tif";
				videos[i]->setActiveFrame(j);
				videos[i]->getImage()->save(outname);
			}
		}
	}
}

void Trial::resetRigidBodyByMarker(Marker* marker, int frame)
{
	for (int i = 0; i < getRigidBodies().size(); i++)
	{
		for (int j = 0; j < getRigidBodies()[i]->getPointsIdx().size(); j++)
		{
			if (getRigidBodies()[i]->getPointsIdx()[j] < getMarkers().size() && getMarkers()[getRigidBodies()[i]->getPointsIdx()[j]] == marker)
			{
				getRigidBodies()[i]->computePose(frame);
				return;
			}

		}
	}
}

void Trial::save2dPoints(QString outputfolder)
{
	for (int i = 0; i < getMarkers().size(); i++)
	{
		getMarkers()[i]->save(outputfolder + "Marker" + QString().sprintf("%03d", i) + "_" + getMarkers()[i]->getDescription() + "_points2d.csv", "", "", "");
	}
}