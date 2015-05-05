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

#include <fstream>

#ifdef WIN32
#define OS_SEP "\\"
#else
#define OS_SEP "/"
#endif
#include <QtGui/QApplication>


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
		for (std::string line; littleHelper::safeGetline(fin, line);)
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
		if (!detailView || Settings::getInstance()->getBoolSetting("ShowEpiLineDetailView")){
			if (activeMarkerIdx >= 0 && activeMarkerIdx < markers.size() && markers[activeMarkerIdx]->getStatus2D()[i][activeFrame] > 0){
				if (cameraId != i)
				{
					std::vector < cv::Point2d > epiline = markers[activeMarkerIdx]->getEpipolarLine(i, cameraId, activeFrame);
					glBegin(GL_LINE_STRIP);
					glColor3f(0.0, 0.0, 1.0);
					for (std::vector < cv::Point2d >::const_iterator pt = epiline.begin(); pt != epiline.end(); ++pt){
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

void Trial::loadMarkers(QString filename){
	std::ifstream fin;
	fin.open(filename.toAscii().data());
	std::string line;
	unsigned int count = 0;
	for (; littleHelper::safeGetline(fin, line);)
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
	for (; littleHelper::safeGetline(fin, line);)
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
		for (int value; in >> value; littleHelper::comma(in)) {
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

void Trial::recomputeAndFilterRigidBodyTransformations()
{
	for (int i = 0; i < getRigidBodies().size(); i++)
	{
		getRigidBodies()[i]->recomputeTransformations();
		
		getRigidBodies()[i]->filterTransformations();
	}
}

void Trial::saveRigidBodyTransformations(QString outputfolder, bool onefile, bool headerRow, bool filtered)
{
	for (int i = 0; i < getRigidBodies().size(); i++)
	{
		getRigidBodies()[i]->recomputeTransformations();
		if (filtered)getRigidBodies()[i]->filterTransformations();
	}

	if (onefile)
	{
		std::ofstream outfile(outputfolder.toAscii().data());
		if (headerRow)
		{
			for (int i = 0; i < getRigidBodies().size(); i++)
			{
				QString name;
				QString filterRate = "";
				if (getRigidBodies()[i]->getDescription().isEmpty())
				{
					name = "RigidBody" + QString().sprintf("%03d", i + 1);
				}
				else
				{
					name = getRigidBodies()[i]->getDescription();
				}

				if (filtered)
				{
					filterRate = "_" + QString::number(getRigidBodies()[i]->getOverrideCutoffFrequency() ? getRigidBodies()[i]->getCutoffFrequency() : getCutoffFrequency()) + "Hz";
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
						<< name.toAscii().data() << "_1" << filterRate.toAscii().data() ;

				if (i != getRigidBodies().size() - 1){
					outfile << " , ";
				}
				else
				{
					outfile << std::endl;
				}
			}

			double trans[16];
			for (unsigned int f = 0; f < nbImages; f++){
				for (int i = 0; i < getRigidBodies().size(); i++)
				{
					if (getRigidBodies()[i]->getTransformationMatrix(f,filtered,&trans[0]))
					{
						outfile << trans[0] << " , " << trans[1] << " , " << trans[2] << " , " << trans[3] << " , ";
						outfile << trans[4] << " , " << trans[5] << " , " << trans[6] << " , " << trans[7] << " , ";
						outfile << trans[8] << " , " << trans[9] << " , " << trans[10] << " , " << trans[11] << " , ";
						outfile << trans[12] << " , " << trans[13] << " , " << trans[14] << " , " << trans[15];
					}
					else{
						outfile << "NaN , NaN , NaN , NaN , ";
						outfile << "NaN , NaN , NaN , NaN , ";
						outfile << "NaN , NaN , NaN , NaN , ";
						outfile << "NaN , NaN , NaN , NaN";
					}

					if (i != getRigidBodies().size() - 1){
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
		for (int i = 0; i < getRigidBodies().size(); i++)
		{
			QString filename;
			if (filtered)
			{
				filename = outputfolder + "RigidBody" + QString().sprintf("%03d", i + 1) + "_" + getRigidBodies()[i]->getDescription() + "_transformationFiltered_" + QString::number(getRigidBodies()[i]->getOverrideCutoffFrequency() ? getRigidBodies()[i]->getCutoffFrequency() : getCutoffFrequency()) + "Hz.csv";
			}
			else
			{
				filename = outputfolder + "RigidBody" + QString().sprintf("%03d", i + 1) + "_" + getRigidBodies()[i]->getDescription() + "_transformation.csv";
			}

			std::ofstream outfile(filename.toAscii().data());
			if (headerRow)
			{
				QString name;
				QString filterRate = "";
				if (getRigidBodies()[i]->getDescription().isEmpty())
				{
					name = "RigidBody" + QString().sprintf("%03d", i + 1);
				}
				else
				{
					name = getRigidBodies()[i]->getDescription();
				}

				if (filtered)
				{
					filterRate = "_" + QString::number(getRigidBodies()[i]->getOverrideCutoffFrequency() ? getRigidBodies()[i]->getCutoffFrequency() : getCutoffFrequency()) + "Hz";
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
			for (unsigned int f = 0; f < nbImages; f++){
				if (getRigidBodies()[i]->getTransformationMatrix(f, filtered, &trans[0]))
				{
					outfile << trans[0] << " , " << trans[1] << " , " << trans[2] << " , " << trans[3] << " , ";
					outfile << trans[4] << " , " << trans[5] << " , " << trans[6] << " , " << trans[7] << " , ";
					outfile << trans[8] << " , " << trans[9] << " , " << trans[10] << " , " << trans[11] << " , ";
					outfile << trans[12] << " , " << trans[13] << " , " << trans[14] << " , " << trans[15];
				}
				else{
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


void Trial::saveTrialImages(QString outputfolder)
{
	for (int i = 0; i < videos.size(); i++)
	{
		QFileInfo info(videos[i]->getFileBasename());
		QString foldername = outputfolder + info.completeBaseName();
		if (!QDir().mkpath(foldername)){
			return;
		}
		if (Project::getInstance()->getCameras()[i]->hasUndistortion()){
			for (int j = 0; j < videos[i]->getNbImages(); j++)
			{
				QString outname = foldername + OS_SEP + info.completeBaseName() + "_" + QString("%1").arg(j + 1, 6, 10, QChar('0')) + ".tif";
				videos[i]->setActiveFrame(j);
				Project::getInstance()->getCameras()[i]->getUndistortionObject()->undistort(videos[i]->getImage(), outname);
			}
		}
		else
		{
			for (int j = 0; j < videos[i]->getNbImages(); j++)
			{
				QString outname = foldername + OS_SEP + info.completeBaseName() + "_" + QString("%1").arg(j + 1, 6, 10, QChar('0')) + ".tif";
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

void Trial::save3dPoints(QString outputfolder, bool onefile, bool headerRow)
{
	if (onefile)
	{
		std::ofstream outfile(outputfolder.toAscii().data());
		if (headerRow)
		{
			for (int i = 0; i < getMarkers().size(); i++)
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

				outfile << name.toAscii().data() << "_X" << " , " << name.toAscii().data() << "_Y" << " , " << name.toAscii().data() << "_Z";

				if (i != getMarkers().size() - 1){
					outfile << " , ";
				}
				else
				{
					outfile << std::endl;
				}
			}
		}

		for (unsigned int f = 0; f < nbImages; f++){
			for (int i = 0; i < getMarkers().size(); i++)
			{				
				if (getMarkers()[i]->getStatus3D()[f] <= 0)
				{
					outfile << "NaN" << " , " << "NaN" << " , " << "NaN";
				}
				else{
					outfile << getMarkers()[i]->getPoints3D()[f].x << " , " << getMarkers()[i]->getPoints3D()[f].y << " , " << getMarkers()[i]->getPoints3D()[f].z ;
				}

				if (i != getMarkers().size() - 1){
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
		for (int i = 0; i < getMarkers().size(); i++)
		{
			QString filename = outputfolder + "Marker" + QString().sprintf("%03d", i + 1) + "_" + getMarkers()[i]->getDescription() + "_points3d.csv";
			std::ofstream outfile(filename.toAscii().data());
			if (headerRow)
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

				outfile << name.toAscii().data() << "_X" << " , " << name.toAscii().data() << "_Y" << " , " << name.toAscii().data() << "_Z";

				outfile << std::endl;
			}
			for (unsigned int f = 0; f < nbImages; f++){
				if (getMarkers()[i]->getStatus3D()[f] <= 0)
				{
					outfile << "NaN" << " , " << "NaN" << " , " << "NaN";
				}
				else{
					outfile << getMarkers()[i]->getPoints3D()[f].x << " , " << getMarkers()[i]->getPoints3D()[f].y << " , " << getMarkers()[i]->getPoints3D()[f].z ;
				}

				outfile << std::endl;
							
			}
			outfile.close();
		}
	}
}

void Trial::save2dPoints(QString outputfolder, bool onefile, bool distorted, bool offset1, bool yinvert, bool headerRow,bool offsetCols)
{
	if (onefile)
	{
		std::ofstream outfile(outputfolder.toAscii().data());
		if (headerRow)
		{
			if (offsetCols){
				for (int j = 0; j < Project::getInstance()->getCameras().size(); j++)
				{
					outfile << "cam" << j << "_offset";
				}
			}

			for (int i = 0; i < getMarkers().size(); i++)
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

				for (int j = 0; j < Project::getInstance()->getCameras().size(); j++)
				{
					outfile << name.toAscii().data() << "_cam" << j + 1 << "_X" << " , " << name.toAscii().data() << "_cam" << j + 1 << "_Y";

					if (i != getMarkers().size() - 1 || j != Project::getInstance()->getCameras().size() - 1){
						outfile << " , ";
					}
					else
					{
						outfile << std::endl;
					}
				}
			}
		}
		
		for (unsigned int f = 0; f < nbImages; f++){
			if (offsetCols){
				for (int j = 0; j < Project::getInstance()->getCameras().size(); j++)
				{
					outfile << 0.0 << " , ";
				}
			}
			for (int i = 0; i < getMarkers().size(); i++)
			{
				for (int j = 0; j < Project::getInstance()->getCameras().size(); j++)
				{
					if (getMarkers()[i]->getStatus2D()[j][f] > 0){

						double x;
						double y;
						if (distorted)
						{
							x = getMarkers()[i]->getPoints2D()[j][f].x;
							y = getMarkers()[i]->getPoints2D()[j][f].y;
						}
						else
						{
							if (Project::getInstance()->getCameras()[j]->hasUndistortion())
							{
								cv::Point2d pt = Project::getInstance()->getCameras()[j]->getUndistortionObject()->transformPoint(getMarkers()[i]->getPoints2D()[j][f], true);
								x = pt.x;
								y = pt.y;
							}
							else
							{
								x = getMarkers()[i]->getPoints2D()[j][f].x;
								y = getMarkers()[i]->getPoints2D()[j][f].y;
							}
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
					
					if (i != getMarkers().size() - 1 || j != Project::getInstance()->getCameras().size() - 1){
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
		for (int i = 0; i < getMarkers().size(); i++)
		{
			QString filename = outputfolder + "Marker" + QString().sprintf("%03d", i + 1) + "_" + getMarkers()[i]->getDescription() + "_points2d.csv";
			std::ofstream outfile(filename.toAscii().data());
			if (headerRow)
			{
				if (offsetCols){
					for (int j = 0; j < Project::getInstance()->getCameras().size(); j++)
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

				for (int j = 0; j < Project::getInstance()->getCameras().size(); j++)
				{
					outfile << name.toAscii().data() << "_cam" << j + 1 << "_X"<< " , " << name.toAscii().data() << "_cam" << j + 1 << "_Y";

					if (j != Project::getInstance()->getCameras().size() - 1){
						outfile << " , ";
					}
					else
					{
						outfile << std::endl;
					}
				}
			}
			
			for (unsigned int f = 0; f < nbImages; f++){
				if (offsetCols){
					for (int j = 0; j < Project::getInstance()->getCameras().size(); j++)
					{
						outfile << 0.0 << " , ";
					}
				}

				for (int j = 0; j < Project::getInstance()->getCameras().size(); j++)
				{
					if (getMarkers()[i]->getStatus2D()[j][f] > 0){
						double x;
						double y;
						if (distorted)
						{
							x = getMarkers()[i]->getPoints2D()[j][f].x;
							y = getMarkers()[i]->getPoints2D()[j][f].y;
						}
						else
						{
							if (Project::getInstance()->getCameras()[j]->hasUndistortion())
							{
								cv::Point2d pt = Project::getInstance()->getCameras()[j]->getUndistortionObject()->transformPoint(getMarkers()[i]->getPoints2D()[j][f], true);
								x = pt.x;
								y = pt.y;
							}
							else
							{
								x = getMarkers()[i]->getPoints2D()[j][f].x;
								y = getMarkers()[i]->getPoints2D()[j][f].y;
							}
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

					if (j != Project::getInstance()->getCameras().size() - 1){
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
	std::vector <Marker *> newMarkers;

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
			for (int i = 0; i < newMarkers.size(); i++)
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
			for (int i = 0; i < newMarkers.size(); i++)
			{
				for (int j = 0; j < Project::getInstance()->getCameras().size(); j++)
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

						newMarkers[i]->setPoint(j, frame, x, y, SET);
					}
				}
			}
			frame++;
		}
	}

	for (int i = 0; i < newMarkers.size(); i++)
	{
		newMarkers[i]->update();
	}
	fin.close();

	return newMarkers.size();
}