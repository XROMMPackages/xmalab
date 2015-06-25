#ifndef TRIAL_H
#define TRIAL_H

#include <vector>
#include <QString>
#include <QStringList>
#include "VideoStream.h"

namespace xma{
	class Image;
	class RigidBody;
	class Marker;
	class Camera;
	class VideoStream;

	class Trial{

	public:
		Trial(QString trialname, std::vector<QStringList> &imageFilenames);
		Trial(QString trialname, QString folder);
		virtual ~Trial();

		int getActiveFrame();
		void setActiveFrame(int _activeFrame);
		const std::vector <VideoStream *> & getVideoStreams();
		QString getActiveFilename(int camera);
		double getRecordingSpeed();
		void setRecordingSpeed(double value);

		
		int getReferenceCalibrationImage();
		void setReferenceCalibrationImage(int value);

		const std::vector <Marker * > &getMarkers();
		const std::vector <RigidBody * > &getRigidBodies();

		Marker *getActiveMarker();
		int getActiveMarkerIdx();
		void setActiveMarkerIdx(int _activeMarker);
		bool isActiveMarkerUndefined(int camera);
		void setActiveToNextUndefinedMarker(int camera);

		RigidBody *getActiveRB();
		int getActiveRBIdx();
		void setActiveRBIdx(int _activeBody);

		void addRigidBody();
		void removeRigidBody(int idx);
		void addMarker();
		void removeMarker(int idx);
		

		double getCutoffFrequency();
		void setCutoffFrequency(double value);
		int getInterpolateMissingFrames();
		void setInterpolateMissingFrames(int value);

		int getNbImages();
		QString getName();
		void bindTextures();
		void save(QString path);

		void drawRigidBodies(Camera * cam);
		void drawPoints(int cameraId, bool detailView);

		int getStartFrame();
		void setStartFrame(int value);
		int getEndFrame();
		void setEndFrame(int value);

		void loadMarkers(QString filename);
		void loadRigidBodies(QString filename);

		void saveMarkers(QString filename);
		void saveRigidBodies(QString filename);

		void update();
		void changeImagePath(int camera, QString newfoldeFr, QString oldfolder);
		void updateAfterChangeImagePath();

		void save3dPoints(QString outputfolder, bool onefile, bool headerRow);
		void save2dPoints(QString outputfolder, bool onefile, bool distorted, bool offset1, bool yinvert, bool headerRow, bool offsetCols);
		int load2dPoints(QString outputfolder, bool distorted, bool offset1, bool yinvert, bool headerRow, bool offsetCols);

		void saveRigidBodyTransformations(QString outputfolder, bool onefile, bool headerRow, bool filtered);
		void saveTrialImages(QString outputfolder);
		void recomputeAndFilterRigidBodyTransformations();
		void resetRigidBodyByMarker(Marker * marker, int frame);

		void getDrawTextData(int cam, int frame, std::vector<double>& x, std::vector<double>& y, std::vector<QString>& text);

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

		std::vector <VideoStream*> videos;
		std::vector <RigidBody * > rigidBodies;
		std::vector <Marker * > markers;
		int id;
	};
}

	

#endif  // TRIAL_H
