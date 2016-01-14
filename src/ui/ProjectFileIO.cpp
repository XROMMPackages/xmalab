//  ----------------------------------
//  XMA Lab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMA Lab software is provided under the terms of the GNU General Public License version 3 
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
///\file ProjectFileIO.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/ProjectFileIO.h" 

#include "ui/ErrorDialog.h"
#include "ui/ProgressDialog.h"
#include "ui/State.h"
#include "ui/ConsoleDockWidget.h"
#include "ui/WorkspaceNavigationFrame.h"
#include "ui/NewProjectDialog.h"
#include "ui/NewTrialDialog.h"

#include "core/Project.h" 
#include "core/Camera.h" 
#include "core/Trial.h" 
#include "core/Marker.h" 
#include "core/RigidBody.h" 
#include "core/CalibrationImage.h"
#include "core/UndistortionObject.h"
#include "core/CalibrationObject.h"
#include "core/HelperFunctions.h"
#include "core/Settings.h"

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


using namespace xma;

ProjectFileIO* ProjectFileIO::instance = NULL;

ProjectFileIO::ProjectFileIO()
{
}

ProjectFileIO::~ProjectFileIO()
{
	instance = NULL;
}

ProjectFileIO* ProjectFileIO::getInstance()
{
	if (!instance)
	{
		instance = new ProjectFileIO();
	}
	return instance;
}

bool ProjectFileIO::removeDir(QString folder)
{
	bool result = true;
	QDir dir(folder);

	if (dir.exists(folder))
	{
		Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst))
			{
				if (info.isDir())
				{
					result = removeDir(info.absoluteFilePath());
				}
				else
				{
					result = QFile::remove(info.absoluteFilePath());
				}

				if (!result)
				{
					return result;
				}
			}
		result = dir.rmdir(folder);
	}
	return result;
}

int ProjectFileIO::saveProject(QString filename)
{
	bool success = true;
	Project::getInstance()->projectFilename = filename;

	QString tmpDir_path = QDir::tempPath() + OS_SEP + "XROMM_tmp" + OS_SEP;
	if (!QDir().mkpath(tmpDir_path))
	{
		ErrorDialog::getInstance()->showErrorDialog("Can not create tmp folder " + tmpDir_path);
		success = false;
	}

	QDir tmpdir = QDir(tmpDir_path);

	if (success)
	{
		success = writeProjectFile(tmpDir_path + "project.xml");
	}

	if (success)
	{
		if (Project::getInstance()->getHasStudyData()){
			QString path = tmpDir_path + "projectMetaData";
			if (!QDir().mkpath(path))
			{
				ErrorDialog::getInstance()->showErrorDialog("Can not create tmp folder " + path);
				success = false;
			}
			Project::getInstance()->saveXMLData(path + OS_SEP + "metadata.xml");
		}
	}

	//Write Images 
	if (success)
	{
		for (std::vector<Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it)
		{
			QString camera_path = tmpDir_path + (*it)->getName() + OS_SEP;
			if (success)
			{
				if (!QDir().mkpath(camera_path))
				{
					ErrorDialog::getInstance()->showErrorDialog("Can not create tmp folder " + camera_path);
					success = false;
				}
				else
				{
					QDir().mkpath(camera_path + OS_SEP + "data");
					(*it)->save(camera_path);
				}
			}
		}
	}

	//Save Calibration Object
	if (success)
	{
		if (!CalibrationObject::getInstance()->isCheckerboard())
		{
			QString path = tmpDir_path + OS_SEP + "CalibrationObject" + OS_SEP;
			if (!QDir().mkpath(path))
			{
				ErrorDialog::getInstance()->showErrorDialog("Can not create tmp folder " + path);
				success = false;
			}
			else
			{
				CalibrationObject::getInstance()->saveCoords(path);
			}
		}
	}

	if (success)
	{
		for (unsigned int i = 0; i < Project::getInstance()->getTrials().size(); i++)
		{
			QString path = tmpDir_path + OS_SEP + Project::getInstance()->getTrials()[i]->getName() + OS_SEP;
			if (!QDir().mkpath(path))
			{
				ErrorDialog::getInstance()->showErrorDialog("Can not create tmp folder " + path);
				success = false;
			}
			else
			{

				if (Project::getInstance()->getTrials()[i]->getHasStudyData()){
					Project::getInstance()->getTrials()[i]->saveXMLData(path + OS_SEP + "metadata.xml");
				}

				Project::getInstance()->getTrials()[i]->save(path);
				QDir().mkpath(path + OS_SEP + "data");
				Project::getInstance()->getTrials()[i]->saveMarkers(path + OS_SEP + "data" + OS_SEP + "MarkerDescription.txt");
				Project::getInstance()->getTrials()[i]->saveRigidBodies(path + OS_SEP + "data" + OS_SEP + "RigidBodies.txt");
				for (unsigned int k = 0; k < Project::getInstance()->getTrials()[i]->getMarkers().size(); k++)
				{
					Project::getInstance()->getTrials()[i]->getMarkers()[k]->save(path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "points2d.csv",
					                                                              path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "status2d.csv",
					                                                              path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "size.csv");
					Project::getInstance()->getTrials()[i]->getMarkers()[k]->save3DPoints(path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "points3d.csv",
					                                                                      path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "status3d.csv");

					if (Project::getInstance()->getTrials()[i]->getMarkers()[k]->Reference3DPointSet())
					{
						Project::getInstance()->getTrials()[i]->getMarkers()[k]->saveReference3DPoint(path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "reference3Dpoint.csv");
					}
				}
				for (unsigned int k = 0; k < Project::getInstance()->getTrials()[i]->getRigidBodies().size(); k++)
				{
					if (Project::getInstance()->getTrials()[i]->getRigidBodies()[k]->isReferencesSet() == 2)
					{
						Project::getInstance()->getTrials()[i]->getRigidBodies()[k]->save(
							path + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "ReferenceNames.csv",
							path + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "ReferencePoints3d.csv");
					}
					for (unsigned int p = 0; p < Project::getInstance()->getTrials()[i]->getRigidBodies()[k]->getDummyNames().size(); p++)
					{
						Project::getInstance()->getTrials()[i]->getRigidBodies()[k]->saveDummy(p,
						                                                                       path + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "DummyMarker" + QString().sprintf("%03d", p) + "PointReferences.csv",
																							   path + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "DummyMarker" + QString().sprintf("%03d", p) + "PointReferences2.csv",
						                                                                       path + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "DummyMarker" + QString().sprintf("%03d", p) + "PointCoordinates.csv");
					}
				}
			}
		}
	}

	//save Log
	ConsoleDockWidget::getInstance()->save(tmpDir_path + OS_SEP + "log.html");

	zipFromFolderToFile(filename, tmpDir_path, "XROMM Project File");

	removeDir(tmpDir_path);

	return success ? 0 : -1;
}

int ProjectFileIO::loadProject(QString filename)
{
	bool success = true;
	Project::getInstance()->projectFilename = filename;
	QString tmpDir_path = QDir::tempPath() + OS_SEP + "XROMM_tmp";

	unzipFromFileToFolder(filename, tmpDir_path);
	
	if (QFile::exists(tmpDir_path + OS_SEP + "project.xml"))
	{
		readProjectFile(tmpDir_path + OS_SEP + "project.xml");
		ConsoleDockWidget::getInstance()->load(tmpDir_path + OS_SEP + "log.html");
	}
	else
	{
		QFileInfo info(filename);
		if (QFile::exists(tmpDir_path + OS_SEP + info.completeBaseName() + OS_SEP + "File_Metadata" + OS_SEP + "XMALab_Files-metadata.xml"))
		{
			return 1;
		}
		else
		{
			removeDir(tmpDir_path);
			return -1;
		}
	}

	removeDir(tmpDir_path);

	return 0;
}

QStringList ProjectFileIO::readTrials(QString filename)
{
	QStringList names;

	QString tmpDir_path = QDir::tempPath() + OS_SEP + "XROMM_tmp";

	unzipFromFileToFolder(filename, tmpDir_path);

	if (QFile::exists(tmpDir_path + OS_SEP + "project.xml"))
	{
		if (filename.isNull() == false)
		{
			QFileInfo info(filename);
			QString basedir = info.absolutePath();
			QString xml_filename = tmpDir_path + OS_SEP + "project.xml";
			if (!xml_filename.isNull())
			{
				QFile file(xml_filename);
				if (file.open(QIODevice::ReadOnly | QIODevice::Text))
				{
					QXmlStreamReader xml(&file);
					//Reading from the file

					while (!xml.atEnd() && !xml.hasError())
					{
						/* Read next element.*/
						QXmlStreamReader::TokenType token = xml.readNext();
						/* If token is just StartDocument, we'll go to next.*/
						if (token == QXmlStreamReader::StartDocument)
						{
							continue;
						}
						if (token == QXmlStreamReader::StartElement)
						{
							if (xml.name() == "Trial")
							{
								QXmlStreamAttributes attr = xml.attributes();
								names << attr.value("Name").toString();
							}
						}
					}
					if (xml.hasError())
					{
						ErrorDialog::getInstance()->showErrorDialog(QString("QXSRExample::parseXML %1").arg(xml.errorString()));
					}
					file.close();
				}
			}
		}
	}

	removeDir(tmpDir_path);
	return names;
}

Trial* ProjectFileIO::loadTrials(QString filename, QString trialname)
{
	Trial* trial;
	QString tmpDir_path = QDir::tempPath() + OS_SEP + "XROMM_tmp";

	unzipFromFileToFolder(filename, tmpDir_path);

	if (QFile::exists(tmpDir_path + OS_SEP + "project.xml"))
	{
		if (filename.isNull() == false)
		{
			QFileInfo info(tmpDir_path + OS_SEP + "project.xml");
			QString basedir = info.absolutePath();
			QString xml_filename = tmpDir_path + OS_SEP + "project.xml";
			if (!xml_filename.isNull())
			{
				QFile file(xml_filename);
				if (file.open(QIODevice::ReadOnly | QIODevice::Text))
				{
					QXmlStreamReader xml(&file);
					//Reading from the file

					while (!xml.atEnd() && !xml.hasError())
					{
						/* Read next element.*/
						QXmlStreamReader::TokenType token = xml.readNext();
						/* If token is just StartDocument, we'll go to next.*/
						if (token == QXmlStreamReader::StartDocument)
						{
							continue;
						}
						if (token == QXmlStreamReader::StartElement)
						{
							if (xml.name() == "Trial")
							{
								QXmlStreamAttributes attr = xml.attributes();
								if (attr.value("Name").toString() == trialname)
								{
									QString trialname = attr.value("Name").toString();
									QString trialfolder = basedir + OS_SEP + trialname + OS_SEP;
									trial = new Trial(trialname, littleHelper::adjustPathToOS(trialfolder));

									int startFrame = attr.value("startFrame").toString().toInt();
									trial->setStartFrame(startFrame);
									int endFrame = attr.value("endFrame").toString().toInt();
									trial->setEndFrame(endFrame);
									int referenceCalibration = attr.value("referenceCalibration").toString().toInt();
									trial->setReferenceCalibrationImage(referenceCalibration);

									QString recordingSpeed = attr.value("recordingSpeed").toString();
									if (!recordingSpeed.isEmpty())trial->setRecordingSpeed(recordingSpeed.toDouble());

									QString cutOffFrequency = attr.value("cutOffFrequency").toString();
									if (!cutOffFrequency.isEmpty())trial->setCutoffFrequency(cutOffFrequency.toDouble());

									QString interpolateMissingFrames = attr.value("interpolateMissingFrames").toString();
									if (!interpolateMissingFrames.isEmpty())trial->setInterpolateMissingFrames(interpolateMissingFrames.toInt());
									trial->loadMarkers(trialfolder + OS_SEP + "data" + OS_SEP + "MarkerDescription.txt");

									trial->loadRigidBodies(trialfolder + OS_SEP + "data" + OS_SEP + "RigidBodies.txt");

									QString xml_file = attr.value("MetaData").toString();
									if (!xml_file.isEmpty())
									{
										loadProjectMetaData(littleHelper::adjustPathToOS(trialfolder + OS_SEP + xml_file));
										trial->setXMLData(littleHelper::adjustPathToOS(trialfolder + OS_SEP + xml_file));
										trial->parseXMLData();
									}

									while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "Trial"))
									{
										if (xml.tokenType() == QXmlStreamReader::StartElement)
										{
											if (xml.name() == "Marker")
											{
												QXmlStreamAttributes attr = xml.attributes();
												QString filename_points2D = basedir + OS_SEP + attr.value("FilenamePoints2D").toString();
												QString filename_status2D = basedir + OS_SEP + attr.value("FilenameStatus2D").toString();
												QString filename_size = basedir + OS_SEP + attr.value("FilenameSize").toString();

												int id = attr.value("ID").toString().toInt();
												trial->getMarkers()[id]->load(littleHelper::adjustPathToOS(filename_points2D), littleHelper::adjustPathToOS(filename_status2D), littleHelper::adjustPathToOS(filename_size));

												QString Reference3DPoint = attr.value("Reference3DPoint").toString();
												if (!Reference3DPoint.isEmpty())trial->getMarkers()[id]->loadReference3DPoint(littleHelper::adjustPathToOS(basedir + OS_SEP + Reference3DPoint));

												QString TrackingPenalty = attr.value("TrackingPenalty").toString();
												if (!TrackingPenalty.isEmpty())trial->getMarkers()[id]->setMaxPenalty(TrackingPenalty.toInt());

												QString DetectionMethod = attr.value("DetectionMethod").toString();
												if (!DetectionMethod.isEmpty())trial->getMarkers()[id]->setMethod(DetectionMethod.toInt());

												QString SizeOverride = attr.value("SizeOverride").toString();
												if (!SizeOverride.isEmpty())trial->getMarkers()[id]->setSizeOverride(SizeOverride.toInt());

												QString ThresholdOffset = attr.value("ThresholdOffset").toString();
												if (!ThresholdOffset.isEmpty())trial->getMarkers()[id]->setThresholdOffset(ThresholdOffset.toInt());
											}

											if (xml.name() == "RigidBody")
											{
												QXmlStreamAttributes attr = xml.attributes();

												QString filename_referenceNames_attr = attr.value("ReferenceNames").toString();
												QString filename_referencePoints3D_attr = attr.value("ReferencePoints3D").toString();

												int id = attr.value("ID").toString().toInt();

												if (!filename_referenceNames_attr.isEmpty() && !filename_referencePoints3D_attr.isEmpty())
												{
													QString filename_referenceNames = basedir + OS_SEP + filename_referenceNames_attr;
													QString filename_referencePoints3D = basedir + OS_SEP + filename_referencePoints3D_attr;
													trial->getRigidBodies()[id]->load(littleHelper::adjustPathToOS(filename_referenceNames), littleHelper::adjustPathToOS(filename_referencePoints3D));
												}
												else
												{
													trial->getRigidBodies()[id]->resetReferences();
												}

												QString visible = attr.value("Visible").toString();
												if (!visible.isEmpty())
												{
													trial->getRigidBodies()[id]->setVisible(visible.toInt());
												}
												QString color = attr.value("Color").toString();
												if (!color.isEmpty())
												{
													trial->getRigidBodies()[id]->setColor(QColor(color));
												}

												QString overrideCutOffFrequency = attr.value("OverrideCutOffFrequency").toString();
												if (!overrideCutOffFrequency.isEmpty())
												{
													trial->getRigidBodies()[id]->setOverrideCutoffFrequency(overrideCutOffFrequency.toInt());
												}

												QString cutOffFrequency = attr.value("CutOffFrequency").toString();
												if (!cutOffFrequency.isEmpty())
												{
													trial->getRigidBodies()[id]->setCutoffFrequency(cutOffFrequency.toDouble());
												}


												while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "RigidBody"))
												{
													if (xml.tokenType() == QXmlStreamReader::StartElement)
													{
														if (xml.name() == "DummyMarker")
														{
															QXmlStreamAttributes attr = xml.attributes();

															QString dummyName = attr.value("Name").toString();
															QString dummyPointReferences = basedir + OS_SEP + attr.value("PointReferences").toString();
															QString dummyPointReferences2 = attr.value("PointReferences2").toString();
															if (dummyPointReferences2.isEmpty())
															{
																dummyPointReferences2 = dummyPointReferences;
															} else
															{
																dummyPointReferences2 = basedir + OS_SEP + dummyPointReferences2;
															}
															QString dummyPointCoordinates = basedir + OS_SEP + attr.value("PointCoordinates").toString();

															trial->getRigidBodies()[id]->addDummyPoint(dummyName, littleHelper::adjustPathToOS(dummyPointReferences), littleHelper::adjustPathToOS(dummyPointReferences2), 1, littleHelper::adjustPathToOS(dummyPointCoordinates));
														}
													}
													xml.readNext();
												}
											}
										}
										xml.readNext();
									}
								}
							}
						}
					}
					if (xml.hasError())
					{
						ErrorDialog::getInstance()->showErrorDialog(QString("QXSRExample::parseXML %1").arg(xml.errorString()));
					}
					file.close();
				}
			}
		}
	}

	removeDir(tmpDir_path);
	
	return trial;
}

void ProjectFileIO::loadMarker(QString filename, QString trialname, Trial* trial)
{
	QString tmpDir_path = QDir::tempPath() + OS_SEP + "XROMM_tmp";

	unzipFromFileToFolder(filename, tmpDir_path);

	if (QFile::exists(tmpDir_path + OS_SEP + "project.xml"))
	{
		if (filename.isNull() == false)
		{
			QFileInfo info(tmpDir_path + OS_SEP + "project.xml");
			QString basedir = info.absolutePath();
			QString xml_filename = tmpDir_path + OS_SEP + "project.xml";
			if (!xml_filename.isNull())
			{
				QFile file(xml_filename);
				if (file.open(QIODevice::ReadOnly | QIODevice::Text))
				{
					QXmlStreamReader xml(&file);
					//Reading from the file

					while (!xml.atEnd() && !xml.hasError())
					{
						/* Read next element.*/
						QXmlStreamReader::TokenType token = xml.readNext();
						/* If token is just StartDocument, we'll go to next.*/
						if (token == QXmlStreamReader::StartDocument)
						{
							continue;
						}
						if (token == QXmlStreamReader::StartElement)
						{
							if (xml.name() == "Trial")
							{
								QXmlStreamAttributes attr = xml.attributes();
								if (attr.value("Name").toString() == trialname)
								{
									QString trialname = attr.value("Name").toString();
									QString trialfolder = basedir + OS_SEP + trialname + OS_SEP;

									trial->loadMarkers(trialfolder + OS_SEP + "data" + OS_SEP + "MarkerDescription.txt");
									trial->loadRigidBodies(trialfolder + OS_SEP + "data" + OS_SEP + "RigidBodies.txt");

									while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "Trial"))
									{
										if (xml.tokenType() == QXmlStreamReader::StartElement)
										{
											if (xml.name() == "Marker")
											{
												QXmlStreamAttributes attr = xml.attributes();
												int id = attr.value("ID").toString().toInt();

												QString Reference3DPoint = basedir + OS_SEP + attr.value("Reference3DPoint").toString();
												if (!Reference3DPoint.isEmpty())trial->getMarkers()[id]->loadReference3DPoint(littleHelper::adjustPathToOS(Reference3DPoint));

												QString TrackingPenalty = attr.value("TrackingPenalty").toString();
												if (!TrackingPenalty.isEmpty())trial->getMarkers()[id]->setMaxPenalty(TrackingPenalty.toInt());

												QString DetectionMethod = attr.value("DetectionMethod").toString();
												if (!DetectionMethod.isEmpty())trial->getMarkers()[id]->setMethod(DetectionMethod.toInt());

												QString SizeOverride = attr.value("SizeOverride").toString();
												if (!SizeOverride.isEmpty())trial->getMarkers()[id]->setSizeOverride(SizeOverride.toInt());

												QString ThresholdOffset = attr.value("ThresholdOffset").toString();
												if (!ThresholdOffset.isEmpty())trial->getMarkers()[id]->setThresholdOffset(ThresholdOffset.toInt());
											}

											if (xml.name() == "RigidBody")
											{
												QXmlStreamAttributes attr = xml.attributes();

												QString filename_referenceNames_attr = attr.value("ReferenceNames").toString();
												QString filename_referencePoints3D_attr = attr.value("ReferencePoints3D").toString();

												int id = attr.value("ID").toString().toInt();

												if (!filename_referenceNames_attr.isEmpty() && !filename_referencePoints3D_attr.isEmpty())
												{
													QString filename_referenceNames = basedir + OS_SEP + filename_referenceNames_attr;
													QString filename_referencePoints3D = basedir + OS_SEP + filename_referencePoints3D_attr;
													trial->getRigidBodies()[id]->load(littleHelper::adjustPathToOS(filename_referenceNames), littleHelper::adjustPathToOS(filename_referencePoints3D));
												}
												else
												{
													trial->getRigidBodies()[id]->resetReferences();
												}

												QString visible = attr.value("Visible").toString();
												if (!visible.isEmpty())
												{
													trial->getRigidBodies()[id]->setVisible(visible.toInt());
												}
												QString color = attr.value("Color").toString();
												if (!color.isEmpty())
												{
													trial->getRigidBodies()[id]->setColor(QColor(color));
												}

												QString overrideCutOffFrequency = attr.value("OverrideCutOffFrequency").toString();
												if (!overrideCutOffFrequency.isEmpty())
												{
													trial->getRigidBodies()[id]->setOverrideCutoffFrequency(overrideCutOffFrequency.toInt());
												}

												QString cutOffFrequency = attr.value("CutOffFrequency").toString();
												if (!cutOffFrequency.isEmpty())
												{
													trial->getRigidBodies()[id]->setCutoffFrequency(cutOffFrequency.toDouble());
												}
											}
										}
										xml.readNext();
									}
								}
							}
						}
					}
					if (xml.hasError())
					{
						ErrorDialog::getInstance()->showErrorDialog(QString("QXSRExample::parseXML %1").arg(xml.errorString()));
					}
					file.close();
				}
			}
		}
	}

	removeDir(tmpDir_path);
}

void ProjectFileIO::loadXMAPortalTrial(QString filename, NewTrialDialog* dialog)
{
	QString tmpDir_path = QDir::tempPath() + OS_SEP + "XROMM_tmp";
	unzipFromFileToFolder(filename, tmpDir_path);
	QString studyName;
	QString trialName;

	std::vector<int> camera_vec;
	std::vector<QString> filename_vec;

	QFileInfo info(filename);
	if (QFile::exists(tmpDir_path + OS_SEP + info.completeBaseName() + OS_SEP + "File_Metadata" + OS_SEP + "XMALab_Files-metadata.xml"))
	{
		QString xml_filename = tmpDir_path + OS_SEP + info.completeBaseName() + OS_SEP + "File_Metadata" + OS_SEP + "XMALab_Files-metadata.xml";
		
		if (xml_filename.isNull() == false)
		{
			QString basedir = tmpDir_path + OS_SEP + info.completeBaseName() + OS_SEP;

			QFile file(xml_filename);
			if (file.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				QXmlStreamReader xml(&file);
				while (!xml.atEnd() && !xml.hasError())
				{
					QXmlStreamReader::TokenType token = xml.readNext();

					if (token == QXmlStreamReader::StartDocument)
					{
						continue;
					}

					if (token == QXmlStreamReader::StartElement)
					{
						if (xml.name() == "StudyName")
						{
							studyName = xml.readElementText();
						}
						else if (xml.name() == "TrialName")
						{
							trialName = xml.readElementText();
						}

						else if (xml.name() == "File")
						{
							QString imagename = "";
							int camera = -1;
							while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "File"))
							{
								if (xml.tokenType() == QXmlStreamReader::StartElement)
								{
									if (xml.name() == "Filename")
									{
										imagename = xml.readElementText();
									}
									
									else if (xml.name() == "metadata")
									{
										while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "metadata"))
										{
											if (xml.tokenType() == QXmlStreamReader::StartElement)
											{
												if (xml.name() == "cameraNumber")
												{
													camera = xml.readElementText().replace("cam", "").toInt() - 1;
												}
											}
											xml.readNext();
										}
									}
								}
								xml.readNext();
							}
							if (camera >= 0 && !imagename.isEmpty())
							{
								camera_vec.push_back(camera);
								filename_vec.push_back(imagename);
							}
						}
					}
				}
			}
		}

		//copy to workspace
		QString folder;
		if (Settings::getInstance()->getBoolSetting("CustomWorkspacePath"))
		{
			folder = Settings::getInstance()->getQStringSetting("WorkspacePath");
			folder = folder + OS_SEP + studyName;
			QDir().mkpath(folder);
			folder = folder + OS_SEP + trialName;
			QDir().mkpath(folder);
		} 
		else
		{

			folder = QFileInfo(Project::getInstance()->getProjectFilename()).absolutePath();
			folder = folder + OS_SEP + trialName;
			QDir().mkpath(folder);
		}
		littleHelper::copyPath(tmpDir_path + OS_SEP + info.completeBaseName(), folder);
		//Fill Dialog

		//check for zip


		for (unsigned int i = 0; i < camera_vec.size(); i++)
		{
			if (filename_vec[i].endsWith("zip"))
			{
				QString videoPath = QString(folder + OS_SEP + filename_vec[i]).replace(".zip", "");
				unzipFromFileToFolder(folder + OS_SEP + filename_vec[i], videoPath);
				QFile(folder + OS_SEP + filename_vec[i]).remove();
			}
			dialog->setTrialName(trialName);
			dialog->setCam(camera_vec[i], folder + OS_SEP + filename_vec[i]);
		}
		dialog->setXmlMetadata(folder + OS_SEP + "File_Metadata" + OS_SEP + "XMALab_Files-metadata.xml");
	} 
	else
	{
		ErrorDialog::getInstance()->showErrorDialog("Not a valid xma file from the portal");
	}
	removeDir(tmpDir_path);
}

void ProjectFileIO::loadXMALabProject(QString filename, NewProjectDialog* dialog)
{
	Project::getInstance()->projectFilename = filename;
	QString tmpDir_path = QDir::tempPath() + OS_SEP + "XROMM_tmp";

	QFileInfo infoDir(filename);
	QString xml_filename = tmpDir_path + OS_SEP + infoDir.completeBaseName() + OS_SEP + "File_Metadata" + OS_SEP + "XMALab_Files-metadata.xml";
	
	loadProjectMetaData(xml_filename);

	std::vector<int> camera_vec;
	std::vector<int> type_vec;
	std::vector<QString> filename_vec;

	if (xml_filename.isNull() == false)
	{
		QString basedir = tmpDir_path + OS_SEP + infoDir.completeBaseName() + OS_SEP;

		QFile file(xml_filename);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QXmlStreamReader xml(&file);
			//Reading from the file

			while (!xml.atEnd() && !xml.hasError())
			{
				QXmlStreamReader::TokenType token = xml.readNext();
				if (token == QXmlStreamReader::StartDocument)
				{
					continue;
				}

				if (token == QXmlStreamReader::StartElement)
				{
					if (xml.name() == "File")
					{
						QString imagename = "";
						int type = -1;
						int camera = -1;
						while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "File"))
						{
							if (xml.tokenType() == QXmlStreamReader::StartElement)
							{
								if (xml.name() == "Filename")
								{
									imagename = basedir + xml.readElementText();
								}
								else if (xml.name() == "FileCategory")
								{
									QString text = xml.readElementText();
									if (text == "XCalibObject")
									{
										type = 1;
									}
									else if (text == "XCalibGrid")
									{
										type = 2;
									}
								}
								else if (xml.name() == "metadata")
								{
									while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "metadata"))
									{
										if (xml.tokenType() == QXmlStreamReader::StartElement)
										{
											if (xml.name() == "cameraNumber")
											{
												camera = xml.readElementText().replace("cam", "").toInt() - 1;
											}
										}
										xml.readNext();
									}
								}
							}
							xml.readNext();
						}
						if (camera >= 0 && type >= 0 && !imagename.isEmpty())
						{
							camera_vec.push_back(camera);
							type_vec.push_back(type);
							filename_vec.push_back(imagename);
						}
					}
				}
			}
		}
	}

	for (unsigned int i = 0; i < camera_vec.size(); i++)
	{
		if (type_vec[i] == 1)
		{
			dialog->addCalibrationImage(camera_vec[i], filename_vec[i]);
		}
		else if (type_vec[i] == 2)
		{
			dialog->addGridImage(camera_vec[i], filename_vec[i]);
		}
	}

	//check for calibration csv and ref
	QDir directory(tmpDir_path + OS_SEP + infoDir.completeBaseName());
	QStringList filtersCSV;
	filtersCSV << "*.csv";
	QStringList csvFiles = directory.entryList(filtersCSV);
	if (csvFiles.size() == 1)
	{
		dialog->setCalibrationCubeCSV(tmpDir_path + OS_SEP + infoDir.completeBaseName() + OS_SEP + csvFiles.at(0));
	}

	QStringList filterREF;
	filterREF << "*.ref";
	QStringList refFiles = directory.entryList(filterREF);
	if (refFiles.size() == 1)
	{
		dialog->setCalibrationCubeREF(tmpDir_path + OS_SEP + infoDir.completeBaseName() + OS_SEP + refFiles.at(0));
	}
}

void ProjectFileIO::loadProjectMetaData(QString xml_filename)
{
	if (xml_filename.isNull() == false)
	{
		Project::getInstance()->setXMLData(xml_filename);
	}
}

void ProjectFileIO::removeTmpDir()
{
	QString tmpDir_path = QDir::tempPath() + OS_SEP + "XROMM_tmp" + OS_SEP;
	removeDir(tmpDir_path);
}

bool ProjectFileIO::writeProjectFile(QString filename)
{
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
			xmlWriter.writeAttribute("ActiveTrial", QString::number(State::getInstance()->getActiveTrial()));
			if (Project::getInstance()->getHasStudyData()){
				xmlWriter.writeAttribute("MetaData", QString("projectMetaData") + OS_SEP + QString("metadata.xml"));
			}
			
			//Cameras
			for (std::vector<Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it)
			{
				xmlWriter.writeStartElement("Camera");
				xmlWriter.writeAttribute("Name", (*it)->getName());
				xmlWriter.writeAttribute("isLightCamera", QString::number((*it)->isLightCamera()));
				xmlWriter.writeAttribute("isCalibrated", QString::number((*it)->isCalibrated()));
				xmlWriter.writeAttribute("isOptimized", QString::number((*it)->isOptimized()));
				if ((*it)->isCalibrated())
				{
					xmlWriter.writeAttribute("CameraMatrix", (*it)->getName() + OS_SEP + "data" + OS_SEP + (*it)->getFilenameCameraMatrix());
					if ((*it)->hasModelDistortion())
					{
						xmlWriter.writeAttribute("UndistortionParameter", (*it)->getName() + OS_SEP + "data" + OS_SEP + (*it)->getFilenameUndistortionParam());
					}
				}

				if ((*it)->hasUndistortion() && (*it)->getUndistortionObject())
				{
					xmlWriter.writeStartElement("UndistortionGrid");
					xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + (*it)->getUndistortionObject()->getFilename());
					xmlWriter.writeAttribute("isComputed", QString::number((*it)->getUndistortionObject()->isComputed()));
					if ((*it)->getUndistortionObject()->isComputed())
					{
						if ((*it)->getUndistortionObject()->isCenterSet())
						{
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

				for (std::vector<CalibrationImage*>::const_iterator it2 = (*it)->getCalibrationImages().begin(); it2 != (*it)->getCalibrationImages().end(); ++it2)
				{
					xmlWriter.writeStartElement("CalibrationImage");
					xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + (*it2)->getFilename());
					xmlWriter.writeAttribute("isCalibrated", QString::number((*it2)->isCalibrated()));
					if ((*it2)->isCalibrated())
					{
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
			xmlWriter.writeAttribute("isPLanar", QString::number(CalibrationObject::getInstance()->isCheckerboard()));
			if (CalibrationObject::getInstance()->isCheckerboard())
			{
				xmlWriter.writeAttribute("HorizontalSquares", QString::number(CalibrationObject::getInstance()->getNbHorizontalSquares()));
				xmlWriter.writeAttribute("VerticalSquares", QString::number(CalibrationObject::getInstance()->getNbVerticalSquares()));
				xmlWriter.writeAttribute("SquareSize", QString::number(CalibrationObject::getInstance()->getSquareSize()));
			}
			else
			{
				QFileInfo frameSpecificationsFilenameInfo(CalibrationObject::getInstance()->getFrameSpecificationsFilename());
				QFileInfo referencesFilenameInfo(CalibrationObject::getInstance()->getReferencesFilename());
				xmlWriter.writeAttribute("FrameSpecifications", QString("CalibrationObject") + OS_SEP + frameSpecificationsFilenameInfo.fileName());
				xmlWriter.writeAttribute("References", QString("CalibrationObject") + OS_SEP + referencesFilenameInfo.fileName());
			}
			xmlWriter.writeEndElement();
			//Trials

			for (unsigned int i = 0; i < Project::getInstance()->getTrials().size(); i++)
			{
				xmlWriter.writeStartElement("Trial");
				xmlWriter.writeAttribute("Name", Project::getInstance()->getTrials()[i]->getName());
				xmlWriter.writeAttribute("startFrame", QString::number(Project::getInstance()->getTrials()[i]->getStartFrame()));
				xmlWriter.writeAttribute("endFrame", QString::number(Project::getInstance()->getTrials()[i]->getEndFrame()));
				xmlWriter.writeAttribute("referenceCalibration", QString::number(Project::getInstance()->getTrials()[i]->getReferenceCalibrationImage()));
				xmlWriter.writeAttribute("recordingSpeed", QString::number(Project::getInstance()->getTrials()[i]->getRecordingSpeed()));
				xmlWriter.writeAttribute("cutOffFrequency", QString::number(Project::getInstance()->getTrials()[i]->getCutoffFrequency()));
				xmlWriter.writeAttribute("interpolateMissingFrames", QString::number(Project::getInstance()->getTrials()[i]->getInterpolateMissingFrames()));
				if (Project::getInstance()->getTrials()[i]->getHasStudyData()){
					xmlWriter.writeAttribute("MetaData", Project::getInstance()->getTrials()[i]->getName() + OS_SEP + QString("metadata.xml"));
				}
				for (unsigned int k = 0; k < Project::getInstance()->getTrials()[i]->getMarkers().size(); k++)
				{
					xmlWriter.writeStartElement("Marker");
					xmlWriter.writeAttribute("Description", Project::getInstance()->getTrials()[i]->getMarkers()[k]->getDescription());
					xmlWriter.writeAttribute("ID", QString::number(k));
					xmlWriter.writeAttribute("TrackingPenalty", QString::number(Project::getInstance()->getTrials()[i]->getMarkers()[k]->getMaxPenalty()));
					xmlWriter.writeAttribute("DetectionMethod", QString::number(Project::getInstance()->getTrials()[i]->getMarkers()[k]->getMethod()));
					xmlWriter.writeAttribute("SizeOverride", QString::number(Project::getInstance()->getTrials()[i]->getMarkers()[k]->getSizeOverride()));
					xmlWriter.writeAttribute("ThresholdOffset", QString::number(Project::getInstance()->getTrials()[i]->getMarkers()[k]->getThresholdOffset()));
					xmlWriter.writeAttribute("RequiresRecomputation", QString::number(Project::getInstance()->getTrials()[i]->getMarkers()[k]->getRequiresRecomputation()));

					if (Project::getInstance()->getTrials()[i]->getMarkers()[k]->Reference3DPointSet())
					{
						xmlWriter.writeAttribute("Reference3DPoint", Project::getInstance()->getTrials()[i]->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "reference3Dpoint.csv");
					}

					xmlWriter.writeAttribute("FilenamePoints2D", Project::getInstance()->getTrials()[i]->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "points2d.csv");
					xmlWriter.writeAttribute("FilenameStatus2D", Project::getInstance()->getTrials()[i]->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "status2d.csv");
					xmlWriter.writeAttribute("FilenameSize", Project::getInstance()->getTrials()[i]->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "size.csv");
					xmlWriter.writeAttribute("FilenamePoints3D", Project::getInstance()->getTrials()[i]->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "points3d.csv");
					xmlWriter.writeAttribute("FilenameStatus3D", Project::getInstance()->getTrials()[i]->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "status3d.csv");

					xmlWriter.writeEndElement();
				}

				for (unsigned int k = 0; k < Project::getInstance()->getTrials()[i]->getRigidBodies().size(); k++)
				{
					xmlWriter.writeStartElement("RigidBody");
					xmlWriter.writeAttribute("Description", Project::getInstance()->getTrials()[i]->getRigidBodies()[k]->getDescription());
					xmlWriter.writeAttribute("ID", QString::number(k));
					if (Project::getInstance()->getTrials()[i]->getRigidBodies()[k]->isReferencesSet() == 2)
					{
						xmlWriter.writeAttribute("ReferenceNames", Project::getInstance()->getTrials()[i]->getName() + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "ReferenceNames.csv");
						xmlWriter.writeAttribute("ReferencePoints3D", Project::getInstance()->getTrials()[i]->getName() + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "ReferencePoints3d.csv");
					}
					xmlWriter.writeAttribute("Visible", QString::number(Project::getInstance()->getTrials()[i]->getRigidBodies()[k]->getVisible()));
					xmlWriter.writeAttribute("Color", Project::getInstance()->getTrials()[i]->getRigidBodies()[k]->getColor().name());
					xmlWriter.writeAttribute("OverrideCutOffFrequency", QString::number(Project::getInstance()->getTrials()[i]->getRigidBodies()[k]->getOverrideCutoffFrequency()));
					xmlWriter.writeAttribute("CutOffFrequency", QString::number(Project::getInstance()->getTrials()[i]->getRigidBodies()[k]->getCutoffFrequency()));
					for (unsigned int p = 0; p < Project::getInstance()->getTrials()[i]->getRigidBodies()[k]->getDummyNames().size(); p++)
					{
						xmlWriter.writeStartElement("DummyMarker");
						xmlWriter.writeAttribute("Name", Project::getInstance()->getTrials()[i]->getRigidBodies()[k]->getDummyNames()[p]);
						xmlWriter.writeAttribute("PointReferences", Project::getInstance()->getTrials()[i]->getName() + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "DummyMarker" + QString().sprintf("%03d", p) + "PointReferences.csv");
						xmlWriter.writeAttribute("PointReferences2", Project::getInstance()->getTrials()[i]->getName() + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "DummyMarker" + QString().sprintf("%03d", p) + "PointReferences2.csv");
						xmlWriter.writeAttribute("PointCoordinates", Project::getInstance()->getTrials()[i]->getName() + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "DummyMarker" + QString().sprintf("%03d", p) + "PointCoordinates.csv");
						xmlWriter.writeEndElement();
					}

					xmlWriter.writeEndElement();
				}
				xmlWriter.writeEndElement();
			}

			xmlWriter.writeEndDocument();
			file.close();
		}
	}
	return true;
}

bool ProjectFileIO::readProjectFile(QString filename)
{
	int activeTrial = -1;

	if (filename.isNull() == false)
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

				while (!xml.atEnd() && !xml.hasError())
				{
					/* Read next element.*/
					QXmlStreamReader::TokenType token = xml.readNext();
					/* If token is just StartDocument, we'll go to next.*/
					if (token == QXmlStreamReader::StartDocument)
					{
						continue;
					}
					if (token == QXmlStreamReader::StartElement)
					{
						if (xml.name() == "Project")
						{
							QXmlStreamAttributes attr = xml.attributes();
							std::cout << "Load project file Version " << attr.value("Version").toString().toAscii().data() << std::endl;

							QString activeTrialString = attr.value("ActiveTrial").toString();
							if (!activeTrialString.isEmpty())activeTrial = activeTrialString.toInt();

							QString xml_file = attr.value("MetaData").toString();
							if (!xml_file.isEmpty())
							{
								loadProjectMetaData(littleHelper::adjustPathToOS(basedir + OS_SEP + xml_file));
							}
						}
						if (xml.name() == "Camera")
						{
							QXmlStreamAttributes attr = xml.attributes();
							QString text = attr.value("Name").toString();
							Camera* cam = new Camera(text, Project::getInstance()->getCameras().size());
							cam->setIsLightCamera(attr.value("isLightCamera").toString().toInt());
							cam->setCalibrated(attr.value("isCalibrated").toString().toInt());

							QString optimized = attr.value("isOptimized").toString();
							if (!optimized.isEmpty())cam->setOptimized(optimized.toInt());


							if (cam->isCalibrated())
							{
								QXmlStreamAttributes attr = xml.attributes();
								text = attr.value("CameraMatrix").toString();
								text.replace("\\",OS_SEP);
								text.replace("/",OS_SEP);
								cam->loadCameraMatrix(basedir + OS_SEP + text);

								QString undistparam = attr.value("UndistortionParameter").toString();
								if (!undistparam.isEmpty())
								{
									undistparam.replace("\\", OS_SEP);
									undistparam.replace("/", OS_SEP);
									cam->loadUndistortionParam(basedir + OS_SEP + undistparam);
								}
							}

							xml.readNext();
							while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "Camera"))
							{
								if (xml.tokenType() == QXmlStreamReader::StartElement)
								{
									if (xml.name() == "UndistortionGrid")
									{
										QXmlStreamAttributes attr = xml.attributes();
										text = attr.value("Filename").toString();
										text.replace("\\",OS_SEP);
										text.replace("/",OS_SEP);
										cam->loadUndistortionImage(basedir + OS_SEP + text);
										cam->getUndistortionObject()->setComputed(attr.value("isComputed").toString().toInt());
										while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "UndistortionGrid"))
										{
											if (xml.tokenType() == QXmlStreamReader::StartElement)
											{
												if (xml.name() == "Center")
												{
													QXmlStreamAttributes attr = xml.attributes();
													cam->getUndistortionObject()->setCenter(attr.value("x").toString().toDouble(), attr.value("y").toString().toDouble());
												}
												else if (xml.name() == "PointsDetected")
												{
													QXmlStreamAttributes attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													cam->getUndistortionObject()->loadPointsDetected(basedir + OS_SEP + text);
												}
												else if (xml.name() == "GridPointsDistorted")
												{
													QXmlStreamAttributes attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													cam->getUndistortionObject()->loadGridPointsDistorted(basedir + OS_SEP + text);
												}
												else if (xml.name() == "GridPointsReferences")
												{
													QXmlStreamAttributes attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													cam->getUndistortionObject()->loadGridPointsReferences(basedir + OS_SEP + text);
												}
												else if (xml.name() == "GridPointsInlier")
												{
													QXmlStreamAttributes attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													cam->getUndistortionObject()->loadGridPointsInlier(basedir + OS_SEP + text);
												}
											}
											xml.readNext();
										}
									}
									else if (xml.name() == "CalibrationImage")
									{
										QXmlStreamAttributes attr = xml.attributes();
										text = attr.value("Filename").toString();
										text.replace("\\",OS_SEP);
										text.replace("/",OS_SEP);
										CalibrationImage* image = cam->addImage(basedir + OS_SEP + text);
										image->setCalibrated(attr.value("isCalibrated").toString().toInt());
										while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "CalibrationImage"))
										{
											if (xml.tokenType() == QXmlStreamReader::StartElement)
											{
												if (xml.name() == "PointsDetectedAll")
												{
													QXmlStreamAttributes attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													image->loadPointsDetectedAll(basedir + OS_SEP + text);
												}
												else if (xml.name() == "PointsDetected")
												{
													QXmlStreamAttributes attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													image->loadPointsDetected(basedir + OS_SEP + text);
												}
												else if (xml.name() == "Inlier")
												{
													QXmlStreamAttributes attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													image->loadPointsInlier(basedir + OS_SEP + text);
												}
												else if (xml.name() == "RotationMatrix")
												{
													QXmlStreamAttributes attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													image->loadRotationMatrix(basedir + OS_SEP + text);
												}
												else if (xml.name() == "TranslationVector")
												{
													QXmlStreamAttributes attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													image->loadTranslationVector(basedir + OS_SEP + text);
												}
											}
											xml.readNext();
										}
									}
								}
								xml.readNext();
							}

							if (!cam->setResolutions())
							{
								ErrorDialog::getInstance()->showErrorDialog(cam->getName() + " : Resolutions do not match");
								return false;
							}

							Project::getInstance()->addCamera(cam);
						}
						if (xml.name() == "CalibrationObject")
						{
							QXmlStreamAttributes attr = xml.attributes();
							bool isPlanar = attr.value("isPLanar").toString().toInt();

							if (isPlanar)
							{
								CalibrationObject::getInstance()->setCheckerboard(
									attr.value("HorizontalSquares").toString().toInt(),
									attr.value("VerticalSquares").toString().toInt(),
									attr.value("SquareSize").toString().toInt()
								);
							}
							else
							{
								QString frameSpec = basedir + OS_SEP + attr.value("FrameSpecifications").toString();
								QString references = basedir + OS_SEP + attr.value("References").toString();
								references.replace("\\",OS_SEP);
								references.replace("/",OS_SEP);
								frameSpec.replace("\\",OS_SEP);
								frameSpec.replace("/",OS_SEP);
								CalibrationObject::getInstance()->loadCoords(frameSpec, references);
							}
						}
						if (xml.name() == "Trial")
						{
							QXmlStreamAttributes attr = xml.attributes();
							QString trialname = attr.value("Name").toString();
							QString trialfolder = basedir + OS_SEP + trialname + OS_SEP;
							Trial* trial = new Trial(trialname, littleHelper::adjustPathToOS(trialfolder));

							int startFrame = attr.value("startFrame").toString().toInt();
							trial->setStartFrame(startFrame);
							int endFrame = attr.value("endFrame").toString().toInt();
							trial->setEndFrame(endFrame);
							int referenceCalibration = attr.value("referenceCalibration").toString().toInt();
							trial->setReferenceCalibrationImage(referenceCalibration);

							QString recordingSpeed = attr.value("recordingSpeed").toString();
							if (!recordingSpeed.isEmpty())trial->setRecordingSpeed(recordingSpeed.toDouble());

							QString cutOffFrequency = attr.value("cutOffFrequency").toString();
							if (!cutOffFrequency.isEmpty())trial->setCutoffFrequency(cutOffFrequency.toDouble());

							QString interpolateMissingFrames = attr.value("interpolateMissingFrames").toString();
							if (!interpolateMissingFrames.isEmpty())trial->setInterpolateMissingFrames(interpolateMissingFrames.toInt());

							trial->loadMarkers(trialfolder + OS_SEP + "data" + OS_SEP + "MarkerDescription.txt");
							trial->loadRigidBodies(trialfolder + OS_SEP + "data" + OS_SEP + "RigidBodies.txt");

							QString xml_file = attr.value("MetaData").toString();
							if (!xml_file.isEmpty())
							{
								loadProjectMetaData(littleHelper::adjustPathToOS(trialfolder + OS_SEP + xml_file));
								trial->setXMLData(littleHelper::adjustPathToOS(trialfolder + OS_SEP + xml_file));
								trial->parseXMLData();
							}

							while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "Trial"))
							{
								if (xml.tokenType() == QXmlStreamReader::StartElement)
								{
									if (xml.name() == "Marker")
									{
										QXmlStreamAttributes attr = xml.attributes();
										QString filename_points2D = basedir + OS_SEP + attr.value("FilenamePoints2D").toString();
										QString filename_status2D = basedir + OS_SEP + attr.value("FilenameStatus2D").toString();
										QString filename_size = basedir + OS_SEP + attr.value("FilenameSize").toString();

										int id = attr.value("ID").toString().toInt();
										trial->getMarkers()[id]->load(littleHelper::adjustPathToOS(filename_points2D), littleHelper::adjustPathToOS(filename_status2D), littleHelper::adjustPathToOS(filename_size));

										QString Reference3DPoint = attr.value("Reference3DPoint").toString();
										if (!Reference3DPoint.isEmpty())trial->getMarkers()[id]->loadReference3DPoint(littleHelper::adjustPathToOS(basedir + OS_SEP + Reference3DPoint));

										QString TrackingPenalty = attr.value("TrackingPenalty").toString();
										if (!TrackingPenalty.isEmpty())trial->getMarkers()[id]->setMaxPenalty(TrackingPenalty.toInt());

										QString DetectionMethod = attr.value("DetectionMethod").toString();
										if (!DetectionMethod.isEmpty())trial->getMarkers()[id]->setMethod(DetectionMethod.toInt());

										QString SizeOverride = attr.value("SizeOverride").toString();
										if (!SizeOverride.isEmpty())trial->getMarkers()[id]->setSizeOverride(SizeOverride.toInt());

										QString ThresholdOffset = attr.value("ThresholdOffset").toString();
										if (!ThresholdOffset.isEmpty())trial->getMarkers()[id]->setThresholdOffset(ThresholdOffset.toInt());

										QString requiresRecomputation = attr.value("RequiresRecomputation").toString();
										QString filename_points3D = attr.value("FilenamePoints3D").toString();
										QString filename_status3D = attr.value("FilenameStatus3D").toString();

										if (!requiresRecomputation.isEmpty() && !filename_points3D.isEmpty() && !filename_status3D.isEmpty())
										{
											filename_points3D = basedir + OS_SEP + filename_points3D;
											filename_status3D = basedir + OS_SEP + filename_status3D;
											trial->getMarkers()[id]->setRequiresRecomputation(requiresRecomputation.toInt());
											trial->getMarkers()[id]->load3DPoints(littleHelper::adjustPathToOS(filename_points3D), littleHelper::adjustPathToOS(filename_status3D));
										}
									}

									if (xml.name() == "RigidBody")
									{
										QXmlStreamAttributes attr = xml.attributes();

										QString filename_referenceNames_attr = attr.value("ReferenceNames").toString();
										QString filename_referencePoints3D_attr = attr.value("ReferencePoints3D").toString();

										int id = attr.value("ID").toString().toInt();

										if (!filename_referenceNames_attr.isEmpty() && !filename_referencePoints3D_attr.isEmpty())
										{
											QString filename_referenceNames = basedir + OS_SEP + filename_referenceNames_attr;
											QString filename_referencePoints3D = basedir + OS_SEP + filename_referencePoints3D_attr;
											trial->getRigidBodies()[id]->load(littleHelper::adjustPathToOS(filename_referenceNames), littleHelper::adjustPathToOS(filename_referencePoints3D));
										}
										else
										{
											trial->getRigidBodies()[id]->resetReferences();
										}

										QString visible = attr.value("Visible").toString();
										if (!visible.isEmpty())
										{
											trial->getRigidBodies()[id]->setVisible(visible.toInt());
										}
										QString color = attr.value("Color").toString();
										if (!color.isEmpty())
										{
											trial->getRigidBodies()[id]->setColor(QColor(color));
										}

										QString overrideCutOffFrequency = attr.value("OverrideCutOffFrequency").toString();
										if (!overrideCutOffFrequency.isEmpty())
										{
											trial->getRigidBodies()[id]->setOverrideCutoffFrequency(overrideCutOffFrequency.toInt());
										}

										QString cutOffFrequency = attr.value("CutOffFrequency").toString();
										if (!cutOffFrequency.isEmpty())
										{
											trial->getRigidBodies()[id]->setCutoffFrequency(cutOffFrequency.toDouble());
										}


										while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "RigidBody"))
										{
											if (xml.tokenType() == QXmlStreamReader::StartElement)
											{
												if (xml.name() == "DummyMarker")
												{
													QXmlStreamAttributes attr = xml.attributes();

													QString dummyName = attr.value("Name").toString();
													QString dummyPointReferences = basedir + OS_SEP + attr.value("PointReferences").toString();
													QString dummyPointReferences2 = attr.value("PointReferences2").toString();
													if (dummyPointReferences2.isEmpty())
													{
														dummyPointReferences2 = dummyPointReferences;
													}
													else
													{
														dummyPointReferences2 = basedir + OS_SEP + dummyPointReferences2;
													}
													QString dummyPointCoordinates = basedir + OS_SEP + attr.value("PointCoordinates").toString();

													trial->getRigidBodies()[id]->addDummyPoint(dummyName, littleHelper::adjustPathToOS(dummyPointReferences), littleHelper::adjustPathToOS(dummyPointReferences2), 1, littleHelper::adjustPathToOS(dummyPointCoordinates));
												}
											}
											xml.readNext();
										}
									}
								}
								xml.readNext();
							}


							Project::getInstance()->addTrial(trial);
							WorkspaceNavigationFrame::getInstance()->addTrial(trialname);
						}
					}
				}
				if (xml.hasError())
				{
					ErrorDialog::getInstance()->showErrorDialog(QString("QXSRExample::parseXML %1").arg(xml.errorString()));
				}
				file.close();

				if (activeTrial >= 0 && activeTrial < (int) Project::getInstance()->getTrials().size())
				{
					State::getInstance()->changeActiveTrial(activeTrial, true);
				}
				else if (Project::getInstance()->getTrials().size() > 0)
				{
					State::getInstance()->changeActiveTrial(Project::getInstance()->getTrials().size() - 1, true);
				}
			}
		}
	}

	return true;
}

void ProjectFileIO::recurseAddDir(QDir d, QStringList& list)
{
	QStringList qsl = d.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);

	foreach (QString file, qsl)
		{
			QFileInfo finfo(QString("%1/%2").arg(d.path()).arg(file));

			if (finfo.isSymLink())
				return;

			if (finfo.isDir())
			{
				QString dirname = finfo.fileName();
				QDir sd(finfo.filePath());

				recurseAddDir(sd, list);
			}
			else
				list << QDir::toNativeSeparators(finfo.filePath());
		}
}

bool ProjectFileIO::zipFromFolderToFile(const QString& filePath, const QDir& dir, const QString& comment)
{
	//Make sure the zip file does not exist
	if (QFile::exists(filePath))
	{
		QFile::remove(filePath);
	}

	QuaZip zip(filePath);
	zip.setFileNameCodec("IBM866");

	if (!zip.open(QuaZip::mdCreate))
	{
		ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile(): zip.open(): %1").arg(zip.getZipError()));
		return false;
	}

	if (!dir.exists())
	{
		ErrorDialog::getInstance()->showErrorDialog(QString("dir.exists(%1)=FALSE").arg(dir.absolutePath()));
		return false;
	}

	QFile inFile;

	QStringList sl;
	recurseAddDir(dir, sl);

	QFileInfoList files;
	foreach (QString fn, sl) files << QFileInfo(fn);

	QuaZipFile outFile(&zip);

	char c;
	foreach(QFileInfo fileInfo, files)
		{
			if (!fileInfo.isFile())
				continue;

			QString fileNameWithRelativePath = fileInfo.filePath().remove(0, dir.absolutePath().length() + 1);

			inFile.setFileName(fileInfo.filePath());

			if (!inFile.open(QIODevice::ReadOnly))
			{
				ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile(): inFile.open(): %1").arg(inFile.errorString().toLocal8Bit().constData()));
				return false;
			}

			if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileNameWithRelativePath, fileInfo.filePath())))
			{
				ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile(): outFile.open(): %1").arg(outFile.getZipError()));
				return false;
			}

			while (inFile.getChar(&c) && outFile.putChar(c));

			if (outFile.getZipError() != UNZ_OK)
			{
				ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile(): outFile.putChar(): %1").arg(outFile.getZipError()));
				return false;
			}

			outFile.close();

			if (outFile.getZipError() != UNZ_OK)
			{
				ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile(): outFile.close(): %1").arg(outFile.getZipError()));
				return false;
			}

			inFile.close();
		}


	if (!comment.isEmpty())
		zip.setComment(comment);

	zip.close();

	if (zip.getZipError() != 0)
	{
		ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile(): zip.close(): %1").arg(zip.getZipError()));
		return false;
	}

	return true;
}

bool ProjectFileIO::unzipFromFileToFolder(const QString& filePath, const QString& extDirPath, const QString& singleFileName)
{
	//Make sure the folder is empty
	removeDir(extDirPath);

	QuaZip zip(filePath);

	if (!zip.open(QuaZip::mdUnzip))
	{
		ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile():  zip.open(): %1").arg(zip.getZipError()));
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
	for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
	{
		if (!zip.getCurrentFileInfo(&info))
		{
			ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile():  getCurrentFileInfo(): %1\n").arg(zip.getZipError()));
			return false;
		}

		if (!singleFileName.isEmpty())
			if (!info.name.contains(singleFileName))
				continue;

		if (!file.open(QIODevice::ReadOnly))
		{
			ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile():  file.open(): %1").arg(file.getZipError()));
			return false;
		}

		name = QString("%1/%2").arg(extDirPath, file.getActualFileName());

		if (file.getZipError() != UNZ_OK)
		{
			ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile():  file.getFileName(): %1").arg(file.getZipError()));
			return false;
		}

		//out.setFileName("out/" + name);
		out.setFileName(name);
		QFileInfo info(name);
		QDir().mkpath(info.absolutePath());

		// this will fail if "name" contains subdirectories, but we don't mind that
		out.open(QIODevice::WriteOnly);
		// Slow like hell (on GNU/Linux at least), but it is not my fault.
		// Not ZIP/UNZIP package's fault either.
		// The slowest thing here is out.putChar(c).
		while (file.getChar(&c)) out.putChar(c);

		out.close();

		if (file.getZipError() != UNZ_OK)
		{
			ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile():  file.getFileName(): %1").arg(file.getZipError()));
			return false;
		}

		if (!file.atEnd())
		{
			ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile():  read all but not EOF"));
			return false;
		}

		file.close();

		if (file.getZipError() != UNZ_OK)
		{
			ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile():  file.close(): %1").arg(file.getZipError()));
			return false;
		}
	}

	zip.close();

	if (zip.getZipError() != UNZ_OK)
	{
		ErrorDialog::getInstance()->showErrorDialog(QString("zipFromFolderToFile():  zip.close(): %1").arg(zip.getZipError()));
		return false;
	}

	return true;
}

