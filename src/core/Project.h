#ifndef PROJECT_H
#define PROJECT_H

#include <vector>
#include <QString>

class Camera;

class Project{
	friend class ProjectFileIO;

	public:
		static Project* getInstance();
		~Project();

		bool createNew();

		const std::vector <Camera *>& getCameras(){return cameras;}
		QString getProjectFilename(){return projectFilename;}
	
		int getNbImages(){return nbImages;}

		void addCamera(Camera * cam);
		void loadTextures();
		void exportDLT(QString foldername);
		void exportMayaCam(QString foldername);
		void exportLUT(QString foldername);
		void recountFrames();

	private:
		bool valid;
		QString projectFilename;

		Project();
		static Project* instance;		
		std::vector <Camera *> cameras;
		int nbImages;
};


	

#endif  // PROJECT_H
