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
///\file ProjectFileIO.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef PROJECTFILEIO_H
#define PROJECTFILEIO_H

#include <QString>
#include <vector>
class QDir;
class QStringList;


namespace xma
{
	class NewProjectDialog;
	class NewTrialDialog;
	class Trial;

	class ProjectFileIO
	{
	public:
		static ProjectFileIO* getInstance();
		virtual ~ProjectFileIO();

		int saveProject(QString filename, std::vector <Trial*> trials, bool subset = false);
		int loadProject(QString filename);

		QStringList readTrials(QString filename);
		Trial* loadTrials(QString filename, QString trialname);
		void loadMarker(QString filename, QString trialname, Trial* trial);

		void loadXMAPortalTrial(QString filename, NewTrialDialog* dialog);
		void loadXMALabProject(QString filename, NewProjectDialog* dialog);
		void removeTmpDir();
		void loadProjectMetaData(QString xml_filename);
		void upgradeTo12(Trial *trial);
		void upgradeTo13(Trial *trial);

	private:

		ProjectFileIO();
		static ProjectFileIO* instance;

		bool writeProjectFile(QString filename,std::vector <Trial*> trials);
		bool readProjectFile(QString filename);

		bool zipFromFolderToFile(const QString& filePath, const QDir& dir, const QString& comment = QString(""));
		bool unzipFromFileToFolder(const QString& filePath, const QString& extDirPath, const QString& singleFileName = QString(""));
		void recurseAddDir(QDir d, QStringList& list);
		bool removeDir(QString folder);
	};
}


#endif // PROJECTFILEIO_H


