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
		Trial::Trial(QString trialname, QString folder);
		virtual ~Trial();

		int getActiveFrame();
		void setActiveFrame(int _activeFrame);
		Image * getImage(int cameraID);

		const std::vector <QStringList> & getFilenames();
		const std::vector <Marker * > &getMarkers();
		const std::vector <RigidBody * > &getRigidBodies();

		Marker *getActiveMarker();
		bool isActiveMarkerUndefined(int camera);

		int getActiveMarkerIdx();
		void setActiveMarkerIdx(int _activeMarker);

		void addRigidBody();
		void removeRigidBody(int idx);

		void addMarker();
		void removeMarker(int idx);
		void moveMarker(int camera, double x, double y);
		
		QString getActiveFilename(int camera);
		int getReferenceCalibrationImage();
		int getNbImages();
		QString getName();
		void bindTextures();
		void save(QString path);

		void drawPoints(int cameraId, bool detailView);

	private:
		QString name;
		void clear();

		int activeFrame;
		int activeMarkerIdx;
		int activeBody;

		int nbImages;
		int referenceCalibrationImage;

		std::vector <Image * > images;
		std::vector <QStringList> filenames;
		std::vector <RigidBody * > rigidBodies;
		std::vector <Marker * > markers;

	};
}

	

#endif  // TRIAL_H
