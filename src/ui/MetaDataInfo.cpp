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
///\file MetaDataInfo.cpp
///\author Benjamin Knorlein
///\date 1/12/2016

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/MetaDataInfo.h"
#include "ui_MetaDataInfo.h"

#include "core/Project.h"


using namespace xma;

MetaDataInfo::MetaDataInfo(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::MetaDataInfo)
{
	diag->setupUi(this);

	if (Project::getInstance()->getHasStudyData())
	{

		QTreeWidgetItem * item = new QTreeWidgetItem(QStringList() << "Calibration" << "");
		diag->treeWidget->addTopLevelItem(item);
		item->addChild(new QTreeWidgetItem(QStringList() << "Repository:"  << Project::getInstance()->getRepository()));
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
	diag->treeWidget->expandAll();
	diag->treeWidget->resizeColumnToContents(0);
	diag->treeWidget->collapseAll();
}

MetaDataInfo::~MetaDataInfo()
{
	delete diag;
}
