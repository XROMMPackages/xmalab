//  ----------------------------------
//  XMALab -- Copyright � 2015, Brown University, Providence, RI.
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
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file Project.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef PROJECT_H
#define PROJECT_H

#include <vector>
#include <QString>
#include <QStringList>

namespace xma
{
	class Camera;
	class Trial;

	enum e_calibrationType
	{
		NO_CALIBRATION = 0,
		INTERNAL = 1,
		EXTERNAL = 2
	};

	class Project
	{
		friend class ProjectFileIO;

	public:
		static Project* getInstance();
		virtual ~Project();

		const std::vector<Camera *>& getCameras();
		const std::vector<Trial *>& getTrials();
		Trial* getTrialByName(QString Name);
		QString getProjectFilename();
		QString getProjectBasename();
		int getNbImagesCalibration();
		bool isCalibrated();
		void checkCalibration();

		void addCamera(Camera* cam);
		void addTrial(Trial* trial);
		void loadTextures();
		void reloadTextures();
		void exportDLT(QString foldername);
		void exportMayaCam(QString foldername, int frame = -1);
		void exportMayaCamVersion2(QString foldername, int frame = -1, int id = -1);
		void exportLUT(QString foldername);
		void recountFrames();
		void deleteTrial(Trial* trial);
		void replaceTrial(Trial* old_trial, Trial*  new_trial);

		void saveXMLData(QString filename);
		void setXMLData(QString filename);
		void parseXMLData(QString xmlData);
		int getFileID(QString filename);

		const QString getXMLData() const;

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


		bool hasDefaultTrial();
		Trial * getDefaultTrail();

		bool camerasOptimized();

		e_calibrationType getCalibration();
		void setCalibation(e_calibrationType type);

		void set_date_created();
		void set_date_created(QString & date)
		{
			date_created = date;
		}
		const QString &get_date_created()
		{
			return date_created;
		};

	private:
		Project();
		static Project* instance;

		QString projectFilename;
		e_calibrationType calibrationType;
		int nbImagesCalibration;
		bool calibrated;
		bool flipImages;

		std::vector<Camera *> cameras;
		std::vector<int> cameraIDs;

		std::vector<Trial *> trials;

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
		QString date_created;
	};
}

#endif // PROJECT_H


