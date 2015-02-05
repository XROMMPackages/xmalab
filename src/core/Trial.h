#ifndef TRIAL_H
#define TRIAL_H

#include <vector>
#include <QString>
#include <QStringList>

namespace xma{
	class Image;
	class RigidBody;
	class Marker;

	class Trial{

	public:
		Trial(QString trialname, std::vector<QStringList> &imageFilenames);
		Trial(QString trialname, QString folder);
		virtual ~Trial();

		int getActiveFrame();
		void setActiveFrame(int _activeFrame);
		Image * getImage(int cameraID);

		const std::vector <QStringList> & getFilenames();
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
		
		QString getActiveFilename(int camera);
		int getReferenceCalibrationImage();
		void setReferenceCalibrationImage(int value);

		int getNbImages();
		QString getName();
		void bindTextures();
		void save(QString path);

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
		void changeImagePath(int camera, QString newfolder, QString oldfolder);

		void save3dPoints(QString outputfolder);
		void save2dPoints(QString outputfolder);


	private:
		QString name;
		void clear();

		int activeFrame;
		int activeMarkerIdx;
		int activeBodyIdx;

		int nbImages;
		int referenceCalibrationImage;

		int startFrame;
		int endFrame;

		std::vector <Image * > images;
		std::vector <QStringList> filenames;
		std::vector <RigidBody * > rigidBodies;
		std::vector <Marker * > markers;
		int id;
		

		std::istream &comma(std::istream& in);
		std::istream &getline(std::istream &is, std::string &s);
	};
}

	

#endif  // TRIAL_H
