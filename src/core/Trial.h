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
///\file Trial.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef TRIAL_H
#define TRIAL_H

#include <vector>
#include <QString>
#include <QStringList>
#include "VideoStream.h"
#include "EventData.h"

namespace xma
{
	class Image;
	class RigidBody;
	class Marker;
	class Camera;
	class VideoStream;

	class Trial
	{
	public:
		Trial(QString trialname, std::vector<QStringList>& imageFilenames);
		Trial(QString trialname, QString folder);
		Trial();
		virtual ~Trial();

		void changeTrialData(QString trialname, std::vector<QStringList>& imageFilenames);

		int getActiveFrame();
		void setActiveFrame(int _activeFrame);
		const std::vector<VideoStream *>& getVideoStreams();
		QString getActiveFilename(int camera);
		double getRecordingSpeed();
		void setRecordingSpeed(double value);

		int getReferenceCalibrationImage();
		void setReferenceCalibrationImage(int value);

		const std::vector<Marker *>& getMarkers();
		const std::vector<RigidBody *>& getRigidBodies();
		const std::vector<EventData*>& getEvents();

		Marker* getActiveMarker();
		int getActiveMarkerIdx();
		void setActiveMarkerIdx(int _activeMarker);
		bool isActiveMarkerUndefined(int camera);
		void setActiveToNextUndefinedMarker(int camera);

		RigidBody* getActiveRB();
		int getActiveRBIdx();
		void setActiveRBIdx(int _activeBody);

		void addRigidBody();
		void removeRigidBody(int idx);
		void addMarker();
		void removeMarker(int idx);
		void addEvent(QString name, QColor color);
		void removeEvent(int idx);


		double getCutoffFrequency();
		void setCutoffFrequency(double value);

		int getNbImages();
		QString getName();
		void bindTextures();
		void save(QString path);

		void drawRigidBodies(Camera* cam);
		void drawRigidBodiesMesh();
		bool renderMeshes();
		void drawPoints(int cameraId, bool detailView);

		int getStartFrame();
		void setStartFrame(int value);
		int getEndFrame();
		void setEndFrame(int value);

		bool getRequiresRecomputation();
		void setRequiresRecomputation(bool value);

		void loadMarkersFromCSV(QString filename, bool updateOnly = false);
		void loadMarkers(QString filename);
		void loadRigidBodies(QString filename);

		void saveMarkers(QString filename);
		void saveRigidBodies(QString filename);

		void changeImagePath(int camera, QString newfoldeFr, QString oldfolder);
		void updateAfterChangeImagePath();

		bool save3dPoints(std::vector<int> _markers, QString outputfolder, bool onefile, bool headerRow, double filterFrequency, bool saveColumn, int start, int stop);
		void save2dPoints(QString outputfolder, bool onefile, bool distorted, bool offset1, bool yinvert, bool headerRow, bool offsetCols);
		int load2dPoints(QString outputfolder, bool distorted, bool offset1, bool yinvert, bool headerRow, bool offsetCols);

		void saveRigidBodyTransformations(std::vector<int> _bodies, QString outputfolder, bool onefile, bool headerRow, bool filtered, bool saveColumn, int start, int stop);
		void saveTrialImages(QString outputfolder, int from, int to, QString format);
		void saveMarkerToMarkerDistances(QString filename, int from, int to);
		void recomputeAndFilterRigidBodyTransformations();
		void resetRigidBodyByMarker(Marker* marker, int frame);

		void getDrawTextData(int cam, int frame, std::vector<double>& x, std::vector<double>& y, std::vector<QString>& text);

		void saveXMLData(QString filename);
		void setXMLData(QString filename);
		bool setFrameRateFromXML();
		void parseXMLData();

		const bool& getHasStudyData() const;
		const QString& getStudyName() const;
		const QString& getRepository() const;
		const int& getStudyId() const;
		const QString& getTrialName() const;
		const int& getTrialId() const;
		const int& getTrialNumber() const;
		const QString& getTrialType() const;
		const QString& getLab() const;
		const QString& getAttribcomment() const;
		const QString& getTs() const;
		const QString& getTrialDate() const;

		void setInterpolate3D(bool val);
		bool getInterpolate3D();

		bool getIsDefault();
		void setIsDefault(bool value);
		bool getIsCopyFromDefault();
		void setIsCopyFromDefault(bool value);

		void clearMarkerAndRigidBodies();

		int getFirstTrackedFrame();
		int getLastTrackedFrame();
		double getReprojectionError();
		double getMarkerToMarkerSD();

		void setCameraSizes();

		
	private:
		QString name;
		void setNbImages();

		int activeFrame;
		int activeMarkerIdx;
		int activeBodyIdx;

		int nbImages;

		int referenceCalibrationImage;
		double recordingSpeed;
		double cutoffFrequency;
		int interpolateMissingFrames;

		int startFrame;
		int endFrame;

		std::vector<VideoStream*> videos;
		std::vector<RigidBody*> rigidBodies;
		std::vector<Marker*> markers;
		std::vector<EventData*> events;

		bool requiresRecomputation;

		bool hasStudyData;

		//StudyData
		QString studyName;
		QString repository;
		int studyID;

		//CalibrationTrial Data
		QString trialName;
		int trialID;
		int trialNumber;
		QString trialType;

		QString lab;
		QString attribcomment;
		QString	ts;
		QString	trialDate;

		QStringList xml_data;

		bool interpolate3D;

		bool isDefault; 
		bool IsCopyFromDefault;
	};
}


#endif // TRIAL_H


