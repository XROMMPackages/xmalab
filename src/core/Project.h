#ifndef PROJECT_H
#define PROJECT_H

#include <vector>
#include <QString>

namespace xma{
	class Camera;
	class Trial;

	class Project
	{
		friend class ProjectFileIO;

	public:
		static Project* getInstance();
		virtual ~Project();

		bool createNew();

		const std::vector<Camera *>& getCameras();
		const std::vector<Trial *>& getTrials();
		Trial * getTrialByName(QString Name);
		QString getProjectFilename();
		QString getProjectBasename();
		int getNbImagesCalibration();
		bool isCalibrated();
		void checkCalibration();

		void addCamera(Camera* cam);
		void addTrial(Trial* trial);
		void loadTextures();
		void exportDLT(QString foldername);
		void exportMayaCam(QString foldername);
		void exportLUT(QString foldername);
		void recountFrames();

	private:
		Project();
		static Project* instance;

		QString projectFilename;

		int nbImagesCalibration;
		bool calibrated;

		std::vector<Camera *> cameras;
		std::vector<Trial *> trials;
	};
}

#endif // PROJECT_H

