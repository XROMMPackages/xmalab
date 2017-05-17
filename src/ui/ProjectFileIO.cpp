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
#include "core/CalibrationSequence.h"
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

int ProjectFileIO::saveProject(QString filename, std::vector <Trial*> trials, bool subset)
{
	bool success = true;
	if (!subset)
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
		success = writeProjectFile(tmpDir_path + "project.xml", trials);
		writePortalFile(tmpDir_path, trials);
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
		for (std::vector<Trial*>::const_iterator trial_it = trials.begin(); trial_it != trials.end(); ++trial_it)
		{
			QString path = tmpDir_path + OS_SEP + (*trial_it)->getName() + OS_SEP;
			if (!QDir().mkpath(path))
			{
				ErrorDialog::getInstance()->showErrorDialog("Can not create tmp folder " + path);
				success = false;
			}
			else
			{

				if ((*trial_it)->getHasStudyData()){
					(*trial_it)->saveXMLData(path + OS_SEP + "metadata.xml");
				}

				(*trial_it)->save(path);
				QDir().mkpath(path + OS_SEP + "data");
				(*trial_it)->saveMarkers(path + OS_SEP + "data" + OS_SEP + "MarkerDescription.txt");
				(*trial_it)->saveRigidBodies(path + OS_SEP + "data" + OS_SEP + "RigidBodies.txt");
				for (unsigned int k = 0; k < (*trial_it)->getMarkers().size(); k++)
				{
					(*trial_it)->getMarkers()[k]->save(path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "points2d.csv",
					                                                              path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "status2d.csv",
					                                                              path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "size.csv");
					(*trial_it)->getMarkers()[k]->save3DPoints(path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "points3d.csv",
					                                                                      path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "status3d.csv");
					(*trial_it)->getMarkers()[k]->saveInterpolation(path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "interpolation.csv");



					if ((*trial_it)->getMarkers()[k]->Reference3DPointSet())
					{
						(*trial_it)->getMarkers()[k]->saveReference3DPoint(path + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "reference3Dpoint.csv");
					}
				}
				for (unsigned int k = 0; k < (*trial_it)->getRigidBodies().size(); k++)
				{
					if ((*trial_it)->getRigidBodies()[k]->isReferencesSet() == 2)
					{
						(*trial_it)->getRigidBodies()[k]->save(
							path + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "ReferenceNames.csv",
							path + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "ReferencePoints3d.csv");
					}
					if ((*trial_it)->getRigidBodies()[k]->getHasOptimizedCoordinates())
					{
						(*trial_it)->getRigidBodies()[k]->saveOptimized(
							path + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "ReferencePoints3d_optimized.csv");
					}
					for (unsigned int p = 0; p < (*trial_it)->getRigidBodies()[k]->getDummyNames().size(); p++)
					{
						(*trial_it)->getRigidBodies()[k]->saveDummy(p,
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
	double version;
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
							if (xml.name() == "Project")
							{
								QXmlStreamAttributes attr = xml.attributes();
								std::cout << "Load project file Version " << attr.value("Version").toString().toAscii().data() << std::endl;
								version = attr.value("Version").toString().toDouble();
							}
							if (xml.name() == "Trial")
							{
								QXmlStreamAttributes attr = xml.attributes();
								if (attr.value("Name").toString() == trialname)
								{
									QString _trialname = attr.value("Name").toString();
									QString trialfolder = basedir + OS_SEP + _trialname + OS_SEP;
									if (trialname == "Default")
									{
										trial = new Trial();
									}
									else{
										trial = new Trial(_trialname, littleHelper::adjustPathToOS(trialfolder));
									}
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

									QString interpolate3D = attr.value("interpolate3D").toString();
									if (!interpolate3D.isEmpty())trial->setInterpolate3D(interpolate3D.toInt());

									trial->loadMarkers(trialfolder + OS_SEP + "data" + OS_SEP + "MarkerDescription.txt");

									trial->loadRigidBodies(trialfolder + OS_SEP + "data" + OS_SEP + "RigidBodies.txt");

									QString xml_file = attr.value("MetaData").toString();
									if (!xml_file.isEmpty())
									{
										loadProjectMetaData(littleHelper::adjustPathToOS(trialfolder + OS_SEP + xml_file));
										trial->setXMLData(littleHelper::adjustPathToOS(trialfolder + OS_SEP + xml_file));
										trial->parseXMLData();
									}

									QString isDefault = attr.value("Default").toString();
									if (!isDefault.isEmpty())
									{
										trial->setIsDefault(isDefault.toInt());
									}
									QString fromDefault = attr.value("FromDefault").toString();
									if (!fromDefault.isEmpty())
									{
										trial->setIsCopyFromDefault(fromDefault.toInt());
									}

									while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "Trial"))
									{
										if (xml.tokenType() == QXmlStreamReader::StartElement)
										{
											if (xml.name() == "Marker")
											{
												attr = xml.attributes();
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

												QString InterpolationMethod = attr.value("InterpolationMethod").toString();
												if (!InterpolationMethod.isEmpty()){
													int newMethod = 0;
													switch (InterpolationMethod.toInt())
													{
														case 0:
														default:
															newMethod = 0;
															break;
														case 1:
														case 4:
															newMethod = 10;
															break;
														case 2:
														case 5:
															newMethod = 20;
															break;
														case 3:
														case 6:
															newMethod = 20;
															break;
													}
													for (int i = 0; i < trial->getNbImages(); i++)
														trial->getMarkers()[id]->setInterpolation(i,interpolationMethod(newMethod));

													trial->getMarkers()[id]->updateHasInterpolation();
												}
												QString SizeOverride = attr.value("SizeOverride").toString();
												if (!SizeOverride.isEmpty())trial->getMarkers()[id]->setSizeOverride(SizeOverride.toInt());

												QString ThresholdOffset = attr.value("ThresholdOffset").toString();
												if (!ThresholdOffset.isEmpty())trial->getMarkers()[id]->setThresholdOffset(ThresholdOffset.toInt());

												QString filename_interpolation = attr.value("FilenameInterpolation").toString();
												if (!filename_interpolation.isEmpty())
												{
													trial->getMarkers()[id]->loadInterpolation(basedir + OS_SEP + filename_interpolation);
												}
											}

											if (xml.name() == "RigidBody")
											{
												attr = xml.attributes();

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

												QString filename_referencePoints3DOptimized_attr = attr.value("ReferencePoints3DOptimized").toString();
												if (!filename_referencePoints3DOptimized_attr.isEmpty())
												{
													QString filename_referencePoints3DOptimized = basedir + OS_SEP + filename_referencePoints3DOptimized_attr;
													trial->getRigidBodies()[id]->loadOptimized(littleHelper::adjustPathToOS(filename_referencePoints3DOptimized));
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

												QString cutOffFrequencyBody = attr.value("CutOffFrequency").toString();
												if (!cutOffFrequencyBody.isEmpty())
												{
													trial->getRigidBodies()[id]->setCutoffFrequency(cutOffFrequencyBody.toDouble());
												}

												QString meshmodel = attr.value("Meshmodel").toString();
												if (!meshmodel.isEmpty())
												{
													bool meshSet = trial->getRigidBodies()[id]->addMeshModel(meshmodel);
													if (meshSet)
													{
														QString drawMeshmodel = attr.value("DrawMeshmodel").toString();
														if (!drawMeshmodel.isEmpty())
														{
															trial->getRigidBodies()[id]->setDrawMeshModel(drawMeshmodel.toInt());
														}

														QString meshScale = attr.value("MeshScale").toString();
														if (!meshScale.isEmpty())
														{
															trial->getRigidBodies()[id]->setMeshScale(meshScale.toDouble());
														}
													}
												}

												while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "RigidBody"))
												{
													if (xml.tokenType() == QXmlStreamReader::StartElement)
													{
														if (xml.name() == "DummyMarker")
														{
															attr = xml.attributes();

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

															trial->getRigidBodies()[id]->addDummyPoint(dummyName, littleHelper::adjustPathToOS(dummyPointReferences), littleHelper::adjustPathToOS(dummyPointReferences2), -1, littleHelper::adjustPathToOS(dummyPointCoordinates));
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
	
	if (version < 0.2)
	{
		upgradeTo12(trial);
	}
	if (version < 0.3)
	{
		upgradeTo13(trial);
	}

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
									QString _trialname = attr.value("Name").toString();
									QString trialfolder = basedir + OS_SEP + _trialname + OS_SEP;

									trial->loadMarkers(trialfolder + OS_SEP + "data" + OS_SEP + "MarkerDescription.txt");
									trial->loadRigidBodies(trialfolder + OS_SEP + "data" + OS_SEP + "RigidBodies.txt");

									while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "Trial"))
									{
										if (xml.tokenType() == QXmlStreamReader::StartElement)
										{
											if (xml.name() == "Marker")
											{
												attr = xml.attributes();
												int id = attr.value("ID").toString().toInt();

												QString Reference3DPoint = basedir + OS_SEP + attr.value("Reference3DPoint").toString();
												if (!Reference3DPoint.isEmpty())trial->getMarkers()[id]->loadReference3DPoint(littleHelper::adjustPathToOS(Reference3DPoint));

												QString TrackingPenalty = attr.value("TrackingPenalty").toString();
												if (!TrackingPenalty.isEmpty())trial->getMarkers()[id]->setMaxPenalty(TrackingPenalty.toInt());

												QString DetectionMethod = attr.value("DetectionMethod").toString();
												if (!DetectionMethod.isEmpty())trial->getMarkers()[id]->setMethod(DetectionMethod.toInt());

												QString InterpolationMethod = attr.value("InterpolationMethod").toString();
												if (!InterpolationMethod.isEmpty()){
													int newMethod = 0;
													switch (InterpolationMethod.toInt())
													{
													case 0:
													default:
														newMethod = 0;
														break;
													case 1:
													case 4:
														newMethod = 10;
														break;
													case 2:
													case 5:
														newMethod = 20;
														break;
													case 3:
													case 6:
														newMethod = 20;
														break;
													}
													for (int i = 0; i < trial->getNbImages(); i++)
														trial->getMarkers()[id]->setInterpolation(i, interpolationMethod(newMethod));

													trial->getMarkers()[id]->updateHasInterpolation();
												}

												QString SizeOverride = attr.value("SizeOverride").toString();
												if (!SizeOverride.isEmpty())trial->getMarkers()[id]->setSizeOverride(SizeOverride.toInt());

												QString ThresholdOffset = attr.value("ThresholdOffset").toString();
												if (!ThresholdOffset.isEmpty())trial->getMarkers()[id]->setThresholdOffset(ThresholdOffset.toInt());
											}

											if (xml.name() == "RigidBody")
											{
												attr = xml.attributes();

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

												QString filename_referencePoints3DOptimized_attr = attr.value("ReferencePoints3DOptimized").toString();
												if (!filename_referencePoints3DOptimized_attr.isEmpty())
												{
													QString filename_referencePoints3DOptimized = basedir + OS_SEP + filename_referencePoints3DOptimized_attr;
													trial->getRigidBodies()[id]->loadOptimized(littleHelper::adjustPathToOS(filename_referencePoints3DOptimized));
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

												QString meshmodel = attr.value("Meshmodel").toString();
												if (!meshmodel.isEmpty())
												{
													bool meshSet = trial->getRigidBodies()[id]->addMeshModel(meshmodel);
													if (meshSet)
													{
														QString drawMeshmodel = attr.value("DrawMeshmodel").toString();
														if (!drawMeshmodel.isEmpty())
														{
															trial->getRigidBodies()[id]->setDrawMeshModel(drawMeshmodel.toInt());
														}

														QString meshScale = attr.value("MeshScale").toString();
														if (!meshScale.isEmpty())
														{
															trial->getRigidBodies()[id]->setMeshScale(meshScale.toDouble());
														}
													}
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

void ProjectFileIO::upgradeTo12(Trial *trial)
{
	for (std::vector<Marker*>::const_iterator it = trial->getMarkers().begin(); it < trial->getMarkers().end(); ++it){
		(*it)->updateToProject12();
	}
}

void ProjectFileIO::upgradeTo13(Trial* trial)
{
	for (std::vector<Marker*>::const_iterator it = trial->getMarkers().begin(); it < trial->getMarkers().end(); ++it){
		(*it)->updateToProject13();
	}
}

void ProjectFileIO::writePortalFile(QString path, std::vector <Trial*> trials)
{
	QString xml_filename = path + OS_SEP + "xma_metadata.xml";
	if (!xml_filename.isNull())
	{
		QFile file(xml_filename);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QXmlStreamWriter xmlWriter(&file);
			xmlWriter.writeStartDocument();
			xmlWriter.setAutoFormatting(true);
			xmlWriter.writeStartElement("data");

			xmlWriter.writeStartElement("xmalab-version");
			xmlWriter.writeCharacters(UI_VERSION);
			xmlWriter.writeEndElement();

			xmlWriter.writeStartElement("repository");
			xmlWriter.writeCharacters(Project::getInstance()->getRepository());
			xmlWriter.writeEndElement();

			xmlWriter.writeStartElement("studyid");
			xmlWriter.writeCharacters(QString::number(Project::getInstance()->getStudyId()));
			xmlWriter.writeEndElement();

			xmlWriter.writeStartElement("studyname");
			xmlWriter.writeCharacters(Project::getInstance()->getStudyName());
			xmlWriter.writeEndElement();

			//TOFIX
			xmlWriter.writeStartElement("date-created");
			QDateTime local = QDateTime::currentDateTime();
			QDateTime utc = local.toUTC();
			utc.setTimeSpec(Qt::LocalTime);
			int utcOffset = utc.secsTo(local);
			local.setUtcOffset(utcOffset);
			xmlWriter.writeCharacters(local.toString(Qt::ISODate));
			xmlWriter.writeEndElement();

			xmlWriter.writeStartElement("date-modified");
			//QDateTime local = QDateTime::currentDateTime();
			//QDateTime utc = local.toUTC();
			//utc.setTimeSpec(Qt::LocalTime);
			//int utcOffset = utc.secsTo(local);
			local.setUtcOffset(utcOffset);
			xmlWriter.writeCharacters(local.toString(Qt::ISODate));
			xmlWriter.writeEndElement();

			xmlWriter.writeStartElement("CalibrationTrial");
			xmlWriter.writeAttribute("id", QString::number(Project::getInstance()->getTrialId()));
			xmlWriter.writeAttribute("name", Project::getInstance()->getTrialName());
			xmlWriter.writeStartElement("Undistortion");
			for (auto c : Project::getInstance()->getCameras())
			{
				xmlWriter.writeStartElement("Camera ");
				xmlWriter.writeAttribute("camera-id", QString::number(c->getID()));
				xmlWriter.writeCharacters(c->getUndistortionObject()->getFilename());
				xmlWriter.writeEndElement();
			}
			xmlWriter.writeEndElement();

			xmlWriter.writeStartElement("Calibration");
			for (auto c : Project::getInstance()->getCameras())
			{
				xmlWriter.writeStartElement("Camera");
				xmlWriter.writeAttribute("camera-id", QString::number(c->getID()));
				xmlWriter.writeAttribute("points", QString::number(c->getCalibrationNbInlier()));
				xmlWriter.writeAttribute("error", QString::number(c->getCalibrationError()));
				for (auto f : c->getCalibrationImages())
				{
					xmlWriter.writeStartElement("File");
					xmlWriter.writeAttribute("points", QString::number(f->getCalibrationNbInlier()));
					xmlWriter.writeAttribute("error", QString::number(f->getCalibrationError()));
					xmlWriter.writeCharacters(f->getFilename());
					xmlWriter.writeEndElement();
				}
				xmlWriter.writeEndElement(); // Camera
			}
			

			xmlWriter.writeStartElement("Object");
			if (CalibrationObject::getInstance()->isCheckerboard())
			{
				xmlWriter.writeAttribute("type", "checkerboard");
				xmlWriter.writeAttribute("width", QString::number(CalibrationObject::getInstance()->getNbHorizontalSquares()));
				xmlWriter.writeAttribute("height", QString::number(CalibrationObject::getInstance()->getNbVerticalSquares()));
				xmlWriter.writeAttribute("size", QString::number(CalibrationObject::getInstance()->getSquareSize()));
			} 
			else
			{
				xmlWriter.writeAttribute("type", "cube");
				QFileInfo info(CalibrationObject::getInstance()->getFrameSpecificationsFilename());
				xmlWriter.writeAttribute("csv", info.fileName());
				info = QFileInfo(CalibrationObject::getInstance()->getReferencesFilename());
				xmlWriter.writeAttribute("ref", info.fileName());
			}
			xmlWriter.writeEndElement(); //Object
			xmlWriter.writeEndElement(); //Calibration
			xmlWriter.writeEndElement(); //CalibrationTrial
			for (auto t : trials)
			{
				if (!t->getIsDefault()){
					xmlWriter.writeStartElement("Trial");
					xmlWriter.writeAttribute("id", QString::number(t->getTrialId()));
					xmlWriter.writeAttribute("name", t->getTrialName());
					xmlWriter.writeAttribute("xmalab-name", t->getName());
					xmlWriter.writeAttribute("firstTracked", QString::number(t->getFirstTrackedFrame()));
					xmlWriter.writeAttribute("lastTracked", QString::number(t->getLastTrackedFrame()));
					xmlWriter.writeAttribute("filterfrequency", QString::number(t->getCutoffFrequency()));
					xmlWriter.writeAttribute("framerate", QString::number(t->getRecordingSpeed()));
					xmlWriter.writeAttribute("reprojectionerror", QString::number(t->getReprojectionError()));
					xmlWriter.writeAttribute("averageSD", QString::number(t->getMarkerToMarkerSD()));

					xmlWriter.writeStartElement("Movies");
					for (int m = 0; m < t->getVideoStreams().size(); m++)
					{
						xmlWriter.writeStartElement("File");
						xmlWriter.writeAttribute("camera-id", QString::number(m));
						xmlWriter.writeAttribute("FileId", QString::number(t->getVideoStreams()[m]->getFileId()));
						xmlWriter.writeAttribute("FileName", t->getVideoStreams()[m]->getFilename());

						QFileInfo info(t->getVideoStreams()[m]->getFilenames()[0]);
						xmlWriter.writeCharacters(info.fileName());
						xmlWriter.writeEndElement(); // File
					}
					xmlWriter.writeEndElement(); // Movies

					std::vector <bool> pointsWritten(t->getMarkers().size(), false);

					for (auto rb : t->getRigidBodies())
					{
						xmlWriter.writeStartElement("RigidBody");
						xmlWriter.writeAttribute("name", rb->getDescription());
						xmlWriter.writeAttribute("firstTracked", QString::number(rb->getFirstTrackedFrame()));
						xmlWriter.writeAttribute("lastTracked", QString::number(rb->getLastTrackedFrame()));
						xmlWriter.writeAttribute("numberTracked", QString::number(rb->getFramesTracked()));
						double averageSD;
						int count;
						rb->getMarkerToMarkerSD(averageSD, count);
						xmlWriter.writeAttribute("averageSD", QString::number(averageSD));
						xmlWriter.writeAttribute("error3DUnfiltered", QString::number(rb->getError3D(false)));
						xmlWriter.writeAttribute("error3DFiltered", QString::number(rb->getError3D(true)));
						if (rb->getOverrideCutoffFrequency()){
							xmlWriter.writeAttribute("filterfrequency", QString::number(rb->getCutoffFrequency()));
						}
						else
						{
							xmlWriter.writeAttribute("filterfrequency", QString::number(t->getCutoffFrequency()));
						}
						for (auto idx : rb->getPointsIdx())
						{
							pointsWritten[idx] = true;
							xmlWriter.writeStartElement("Marker");
							xmlWriter.writeAttribute("name", t->getMarkers()[idx]->getDescription());
							xmlWriter.writeAttribute("id", QString::number(idx + 1));
							xmlWriter.writeAttribute("firstTracked", QString::number(t->getMarkers()[idx]->getFirstTrackedFrame()));
							xmlWriter.writeAttribute("lastTracked", QString::number(t->getMarkers()[idx]->getLastTrackedFrame()));
							xmlWriter.writeAttribute("numberTracked", QString::number(t->getMarkers()[idx]->getFramesTracked()));
							xmlWriter.writeAttribute("reprojectionerror", QString::number(t->getMarkers()[idx]->getReprojectionError()));
							xmlWriter.writeEndElement(); // Marker
						}

						xmlWriter.writeEndElement(); // RigidBody
					}

					for (int idx = 0; idx < pointsWritten.size(); idx++)
					{
						if (!pointsWritten[idx])
						{
							pointsWritten[idx] = true;
							xmlWriter.writeStartElement("Marker");
							xmlWriter.writeAttribute("name", t->getMarkers()[idx]->getDescription());
							xmlWriter.writeAttribute("id", QString::number(idx + 1));
							xmlWriter.writeAttribute("firstTracked", QString::number(t->getMarkers()[idx]->getFirstTrackedFrame()));
							xmlWriter.writeAttribute("lastTracked", QString::number(t->getMarkers()[idx]->getLastTrackedFrame()));
							xmlWriter.writeAttribute("numberTracked", QString::number(t->getMarkers()[idx]->getFramesTracked()));
							xmlWriter.writeAttribute("reprojectionerror", QString::number(t->getMarkers()[idx]->getReprojectionError()));
							xmlWriter.writeEndElement(); // Marker
						}
					}

					xmlWriter.writeEndElement(); //Trial
				}
			}


			xmlWriter.writeEndElement(); //data
			xmlWriter.writeEndDocument();
			file.close();
		}		
	}

}

void ProjectFileIO::removeTmpDir()
{
	QString tmpDir_path = QDir::tempPath() + OS_SEP + "XROMM_tmp" + OS_SEP;
	removeDir(tmpDir_path);
}

bool ProjectFileIO::writeProjectFile(QString filename, std::vector<Trial*> trials)
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
			xmlWriter.writeAttribute("Version", "0.3");
			xmlWriter.writeAttribute("ActiveTrial", QString::number(State::getInstance()->getActiveTrial()));
			if (Project::getInstance()->getHasStudyData()){
				xmlWriter.writeAttribute("MetaData", QString("projectMetaData") + OS_SEP + QString("metadata.xml"));
			}
			xmlWriter.writeAttribute("FlipImages", QString::number(Project::getInstance()->getFlipImages()));
			
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

				if ((*it)->getCalibrationSequence()->hasCalibrationSequence())
				{
					xmlWriter.writeStartElement("CalibrationSequence");
					xmlWriter.writeAttribute("Filename", (*it)->getCalibrationSequence()->getFilename());
					xmlWriter.writeAttribute("Frames", QString::number((*it)->getCalibrationSequence()->getNbImages()));
					int width, height;
					(*it)->getCalibrationSequence()->getResolution(width, height);
					xmlWriter.writeAttribute("Width", QString::number(width));
					xmlWriter.writeAttribute("Height", QString::number(height));
					xmlWriter.writeEndElement();
				}
				int count = 0;
				for (std::vector<CalibrationImage*>::const_iterator it2 = (*it)->getCalibrationImages().begin(); it2 != (*it)->getCalibrationImages().end(); ++it2)
				{
					if ((*it)->getCalibrationSequence()->hasCalibrationSequence())
					{
						if ((*it2)->isCalibrated())
						{
							xmlWriter.writeStartElement("CalibrationImage");
							xmlWriter.writeAttribute("Frame", QString::number(count));
							xmlWriter.writeAttribute("isCalibrated", QString::number((*it2)->isCalibrated()));
						}
					} 
					else
					{
						xmlWriter.writeStartElement("CalibrationImage");
						xmlWriter.writeAttribute("Filename", (*it)->getName() + OS_SEP + (*it2)->getFilename());
						xmlWriter.writeAttribute("isCalibrated", QString::number((*it2)->isCalibrated()));
					}

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
					if (!(*it)->getCalibrationSequence()->hasCalibrationSequence() || (*it2)->isCalibrated())
						xmlWriter.writeEndElement();

					count++;
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

			for (std::vector<Trial*>::const_iterator trial_it = trials.begin(); trial_it != trials.end(); ++trial_it)
			{
				xmlWriter.writeStartElement("Trial");
				xmlWriter.writeAttribute("Name", (*trial_it)->getName());
				xmlWriter.writeAttribute("Default", QString::number((*trial_it)->getIsDefault()));
				xmlWriter.writeAttribute("FromDefault", QString::number((*trial_it)->getIsCopyFromDefault()));
				xmlWriter.writeAttribute("startFrame", QString::number((*trial_it)->getStartFrame()));
				xmlWriter.writeAttribute("endFrame", QString::number((*trial_it)->getEndFrame()));
				xmlWriter.writeAttribute("referenceCalibration", QString::number((*trial_it)->getReferenceCalibrationImage()));
				xmlWriter.writeAttribute("recordingSpeed", QString::number((*trial_it)->getRecordingSpeed()));
				xmlWriter.writeAttribute("cutOffFrequency", QString::number((*trial_it)->getCutoffFrequency()));
				xmlWriter.writeAttribute("interpolate3D", QString::number((*trial_it)->getInterpolate3D()));

				
				if ((*trial_it)->getHasStudyData()){
					xmlWriter.writeAttribute("MetaData", (*trial_it)->getName() + OS_SEP + QString("metadata.xml"));
				}
				for (unsigned int k = 0; k < (*trial_it)->getMarkers().size(); k++)
				{
					xmlWriter.writeStartElement("Marker");
					xmlWriter.writeAttribute("Description", (*trial_it)->getMarkers()[k]->getDescription());
					xmlWriter.writeAttribute("ID", QString::number(k));
					xmlWriter.writeAttribute("TrackingPenalty", QString::number((*trial_it)->getMarkers()[k]->getMaxPenalty()));
					xmlWriter.writeAttribute("DetectionMethod", QString::number((*trial_it)->getMarkers()[k]->getMethod()));
					xmlWriter.writeAttribute("SizeOverride", QString::number((*trial_it)->getMarkers()[k]->getSizeOverride()));
					xmlWriter.writeAttribute("ThresholdOffset", QString::number((*trial_it)->getMarkers()[k]->getThresholdOffset()));
					xmlWriter.writeAttribute("RequiresRecomputation", QString::number((*trial_it)->getMarkers()[k]->getRequiresRecomputation()));

					if ((*trial_it)->getMarkers()[k]->Reference3DPointSet())
					{
						xmlWriter.writeAttribute("Reference3DPoint", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "reference3Dpoint.csv");
					}

					xmlWriter.writeAttribute("FilenamePoints2D", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "points2d.csv");
					xmlWriter.writeAttribute("FilenameStatus2D", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "status2d.csv");
					xmlWriter.writeAttribute("FilenameSize", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "size.csv");
					xmlWriter.writeAttribute("FilenamePoints3D", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "points3d.csv");
					xmlWriter.writeAttribute("FilenameStatus3D", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "status3d.csv");
					xmlWriter.writeAttribute("FilenameInterpolation", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "Marker" + QString().sprintf("%03d", k) + "interpolation.csv");


					xmlWriter.writeEndElement();
				}

				for (unsigned int k = 0; k < (*trial_it)->getRigidBodies().size(); k++)
				{
					xmlWriter.writeStartElement("RigidBody");
					xmlWriter.writeAttribute("Description", (*trial_it)->getRigidBodies()[k]->getDescription());
					xmlWriter.writeAttribute("ID", QString::number(k));
					if ((*trial_it)->getRigidBodies()[k]->isReferencesSet() == 2)
					{
						xmlWriter.writeAttribute("ReferenceNames", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "ReferenceNames.csv");
						xmlWriter.writeAttribute("ReferencePoints3D", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "ReferencePoints3d.csv");
					}
					if ((*trial_it)->getRigidBodies()[k]->getHasOptimizedCoordinates())
					{
						xmlWriter.writeAttribute("ReferencePoints3DOptimized", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "ReferencePoints3d_optimized.csv");
					}
					xmlWriter.writeAttribute("Visible", QString::number((*trial_it)->getRigidBodies()[k]->getVisible()));
					xmlWriter.writeAttribute("Color", (*trial_it)->getRigidBodies()[k]->getColor().name());
					xmlWriter.writeAttribute("OverrideCutOffFrequency", QString::number((*trial_it)->getRigidBodies()[k]->getOverrideCutoffFrequency()));
					xmlWriter.writeAttribute("CutOffFrequency", QString::number((*trial_it)->getRigidBodies()[k]->getCutoffFrequency()));
					if ((*trial_it)->getRigidBodies()[k]->hasMeshModel()){
						xmlWriter.writeAttribute("Meshmodel", (*trial_it)->getRigidBodies()[k]->getMeshModelname());
						xmlWriter.writeAttribute("DrawMeshmodel", QString::number((*trial_it)->getRigidBodies()[k]->getDrawMeshModel()));
						xmlWriter.writeAttribute("MeshScale", QString::number((*trial_it)->getRigidBodies()[k]->getMeshScale()));
					}
		
					for (unsigned int p = 0; p < (*trial_it)->getRigidBodies()[k]->getDummyNames().size(); p++)
					{
						xmlWriter.writeStartElement("DummyMarker");
						xmlWriter.writeAttribute("Name", (*trial_it)->getRigidBodies()[k]->getDummyNames()[p]);
						xmlWriter.writeAttribute("PointReferences", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "DummyMarker" + QString().sprintf("%03d", p) + "PointReferences.csv");
						xmlWriter.writeAttribute("PointReferences2", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "DummyMarker" + QString().sprintf("%03d", p) + "PointReferences2.csv");
						xmlWriter.writeAttribute("PointCoordinates", (*trial_it)->getName() + OS_SEP + "data" + OS_SEP + "RigidBody" + QString().sprintf("%03d", k) + "DummyMarker" + QString().sprintf("%03d", p) + "PointCoordinates.csv");
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
	double version;
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
							version = attr.value("Version").toString().toDouble();
							QString activeTrialString = attr.value("ActiveTrial").toString();
							if (!activeTrialString.isEmpty())activeTrial = activeTrialString.toInt();

							QString xml_file = attr.value("MetaData").toString();
							if (!xml_file.isEmpty())
							{
								loadProjectMetaData(littleHelper::adjustPathToOS(basedir + OS_SEP + xml_file));
							}

							QString flipImages = attr.value("FlipImages").toString();
							if (!flipImages.isEmpty())
							{
								Project::getInstance()->setFlipImages(flipImages.toInt());
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
								attr = xml.attributes();
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
										attr = xml.attributes();
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
													attr = xml.attributes();
													cam->getUndistortionObject()->setCenter(attr.value("x").toString().toDouble(), attr.value("y").toString().toDouble());
												}
												else if (xml.name() == "PointsDetected")
												{
													attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													cam->getUndistortionObject()->loadPointsDetected(basedir + OS_SEP + text);
												}
												else if (xml.name() == "GridPointsDistorted")
												{
													attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													cam->getUndistortionObject()->loadGridPointsDistorted(basedir + OS_SEP + text);
												}
												else if (xml.name() == "GridPointsReferences")
												{
													attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													cam->getUndistortionObject()->loadGridPointsReferences(basedir + OS_SEP + text);
												}
												else if (xml.name() == "GridPointsInlier")
												{
													attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													cam->getUndistortionObject()->loadGridPointsInlier(basedir + OS_SEP + text);
												}
											}
											xml.readNext();
										}
									}
									else if (xml.name() == "CalibrationSequence")
									{
										attr = xml.attributes();
										QString filename_seq = attr.value("Filename").toString();
										filename_seq.replace("\\", OS_SEP);
										filename_seq.replace("/", OS_SEP);

										int frames_seq = attr.value("Frames").toString().toInt();
										int width_seq = attr.value("Width").toString().toInt();
										int height_seq = attr.value("Height").toString().toInt();
										cam->setCalibrationSequence(filename_seq,frames_seq, width_seq, height_seq);
									}
									else if (xml.name() == "CalibrationImage")
									{
										CalibrationImage* image;
										attr = xml.attributes();
										text = attr.value("Filename").toString();
										if (!text.isEmpty()){
											text.replace("\\", OS_SEP);
											text.replace("/", OS_SEP);
											//TODO
											image = cam->addImage(basedir + OS_SEP + text);
										} 
										else
										{
											int frame = attr.value("Frame").toString().toInt();
											image = cam->getCalibrationImages()[frame];
										}
										image->setCalibrated(attr.value("isCalibrated").toString().toInt());
										while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "CalibrationImage"))
										{
											if (xml.tokenType() == QXmlStreamReader::StartElement)
											{
												if (xml.name() == "PointsDetectedAll")
												{
													attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													image->loadPointsDetectedAll(basedir + OS_SEP + text);
												}
												else if (xml.name() == "PointsDetected")
												{
													attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													image->loadPointsDetected(basedir + OS_SEP + text);
												}
												else if (xml.name() == "Inlier")
												{
													attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													image->loadPointsInlier(basedir + OS_SEP + text);
												}
												else if (xml.name() == "RotationMatrix")
												{
													attr = xml.attributes();
													text = attr.value("Filename").toString();
													text.replace("\\",OS_SEP);
													text.replace("/",OS_SEP);
													image->loadRotationMatrix(basedir + OS_SEP + text);
												}
												else if (xml.name() == "TranslationVector")
												{
													attr = xml.attributes();
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
									attr.value("SquareSize").toString().toDouble()
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
							Trial* trial;
							if (trialname == "Default")
							{
								trial = new Trial();
							}
							else{
								trial = new Trial(trialname, littleHelper::adjustPathToOS(trialfolder));
							}
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

							QString interpolate3D = attr.value("interpolate3D").toString();
							if (!interpolate3D.isEmpty())trial->setInterpolate3D(interpolate3D.toInt());

							trial->loadMarkers(trialfolder + OS_SEP + "data" + OS_SEP + "MarkerDescription.txt");
							trial->loadRigidBodies(trialfolder + OS_SEP + "data" + OS_SEP + "RigidBodies.txt");

							QString xml_file = attr.value("MetaData").toString();
							if (!xml_file.isEmpty())
							{
								loadProjectMetaData(littleHelper::adjustPathToOS(trialfolder + OS_SEP + xml_file));
								trial->setXMLData(littleHelper::adjustPathToOS(trialfolder + OS_SEP + xml_file));
								trial->parseXMLData();
							}

							QString isDefault = attr.value("Default").toString();
							if (!isDefault.isEmpty())
							{
								trial->setIsDefault(isDefault.toInt());
							}
							QString fromDefault = attr.value("FromDefault").toString();
							if (!fromDefault.isEmpty())
							{
								trial->setIsCopyFromDefault(fromDefault.toInt());
							}

							while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "Trial"))
							{
								if (xml.tokenType() == QXmlStreamReader::StartElement)
								{
									if (xml.name() == "Marker")
									{
										attr = xml.attributes();
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

										QString InterpolationMethod = attr.value("InterpolationMethod").toString();
										if (!InterpolationMethod.isEmpty()){
											int newMethod = 0;
											switch (InterpolationMethod.toInt())
											{
												case 0:
												default:
													newMethod = 0;
													break;
												case 1:
												case 4:
													newMethod = 10;
													break;
												case 2:
												case 5:
													newMethod = 20;
													break;
												case 3:
												case 6:
													newMethod = 20;
													break;
											}
											for (int i = 0; i < trial->getNbImages(); i++)
												trial->getMarkers()[id]->setInterpolation(i,interpolationMethod(newMethod));

											trial->getMarkers()[id]->updateHasInterpolation();
										}
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

										QString filename_interpolation = attr.value("FilenameInterpolation").toString();
										if (!filename_interpolation.isEmpty())
										{
											trial->getMarkers()[id]->loadInterpolation(basedir + OS_SEP + filename_interpolation);
										}
									}

									if (xml.name() == "RigidBody")
									{
										attr = xml.attributes();

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

										QString filename_referencePoints3DOptimized_attr = attr.value("ReferencePoints3DOptimized").toString();
										if (!filename_referencePoints3DOptimized_attr.isEmpty())
										{
											QString filename_referencePoints3DOptimized = basedir + OS_SEP + filename_referencePoints3DOptimized_attr;
											trial->getRigidBodies()[id]->loadOptimized(littleHelper::adjustPathToOS(filename_referencePoints3DOptimized));
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

										QString cutOffFrequencyBody = attr.value("CutOffFrequency").toString();
										if (!cutOffFrequencyBody.isEmpty())
										{
											trial->getRigidBodies()[id]->setCutoffFrequency(cutOffFrequencyBody.toDouble());
										}

										QString meshmodel = attr.value("Meshmodel").toString();
										if (!meshmodel.isEmpty())
										{
											bool meshSet = trial->getRigidBodies()[id]->addMeshModel(meshmodel);
											if (meshSet)
											{
												QString drawMeshmodel = attr.value("DrawMeshmodel").toString();
												if (!drawMeshmodel.isEmpty())
												{
													trial->getRigidBodies()[id]->setDrawMeshModel(drawMeshmodel.toInt());
												}

												QString meshScale = attr.value("MeshScale").toString();
												if (!meshScale.isEmpty())
												{
													trial->getRigidBodies()[id]->setMeshScale(meshScale.toDouble());
												}
											}
										}

										while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "RigidBody"))
										{
											if (xml.tokenType() == QXmlStreamReader::StartElement)
											{
												if (xml.name() == "DummyMarker")
												{
													attr = xml.attributes();

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

	if (version < 0.2)
	{
		for (std::vector<Trial*>::const_iterator it = Project::getInstance()->getTrials().begin(); it < Project::getInstance()->getTrials().end(); ++it)
		upgradeTo12(*it);
	}
	if (version < 0.3)
	{
		for (std::vector<Trial*>::const_iterator it = Project::getInstance()->getTrials().begin(); it < Project::getInstance()->getTrials().end(); ++it)
			upgradeTo13(*it);
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

			while (inFile.getChar(&c)){
				if (!outFile.putChar(c))
				{
					ErrorDialog::getInstance()->showErrorDialog("Could not write file. Please check your diskspace and restart XMALab!");
					inFile.close();
					outFile.close();
					zip.close();
					return false;
				};
			}

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
		QFileInfo fileinfo(name);
		QDir().mkpath(fileinfo.absolutePath());
		// this will fail if "name" contains subdirectories, but we don't mind that
		out.open(QIODevice::WriteOnly);
		// Slow like hell (on GNU/Linux at least), but it is not my fault.
		// Not ZIP/UNZIP package's fault either.
		// The slowest thing here is out.putChar(c).
		while (file.getChar(&c)){
			if (!out.putChar(c))
			{
				ErrorDialog::getInstance()->showErrorDialog("Could not write file. Please check your diskspace and restart XMALab!");
				out.close();
				file.close();
				zip.close();
				return false;
			};
		}
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

