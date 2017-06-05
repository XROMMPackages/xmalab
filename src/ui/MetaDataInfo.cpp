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
///\file MetaDataInfo.cpp
///\author Benjamin Knorlein
///\date 1/12/2016

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/MetaDataInfo.h"
#include "ui_MetaDataInfo.h"

#include "core/Project.h"
#include "core/Trial.h"
#include "core/VideoStream.h"


using namespace xma;

MetaDataInfo::MetaDataInfo(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::MetaDataInfo)
{
	diag->setupUi(this);

	if (Project::getInstance()->hasCalibration()){
		QTreeWidgetItem * item = new QTreeWidgetItem(QStringList() << "Calibration" << "");
		diag->treeWidget->addTopLevelItem(item);
		item->addChild(new QTreeWidgetItem(QStringList() << "Repository:" << Project::getInstance()->getRepository()));
		item->addChild(new QTreeWidgetItem(QStringList() << "Studyname:" << Project::getInstance()->getStudyName()));
		item->addChild(new QTreeWidgetItem(QStringList() << "StudyID" << QString::number(Project::getInstance()->getStudyId())));
		item->addChild(new QTreeWidgetItem(QStringList() << "TrialName" << Project::getInstance()->getTrialName()));
		item->addChild(new QTreeWidgetItem(QStringList() << "TrialID" << QString::number(Project::getInstance()->getTrialId())));
		item->addChild(new QTreeWidgetItem(QStringList() << "Trialnumber" << QString::number(Project::getInstance()->getTrialNumber())));
		item->addChild(new QTreeWidgetItem(QStringList() << "Trialtype" << Project::getInstance()->getTrialType()));
		item->addChild(new QTreeWidgetItem(QStringList() << "Lab" << Project::getInstance()->getLab()));
		item->addChild(new QTreeWidgetItem(QStringList() << "Comment" << Project::getInstance()->getAttribcomment()));
		item->addChild(new QTreeWidgetItem(QStringList() << "Timestamp" << Project::getInstance()->getTs()));
		item->addChild(new QTreeWidgetItem(QStringList() << "TrialDate" << Project::getInstance()->getTrialDate()));
	}

	for (unsigned int i = 0; i < Project::getInstance()->getInstance()->getTrials().size(); i++)
		{
		if (Project::getInstance()->getTrials()[i]->getIsDefault())
			continue;

		QTreeWidgetItem * itemTrial = new QTreeWidgetItem(QStringList() << "Trial" << Project::getInstance()->getTrials()[i]->getName());
		diag->treeWidget->addTopLevelItem(itemTrial);
		itemTrial->addChild(new QTreeWidgetItem(QStringList() << "Repository:" << Project::getInstance()->getTrials()[i]->getRepository()));
		itemTrial->addChild(new QTreeWidgetItem(QStringList() << "Studyname:" << Project::getInstance()->getTrials()[i]->getStudyName()));
		itemTrial->addChild(new QTreeWidgetItem(QStringList() << "StudyID" << QString::number(Project::getInstance()->getTrials()[i]->getStudyId())));
		itemTrial->addChild(new QTreeWidgetItem(QStringList() << "TrialName" << Project::getInstance()->getTrials()[i]->getTrialName()));
		itemTrial->addChild(new QTreeWidgetItem(QStringList() << "TrialID" << QString::number(Project::getInstance()->getTrials()[i]->getTrialId())));
		itemTrial->addChild(new QTreeWidgetItem(QStringList() << "Trialnumber" << QString::number(Project::getInstance()->getTrials()[i]->getTrialNumber())));
		itemTrial->addChild(new QTreeWidgetItem(QStringList() << "Trialtype" << Project::getInstance()->getTrials()[i]->getTrialType()));
		itemTrial->addChild(new QTreeWidgetItem(QStringList() << "Lab" << Project::getInstance()->getTrials()[i]->getLab()));
		itemTrial->addChild(new QTreeWidgetItem(QStringList() << "Comment" << Project::getInstance()->getTrials()[i]->getAttribcomment()));
		itemTrial->addChild(new QTreeWidgetItem(QStringList() << "Timestamp" << Project::getInstance()->getTrials()[i]->getTs()));
		itemTrial->addChild(new QTreeWidgetItem(QStringList() << "TrialDate" << Project::getInstance()->getTrials()[i]->getTrialDate()));
		for (unsigned int j = 0; j < Project::getInstance()->getInstance()->getTrials()[i]->getVideoStreams().size(); j++)
		{
			int camera_id = Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getPortalID();
			if (camera_id == -1) camera_id = j + 1;
			QTreeWidgetItem * itemVideo = new QTreeWidgetItem(QStringList() << "Camera" + QString::number(camera_id) << "");
			itemTrial->addChild(itemVideo);
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "Filename:" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getFilename()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "FileID:" << QString::number(Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getFileId())));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "FileCategory" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getFileCategory()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "cameraNumber" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getCameraNumber()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "Instrument" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getInstrument()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "FrameRate" << QString::number(Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getFrameRate())));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "ShutterSpeed" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getShutterSpeed()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "kV" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getKV()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "EDR" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getEDR()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "mA" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getMA()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "SID" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getSid()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "MagLevel" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getMagLevel()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "Radiationtype" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getRadiationtype()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "Pulsewidth" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getPulsewidth()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "FileDescription" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getFileDescription()));
			itemVideo->addChild(new QTreeWidgetItem(QStringList() << "Lab" << Project::getInstance()->getTrials()[i]->getVideoStreams()[j]->getLab()));
		}
	}
	
	diag->treeWidget->expandAll();
	diag->treeWidget->resizeColumnToContents(0);
	diag->treeWidget->collapseAll();
}

MetaDataInfo::~MetaDataInfo()
{
	delete diag;
}
