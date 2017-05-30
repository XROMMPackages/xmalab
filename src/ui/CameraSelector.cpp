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
///\file CameraSelector.cpp
///\author Benjamin Knorlein
///\date 05/30/2017

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif


#include "ui/CameraSelector.h"
#include "ui_CameraSelector.h"
#include "ui/MainWindow.h"
#include "core/Project.h"
#include "core/Camera.h"


#include <QCheckBox>

using namespace xma;

CameraSelector::CameraSelector(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::CameraSelector)
{
	diag->setupUi(this);
	int cam_count = 0;
	for (auto cam : Project::getInstance()->getCameras()){
		QCheckBox * box = new QCheckBox(cam->getName(), this);
		box->setChecked(cam->isVisible());
		boxes.push_back(box);
		diag->gridLayout_2->addWidget(box, cam_count, 0, 1, 1);
		cam_count++;
	}
}

CameraSelector::~CameraSelector()
{
	boxes.clear();
	delete diag;
}

void CameraSelector::on_pushButton_OK_clicked()
{
	for (int i = 0; i < boxes.size(); i++)
	{
		Project::getInstance()->getCameras()[i]->setVisible(boxes[i]->isChecked());
		MainWindow::getInstance()->setCameraVisible(i, boxes[i]->isChecked());
	}
	this->accept();
}

void CameraSelector::on_pushButton_Cancel_clicked()
{
	this->reject();
}

