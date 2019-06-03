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
///\file NewTrialDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/NewTrialDialog.h"
#include "ui_NewTrialDialog.h"
#include "ui/CameraBoxTrial.h"
#include "ui/ErrorDialog.h"
#include "ui/MainWindow.h"
#include "ui/ProjectFileIO.h"
#include "ui/WorkspaceNavigationFrame.h"
#include "ui/ProgressDialog.h"

#include "core/Camera.h"
#include "core/Project.h"
#include "core/Trial.h"
#include "core/Settings.h"

#include <QFileDialog>
#include <QtCore>
#include "ConfirmationDialog.h"

using namespace xma;

NewTrialDialog::NewTrialDialog(Trial * trial, QWidget* parent) :
QDialog(MainWindow::getInstance()), m_trial(trial),
	diag(new Ui::NewTrialDialog)
{
	diag->setupUi(this);
	diag->frameNBCameras->hide();
	trialname = "Trial " + QString::number(Project::getInstance()->getTrials().size() + 1);
	diag->lineEditTrialName->setText(trialname);
	xml_metadata = "";
	nbCams = 0;
	for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
	{
		CameraBoxTrial* box = new CameraBoxTrial();
		box->setCameraName(Project::getInstance()->getCameras()[i]->getName());
		diag->gridLayout_6->addWidget(box, i, 0, 1, 1);
		cameras.push_back(box);
		nbCams++;
	}
	diag->labelNbCameras->setText(QString::number(nbCams));

	if (m_trial)
	{
		this->setWindowTitle("Change trial data");
		trialname = m_trial->getName();
		diag->lineEditTrialName->setText(trialname);
	}

	if (Project::getInstance()->hasDefaultTrial() || (Project::getInstance()->getCalibration() == NO_CALIBRATION)){
		diag->pushButton_Default->setEnabled(false);
	}
	noCalibration = false;
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

NewTrialDialog::~NewTrialDialog()
{
	delete diag;

	for (std::vector<CameraBoxTrial*>::iterator it = cameras.begin(); it != cameras.end(); ++it)
		delete (*it);

	cameras.clear();
}

void NewTrialDialog::setNBCamerasVisible()
{
	diag->pushButton_Default->setEnabled(false);
	diag->frameNBCameras->show();
	noCalibration = true;
	if (nbCams < 1)
	{
		nbCams++;
		CameraBoxTrial* box = new CameraBoxTrial();
		box->setCameraName("Camera " + QString::number(nbCams));
		diag->gridLayout_6->addWidget(box, nbCams - 1, 0, 1, 1);
		cameras.push_back(box);	

		diag->labelNbCameras->setText(QString::number(nbCams));
	}
}

void NewTrialDialog::setCam(int i, QString filename)
{
	cameras[i]->setFilename(filename);
}

void NewTrialDialog::setTrialName(QString trialName)
{
	diag->lineEditTrialName->setText(trialName);
}

void NewTrialDialog::setXmlMetadata(const QString& _xml_metadata)
{
	this->xml_metadata = _xml_metadata;
}

bool NewTrialDialog::createTrial()
{
	if (m_trial)
	{
		std::vector<QStringList> list;
		for (std::vector<CameraBoxTrial*>::const_iterator it = getCameras().begin(); it != getCameras().end(); ++it)
		{
			list.push_back((*it)->getImageFileNames());
		}
		if(!m_trial->changeTrialData(trialname, list))
		{
			ErrorDialog::getInstance()->showErrorDialog("Width and Height of the videos does is not equal to the calibration images. Cannot change data for Trial.");
			return false;
		}

		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->setXMLData(xml_metadata);
		if (!Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->setFrameRateFromXML())
		{
			ErrorDialog::getInstance()->showErrorDialog("Framerate could not be set. Either the framerates of the files differ or Framerates have not been defined");
		}

		return true;
	}
	else
	{
		if (noCalibration)
		{
			for (std::vector<CameraBoxTrial*>::const_iterator it = getCameras().begin(); it != getCameras().end(); ++it)
			{
				Camera* cam = new Camera((*it)->getCameraName(), Project::getInstance()->getCameras().size());
				Project::getInstance()->addCamera(cam);
			}
		}

		std::vector<QStringList> list;
		for (std::vector<CameraBoxTrial*>::const_iterator it = getCameras().begin(); it != getCameras().end(); ++it)
		{
			list.push_back((*it)->getImageFileNames());
		}
		Trial * t = new Trial(trialname, list);
		
		if (noCalibration)
		{
			t->setCameraSizes();
		}
		else if (!t->checkTrialImageSizeValid())
		{
			delete t;
			ErrorDialog::getInstance()->showErrorDialog("Width and Height of the videos does is not equal to the calibration images. Cannot create Trial.");
			return false;
		}

		Project::getInstance()->addTrial(t);
		WorkspaceNavigationFrame::getInstance()->addTrial(trialname);
		State::getInstance()->changeActiveTrial(Project::getInstance()->getTrials().size() - 1, true);
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->bindTextures();
		State::getInstance()->changeActiveFrameTrial(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame(), true);
		State::getInstance()->changeWorkspace(DIGITIZATION, true);
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->setXMLData(xml_metadata);
		if (!Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->setFrameRateFromXML())
		{
			ErrorDialog::getInstance()->showErrorDialog("Framerate could not be set. Either the framerates of the files differ or Framerates have not been defined");
		}

		return true;
	}
}

int NewTrialDialog::getNBCameras()
{
	return cameras.size();
}

bool NewTrialDialog::isComplete()
{
	int nbImages = cameras[0]->getImageFileNames().size();
	//Check if Cameras are all complete
	bool nameUnique = true;
	for (std::vector<Trial *>::const_iterator trial = Project::getInstance()->getTrials().begin(); trial != Project::getInstance()->getTrials().end(); ++trial)
	{
		if (trialname == (*trial)->getName() && (*trial) != m_trial)nameUnique = false;
	}

	if (!nameUnique)
	{
		ErrorDialog::getInstance()->showErrorDialog("Please choose another Trialname. A trial with the name " + trialname + " already exists.");
		return false;
	}

	for (std::vector<CameraBoxTrial*>::iterator it = cameras.begin(); it != cameras.end(); ++it)
	{
		if (!(*it)->isComplete())
		{
			return false;
		}
		else if ((*it)->getImageFileNames().size() != nbImages)
		{
			ErrorDialog::getInstance()->showErrorDialog((*it)->getCameraName() + " has wrong number of images : " + QString::number((*it)->getImageFileNames().size()) +
				" instead of " + QString::number(nbImages));
			return false;
		}
	}

	return true;
}


/***************************************************************
UI - SLOTS
***************************************************************/

//Footer buttons
void NewTrialDialog::on_pushButton_OK_clicked()
{
	if (isComplete()) this->accept();
}

void NewTrialDialog::on_pushButton_Cancel_clicked()
{
	this->reject();
}

void NewTrialDialog::on_pushButton_LoadXMA_clicked()
{
	if (!ConfirmationDialog::getInstance()->showConfirmationDialog("The data will be extracted to the folder where your xmatrial-file is located in case you did not setup a workspace. Also please make sure that you added enough cameras to the trial before importing. Do you want to proceed?", true))
		return;

	xmaTrial_filename = QFileDialog::getOpenFileName(this,
		tr("Select dataset"), Settings::getInstance()->getLastUsedDirectory(), tr("Dataset (*.xmatrial  *.zip)"));
	if (xmaTrial_filename.isNull() == false)
	{
		deleteAfterLoad = ConfirmationDialog::getInstance()->showConfirmationDialog("Do you want to delete the .xmatrial file after the import? Click Yes if you want to delete it. No if you do not want to delete it.", true);

		m_FutureWatcher = new QFutureWatcher<void>();
		connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(LoadXMAFinished()));

		QFuture<void> future = QtConcurrent::run(ProjectFileIO::getInstance(), &ProjectFileIO::loadXMAPortalTrial, xmaTrial_filename, this);
		m_FutureWatcher->setFuture(future);

		ProgressDialog::getInstance()->showProgressbar(0, 0, ("Load trial " + xmaTrial_filename).toAscii().data());
	}
}

void NewTrialDialog::on_pushButton_Default_clicked()
{
	Trial * t = new Trial();
	Project::getInstance()->addTrial(t);
	WorkspaceNavigationFrame::getInstance()->addTrial(t->getName());
	State::getInstance()->changeActiveTrial(Project::getInstance()->getTrials().size() - 1, true);
	State::getInstance()->changeActiveFrameTrial(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame(), true);
	State::getInstance()->changeWorkspace(DIGITIZATION, true);
	
	this->close();
}

void NewTrialDialog::LoadXMAFinished()
{
	delete m_FutureWatcher;
	if (deleteAfterLoad) QFile(xmaTrial_filename).remove();

	ProgressDialog::getInstance()->closeProgressbar();
}

void NewTrialDialog::on_lineEditTrialName_textChanged(QString text)
{
	trialname = text;
}

//Cameras
void NewTrialDialog::on_toolButtonCameraMinus_clicked()
{
	if (nbCams > 1)
	{
		diag->gridLayout_6->removeWidget(cameras[nbCams - 1]);
		delete cameras[nbCams - 1];
		cameras.pop_back();

		nbCams -= 1;
		diag->labelNbCameras->setText(QString::number(nbCams));
	}
}

void NewTrialDialog::on_toolButtonCameraPlus_clicked()
{
	nbCams += 1;
	CameraBoxTrial* box = new CameraBoxTrial();
	box->setCameraName("Camera " + QString::number(nbCams));
	diag->gridLayout_6->addWidget(box, nbCams -1, 0, 1, 1);
	cameras.push_back(box);

	diag->labelNbCameras->setText(QString::number(nbCams));		
}