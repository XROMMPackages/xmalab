#ifndef PROJECTFILEIO_H
#define PROJECTFILEIO_H

#include <QString>
class QDir;
class QStringList;

namespace xma{
	class ProjectFileIO{

	public:
		static ProjectFileIO* getInstance();
		virtual ~ProjectFileIO();

		bool saveProject(QString filename);
		bool loadProject(QString filename);

	private:

		ProjectFileIO();
		static ProjectFileIO* instance;

		bool writeProjectFile(QString filename);
		bool readProjectFile(QString filename);

		bool zipFromFolderToFile(const QString & filePath, const QDir & dir, const QString & comment = QString(""));
		bool unzipFromFileToFolder(const QString & filePath, const QString & extDirPath, const QString & singleFileName = QString(""));
		void recurseAddDir(QDir d, QStringList & list);
		void correctFolder(QString filename, QString &newfolder, QString &oldfolder);
		bool removeDir(QString folder);
	};
}

	

#endif  // PROJECTFILEIO_H
