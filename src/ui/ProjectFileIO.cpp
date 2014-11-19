#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/ProjectFileIO.h" 
#include "core/Project.h" 
#include "core/Camera.h" 
#include "core/CalibrationImage.h"
#include "core/UndistortionObject.h"
#include "core/CalibrationObject.h"
#include "ui/ErrorDialog.h"
#include "ui/ProgressDialog.h"
#include "ui/State.h"

#include <QDir>
#include <QXmlStreamWriter>
#include <QFileInfo>
#include <QFile>

#include "quazip.h"
#include "quazipfile.h"

#ifdef WIN32
#define OS_SEP "\\"
#else
#define OS_SEP "/"
#endif

ProjectFileIO* ProjectFileIO::instance = NULL;

ProjectFileIO::ProjectFileIO(){

}

ProjectFileIO::~ProjectFileIO(){
	instance = NULL;
}

ProjectFileIO* ProjectFileIO::getInstance()
{
	if(!instance) 
	{
		instance = new ProjectFileIO();
	}
	return instance;
}

bool ProjectFileIO::removeDir(QString folder){
	bool result = true;
    QDir dir(folder);

    if (dir.exists(folder)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = removeDir(info.absoluteFilePath());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(folder);
    }
    return result;
}

bool ProjectFileIO::saveProject(QString filename){
	bool success = true;
	Project::getInstance()->projectFilename = filename;

	QString tmpDir_path = QDir::tempPath() +  OS_SEP + "XROMM_tmp" +  OS_SEP;
	if(!QDir().mkpath ( tmpDir_path)){
		ErrorDialog::getInstance()->showErrorDialog("Can not create tmp folder " + tmpDir_path);
		success = false;
	}

	QDir tmpdir = QDir(tmpDir_path);

	if(success){
		success = writeProjectFile(tmpDir_path + "project.xml");
	}
	
	//Write Images 
	if(success){
		for(std::vector <Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it){
			QString camera_path = tmpDir_path + (*it)->getName() + OS_SEP;
			if(success){
				if(!QDir().mkpath (camera_path)){
					ErrorDialog::getInstance()->showErrorDialog("Can not create tmp folder " + camera_path);
					success = false;
				}else{
					QDir().mkpath (camera_path + OS_SEP + "data");
					(*it)->save(camera_path);
				}
			}
		}
	}

	//Save Calibration Object
	if(success){
		if(!CalibrationObject::getInstance()->isPlanar()){
			QString path = tmpDir_path + OS_SEP + "CalibrationObject" + OS_SEP;
			if(!QDir().mkpath (path)){
					ErrorDialog::getInstance()->showErrorDialog("Can not create tmp folder " + path);
					success = false;
			}else{
				CalibrationObject::getInstance()->saveCoords(path);
			}
		}
	}

	zipFromFolderToFile(filename, tmpDir_path,"XROMM Project File");

	removeDir(tmpDir_path);

	return success;
}

bool ProjectFileIO::loadProject(QString filename){
	bool success = true;
	Project::getInstance()->projectFilename = filename;
	QString tmpDir_path = QDir::tempPath() +  OS_SEP + "XROMM_tmp" ;

	unzipFromFileToFolder(filename,tmpDir_path);
	readProjectFile(tmpDir_path + OS_SEP + "project.xml");

	removeDir(tmpDir_path);

	return success;
}

bool ProjectFileIO::writeProjectFile(QString filename){
	QString xml_filename = filename;
	if (!xml_filename.isNull())
	{
		QFile file(xml_filename);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QXmlStreamWriter xmlWriter(&file);
			xmlWriter.writeStartDocument();
			xmlWriter.setAutoFormatting(true);
			xmlWriter.writeStartElement("Project");
			xmlWriter.writeAttribute("Version", "0.1");

			//Cameras
			for(std::vector <Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it){
				xmlWriter.writeStartElement("Camera");
				xmlWriter.writeAttribute("Name", (*it)->getName());
				xmlWriter.writeAttribute("isLightCamera", QString::number((*it)->isLightCamera()));
				xmlWriter.writeAttribute("isCalibrated", QString::number((*it)->isCalibrated()));
				if((*it)->isCalibrated()){
					xmlWriter.writeAttribute("CameraMatrix", (*it)->getName() + OS_SEP + "data" + OS_SEP + (*it)->getFilenameCameraMatrix());
				}

				if((*it)->hasUndistortion()){
					xmlWriter.writeStartElement("UndistortionGrid");
					xmlWriter.writeAttribute("Filename",  (*it)->getName() + OS_SEP + (*it)->getUndistortionObject()->getFilename());
					xmlWriter.writeAttribute("isComputed", QString::number((*it)->getUndistortionObject()->isComputed()));
					if((*it)->getUndistortionObject()->isComputed()){						
						if( (*it)->getUndistortionObject()->isCenterSet()){
							xmlWriter.writeStartElement("Center");
							xmlWriter.writeAttribute("x", QString::number((*it)->getUndistortionObject()->getCenter().x));
							xmlWriter.writeAttribute("y", QString::number((*it)->getUndistortionObject()->getCenter().y));
							xmlWriter.writeEndElement();
						}

						xmlWriter.writeStartElement("PointsDetected");
						xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + "data" + OS_SEP + (*it)->getUndistortionObject()->getFilenamePointsDetected());
						xmlWriter.writeEndElement();
						
						xmlWriter.writeStartElement("GridPointsDistorted");
						xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + "data" + OS_SEP + (*it)->getUndistortionObject()->getFilenameGridPointsDistorted());
						xmlWriter.writeEndElement();
						
						xmlWriter.writeStartElement("GridPointsReferences");
						xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + "data" + OS_SEP + (*it)->getUndistortionObject()->getFilenameGridPointsReferences());
						xmlWriter.writeEndElement();
						
						xmlWriter.writeStartElement("GridPointsInlier");
						xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + "data" + OS_SEP + (*it)->getUndistortionObject()->getFilenameGridPointsInlier());
						xmlWriter.writeEndElement();
					}
					xmlWriter.writeEndElement();
				}

				for(std::vector<CalibrationImage*>::const_iterator it2 = (*it)->getCalibrationImages().begin(); it2 != (*it)->getCalibrationImages().end(); ++it2){
					xmlWriter.writeStartElement("CalibrationImage");
					xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + (*it2)->getFilename());		
					xmlWriter.writeAttribute("isCalibrated", QString::number((*it2)->isCalibrated()));
					if((*it2)->isCalibrated()){						
						xmlWriter.writeStartElement("PointsDetectedAll");
						xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + "data" + OS_SEP + (*it2)->getFilenamePointsDetectedAll());
						xmlWriter.writeEndElement();
						
						xmlWriter.writeStartElement("PointsDetected");
						xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + "data" + OS_SEP + (*it2)->getFilenamePointsDetected());
						xmlWriter.writeEndElement();
						
						xmlWriter.writeStartElement("Inlier");
						xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + "data" + OS_SEP + (*it2)->getFilenamePointsInlier());
						xmlWriter.writeEndElement();

						xmlWriter.writeStartElement("RotationMatrix");
						xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + "data" + OS_SEP + (*it2)->getFilenameRotationMatrix());
						xmlWriter.writeEndElement();

						xmlWriter.writeStartElement("TranslationVector");
						xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + "data" + OS_SEP + (*it2)->getFilenameTranslationVector());
						xmlWriter.writeEndElement();
					}

					xmlWriter.writeEndElement();
				}

				xmlWriter.writeEndElement();
			}
	
			//CalibrationObject
			xmlWriter.writeStartElement("CalibrationObject");
			xmlWriter.writeAttribute("isPLanar", QString::number(CalibrationObject::getInstance()->isPlanar()));
			if(CalibrationObject::getInstance()->isPlanar()){
				xmlWriter.writeAttribute("HorizontalSquares", QString::number(CalibrationObject::getInstance()->getNbHorizontalSquares()));
				xmlWriter.writeAttribute("VerticalSquares", QString::number(CalibrationObject::getInstance()->getNbVerticalSquares()));
				xmlWriter.writeAttribute("SquareSize", QString::number(CalibrationObject::getInstance()->getSquareSize()));
			}else{
				QFileInfo frameSpecificationsFilenameInfo(CalibrationObject::getInstance()->getFrameSpecificationsFilename());
				QFileInfo referencesFilenameInfo(CalibrationObject::getInstance()->getReferencesFilename());
				xmlWriter.writeAttribute("FrameSpecifications", QString("CalibrationObject") + OS_SEP + frameSpecificationsFilenameInfo.fileName());
				xmlWriter.writeAttribute("References", QString("CalibrationObject") + OS_SEP + referencesFilenameInfo.fileName());
			}

			xmlWriter.writeEndDocument();
			file.close();
		}
	}
	return true;
}
		
bool ProjectFileIO::readProjectFile(QString filename){
	if ( filename.isNull() == false )
    {
		QFileInfo info(filename);
		QString basedir = info.absolutePath();
		QString xml_filename = filename;
		if (!xml_filename.isNull())
		{
			QFile file(xml_filename);
			if (file.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				QXmlStreamReader xml(&file);
				//Reading from the file

				 while(!xml.atEnd() && !xml.hasError()) {
					/* Read next element.*/
					QXmlStreamReader::TokenType token = xml.readNext();
					/* If token is just StartDocument, we'll go to next.*/
					if(token == QXmlStreamReader::StartDocument) {
						continue;
					}
					if(token == QXmlStreamReader::StartElement) {
						if (xml.name() == "Project")
						{
							QXmlStreamAttributes attr = xml.attributes() ;
							std::cout << "Load project file Version " << attr.value("Version").toString().toAscii().data() << std::endl;
						}
						if (xml.name() == "Camera")
						{
							QXmlStreamAttributes attr = xml.attributes() ;
							QString text = attr.value("Name").toString();
							Camera * cam = new Camera(text, Project::getInstance()->getCameras().size());
							cam->setIsLightCamera(attr.value("isLightCamera").toString().toInt());
							cam->setCalibrated(attr.value("isCalibrated").toString().toInt());
							
							if(cam->isCalibrated()){
								QXmlStreamAttributes attr = xml.attributes() ;
								text = attr.value("CameraMatrix").toString();
								cam->loadCameraMatrix( basedir + OS_SEP + text);
							}

							xml.readNext();
							while(!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "Camera")) {
								if(xml.tokenType() == QXmlStreamReader::StartElement) {
									if (xml.name() == "UndistortionGrid")
									{
										QXmlStreamAttributes attr = xml.attributes() ;
										text = attr.value("Filename").toString();
										cam->loadUndistortionImage( basedir + OS_SEP + text);
										cam->getUndistortionObject()->setComputed(attr.value("isComputed").toString().toInt());
										while(!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "UndistortionGrid")) {
											if(xml.tokenType() == QXmlStreamReader::StartElement) {		
												if (xml.name() == "Center"){
													QXmlStreamAttributes attr = xml.attributes() ;
													cam->getUndistortionObject()->setCenter(attr.value("x").toString().toDouble(),attr.value("y").toString().toDouble());
												}else if (xml.name() == "PointsDetected"){
													QXmlStreamAttributes attr = xml.attributes() ;
													text = attr.value("Filename").toString();
													cam->getUndistortionObject()->loadPointsDetected( basedir + OS_SEP + text);
												}else if(xml.name() == "GridPointsDistorted"){
													QXmlStreamAttributes attr = xml.attributes() ;
													text = attr.value("Filename").toString();
													cam->getUndistortionObject()->loadGridPointsDistorted( basedir + OS_SEP  + text);
												}else if(xml.name() == "GridPointsReferences"){
													QXmlStreamAttributes attr = xml.attributes() ;
													text = attr.value("Filename").toString();
													cam->getUndistortionObject()->loadGridPointsReferences( basedir + OS_SEP + text);
												}else if(xml.name() == "GridPointsInlier"){
													QXmlStreamAttributes attr = xml.attributes() ;
													text = attr.value("Filename").toString();
													cam->getUndistortionObject()->loadGridPointsInlier( basedir + OS_SEP + text);
												}
											}
											xml.readNext();		
										}
									}
									else if(xml.name() == "CalibrationImage"){
										QXmlStreamAttributes attr = xml.attributes() ;
										text = attr.value("Filename").toString();
										CalibrationImage * image = cam->addImage(basedir + OS_SEP + text);
										image->setCalibrated(attr.value("isCalibrated").toString().toInt());
										while(!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "CalibrationImage")) {
											if(xml.tokenType() == QXmlStreamReader::StartElement) {		
												if (xml.name() == "PointsDetectedAll"){
													QXmlStreamAttributes attr = xml.attributes() ;
													text = attr.value("Filename").toString();
													image->loadPointsDetectedAll( basedir + OS_SEP + text);
												}else if(xml.name() == "PointsDetected"){
													QXmlStreamAttributes attr = xml.attributes() ;
													text = attr.value("Filename").toString();
													image->loadPointsDetected( basedir + OS_SEP  + text);
												}else if(xml.name() == "Inlier"){
													QXmlStreamAttributes attr = xml.attributes() ;
													text = attr.value("Filename").toString();
													image->loadPointsInlier( basedir + OS_SEP + text);
												}else if(xml.name() == "RotationMatrix"){
													QXmlStreamAttributes attr = xml.attributes() ;
													text = attr.value("Filename").toString();
													image->loadRotationMatrix( basedir + OS_SEP  + text);
												}else if(xml.name() == "TranslationVector"){
													QXmlStreamAttributes attr = xml.attributes() ;
													text = attr.value("Filename").toString();
													image->loadTranslationVector( basedir + OS_SEP + text);
												}
											}
											xml.readNext();		
										}
									}
								}
								xml.readNext();		
							}

							if(!cam->setResolutions()){
								ErrorDialog::getInstance()->showErrorDialog(cam->getName() + " : Resolutions do not match");
								return false;
							}

							Project::getInstance()->addCamera(cam);
						}
						if (xml.name() == "CalibrationObject")
						{
							QXmlStreamAttributes attr = xml.attributes() ;
							bool isPlanar = attr.value("isPLanar").toString().toInt();

							if(isPlanar){
								CalibrationObject::getInstance()->setCheckerboard(
										attr.value("HorizontalSquares").toString().toInt(),
										attr.value("VerticalSquares").toString().toInt(),
										attr.value("SquareSize").toString().toInt()
									);
							}else{
								CalibrationObject::getInstance()->loadCoords(
										basedir + OS_SEP + attr.value("FrameSpecifications").toString(),
										basedir + OS_SEP + attr.value("References").toString()
									);
							}
						}
					}
				}
				if(xml.hasError()) {
					ErrorDialog::getInstance()->showErrorDialog(QString("QXSRExample::parseXML %1").arg(xml.errorString()));
				}
				file.close();
			}
		}
	}
	return true;
}

void ProjectFileIO::recurseAddDir(QDir d, QStringList & list) {

    QStringList qsl = d.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);

    foreach (QString file, qsl) {

        QFileInfo finfo(QString("%1/%2").arg(d.path()).arg(file));

        if (finfo.isSymLink())
            return;

        if (finfo.isDir()) {

            QString dirname = finfo.fileName();
            QDir sd(finfo.filePath());

            recurseAddDir(sd, list);

        } else
            list << QDir::toNativeSeparators(finfo.filePath());
    }
}

bool ProjectFileIO::zipFromFolderToFile(const QString & filePath, const QDir & dir, const QString & comment){
	//Make sure the zip file does not exist
	if (QFile::exists(filePath))
	{
		QFile::remove(filePath);
	}

	QuaZip zip(filePath);
	zip.setFileNameCodec("IBM866");

	if (!zip.open(QuaZip::mdCreate)) {
		ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile(): zip.open(): %1").arg(zip.getZipError()));
		return false;
	}

	if (!dir.exists()) {
		ErrorDialog::getInstance()->showErrorDialog( QString("dir.exists(%1)=FALSE").arg(dir.absolutePath()));
		return false;
	}

	QFile inFile;

	QStringList sl;
	recurseAddDir(dir, sl);

	QFileInfoList files;
	foreach (QString fn, sl) files << QFileInfo(fn);

	QuaZipFile outFile(&zip);

	char c;
	foreach(QFileInfo fileInfo, files) {

		if (!fileInfo.isFile())
			continue;

		QString fileNameWithRelativePath = fileInfo.filePath().remove(0, dir.absolutePath().length() + 1);

		inFile.setFileName(fileInfo.filePath());

		if (!inFile.open(QIODevice::ReadOnly)) {
			ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile(): inFile.open(): %1").arg(inFile.errorString().toLocal8Bit().constData()));
			return false;
		}

		if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileNameWithRelativePath, fileInfo.filePath()))) {
			ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile(): outFile.open(): %1").arg(outFile.getZipError()));
			return false;
		}

		while (inFile.getChar(&c) && outFile.putChar(c));

		if (outFile.getZipError() != UNZ_OK) {
			ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile(): outFile.putChar(): %1").arg(outFile.getZipError()));
			return false;
		}

		outFile.close();

		if (outFile.getZipError() != UNZ_OK) {
			ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile(): outFile.close(): %1").arg(outFile.getZipError()));
			return false;
		}

		inFile.close();
	}


	if (!comment.isEmpty())
		zip.setComment(comment);

	zip.close();

	if (zip.getZipError() != 0) {
		ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile(): zip.close(): %1").arg(zip.getZipError()));
		return false;
	}

	return true;
}

bool ProjectFileIO::unzipFromFileToFolder(const QString & filePath, const QString & extDirPath, const QString & singleFileName) {
	//Make sure the folder is empty
	removeDir(extDirPath);

	QuaZip zip(filePath);

	if (!zip.open(QuaZip::mdUnzip)) {
		ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile():  zip.open(): %1").arg(zip.getZipError()));
		return false;
	}

	zip.setFileNameCodec("IBM866");

	qWarning("%d entries\n", zip.getEntriesCount());
	qWarning("Global comment: %s\n", zip.getComment().toLocal8Bit().constData());

	QuaZipFileInfo info;

	QuaZipFile file(&zip);

	QFile out;
	QString name;
	char c;
	for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {

		if (!zip.getCurrentFileInfo(&info)) {
			ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile():  getCurrentFileInfo(): %1\n").arg(zip.getZipError()));
			return false;
		}

		if (!singleFileName.isEmpty())
			if (!info.name.contains(singleFileName))
				continue;

		if (!file.open(QIODevice::ReadOnly)) {
			ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile():  file.open(): %1").arg(file.getZipError()));
			return false;
		}

		name = QString("%1/%2").arg(extDirPath,file.getActualFileName());

		if (file.getZipError() != UNZ_OK) {
			ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile():  file.getFileName(): %1").arg(file.getZipError()));
			return false;
		}

		//out.setFileName("out/" + name);
		out.setFileName(name);
		QFileInfo info (name);
		QDir().mkpath(info.absolutePath());

		// this will fail if "name" contains subdirectories, but we don't mind that
		out.open(QIODevice::WriteOnly);
		// Slow like hell (on GNU/Linux at least), but it is not my fault.
		// Not ZIP/UNZIP package's fault either.
		// The slowest thing here is out.putChar(c).
		while (file.getChar(&c)) out.putChar(c);

		out.close();

		if (file.getZipError() != UNZ_OK) {
			ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile():  file.getFileName(): %1").arg(file.getZipError()));
			return false;
		}

		if (!file.atEnd()) {
			ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile():  read all but not EOF"));
			return false;
		}

		file.close();

		if (file.getZipError() != UNZ_OK) {
			ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile():  file.close(): %1").arg(file.getZipError()));
			return false;
		}
	}

	zip.close();

	if (zip.getZipError() != UNZ_OK) {
		ErrorDialog::getInstance()->showErrorDialog( QString("zipFromFolderToFile():  zip.close(): %1").arg(zip.getZipError()));
		return false;
	}

	return true;

}