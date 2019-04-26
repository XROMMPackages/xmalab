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
///\file DisplayOptionsDockWidget.cpp
///\author Benjamin Knorlein
///\date 05/01/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif


#include "ui/MainWindow.h"
#include "ui/Shortcuts.h"
#include "ui/DisplayOptionsDockWidget.h"
#include "ui_DisplayOptionsDockWidget.h"

#include "core/Settings.h"
#include "core/Project.h"
#include "core/Trial.h"
#include "core/VideoStream.h"
#include "core/Image.h"

#include <QCloseEvent>
#include <iostream>
#include "VisualFilterDialog.h"


using namespace xma;

DisplayOptionsDockWidget* DisplayOptionsDockWidget::instance = NULL;

DisplayOptionsDockWidget::DisplayOptionsDockWidget(QWidget* parent) :
	QDockWidget(parent),
	dock(new Ui::DisplayOptionsDockWidget)
{
	dock->setupUi(this);
	dock->checkBoxHide->setChecked(Settings::getInstance()->getBoolSetting("TrialDrawHideAll"));
	dock->checkBoxMarkers->setChecked(Settings::getInstance()->getBoolSetting("TrialDrawMarkers"));
	dock->checkBoxMarkerIds->setChecked(Settings::getInstance()->getBoolSetting("TrialDrawMarkerIds"));
	dock->checkBoxEpipolar->setChecked(Settings::getInstance()->getBoolSetting("TrialDrawEpipolar"));
	dock->checkBoxRigidBodyConstellation->setChecked(Settings::getInstance()->getBoolSetting("TrialDrawRigidBodyConstellation"));
	dock->checkBoxRigidBodyMeshmodels->setChecked(Settings::getInstance()->getBoolSetting("TrialDrawRigidBodyMeshmodels"));
	dock->checkBox_DrawFiltered->setChecked(Settings::getInstance()->getBoolSetting("TrialDrawFiltered"));
	dock->checkBox_VisualFilter->setChecked(Settings::getInstance()->getBoolSetting("VisualFilterEnabled"));
	toggleEnabled(!dock->checkBoxHide->isChecked());

#ifdef __APPLE__
	dock->checkBoxHide->setMinimumHeight(26);
	dock->checkBoxMarkers->setMinimumHeight(26);
	dock->checkBoxMarkerIds->setMinimumHeight(26);
	dock->checkBoxEpipolar->setMinimumHeight(26);
	dock->checkBoxRigidBodyConstellation->setMinimumHeight(26);
	dock->checkBoxRigidBodyMeshmodels->setMinimumHeight(26);
	dock->checkBox_DrawFiltered->setMinimumHeight(26);
	dock->checkBox_VisualFilter->setMinimumHeight(26);

	dock->checkBoxHide->setFocusPolicy(Qt::StrongFocus);
	dock->checkBoxMarkers->setFocusPolicy(Qt::StrongFocus);
	dock->checkBoxMarkerIds->setFocusPolicy(Qt::StrongFocus);
	dock->checkBoxEpipolar->setFocusPolicy(Qt::StrongFocus);
	dock->checkBoxRigidBodyConstellation->setFocusPolicy(Qt::StrongFocus);
	dock->checkBoxRigidBodyMeshmodels->setFocusPolicy(Qt::StrongFocus);
	dock->checkBox_DrawFiltered->setFocusPolicy(Qt::StrongFocus);
	dock->checkBox_VisualFilter->setFocusPolicy(Qt::StrongFocus);
#endif

	Shortcuts::getInstance()->installEventFilterToChildren(this);

}

DisplayOptionsDockWidget::~DisplayOptionsDockWidget()
{
	delete dock;
	instance = NULL;
}

DisplayOptionsDockWidget* DisplayOptionsDockWidget::getInstance()
{
	if (!instance)
	{
		instance = new DisplayOptionsDockWidget(MainWindow::getInstance());
		MainWindow::getInstance()->addDockWidget(Qt::TopDockWidgetArea, instance);
	}
	return instance;
}


void DisplayOptionsDockWidget::closeEvent(QCloseEvent* event)
{
	event->ignore();
	MainWindow::getInstance()->on_actionDisplay_Options_triggered(false);
}


void  DisplayOptionsDockWidget::toggleEnabled(bool enabled)
{
	dock->checkBoxMarkers->setEnabled(enabled);
	dock->checkBoxMarkerIds->setEnabled(enabled);
	dock->checkBoxEpipolar->setEnabled(enabled);
	dock->checkBoxRigidBodyConstellation->setEnabled(enabled);
	dock->checkBoxRigidBodyMeshmodels->setEnabled(enabled);
	dock->checkBox_DrawFiltered->setEnabled(enabled);
}

void DisplayOptionsDockWidget::on_checkBoxHide_clicked()
{
	Settings::getInstance()->set("TrialDrawHideAll", dock->checkBoxHide->isChecked());
	toggleEnabled(!dock->checkBoxHide->isChecked());
	MainWindow::getInstance()->redrawGL();
}

void DisplayOptionsDockWidget::on_checkBoxMarkers_clicked()
{
	Settings::getInstance()->set("TrialDrawMarkers", dock->checkBoxMarkers->isChecked());
	MainWindow::getInstance()->redrawGL();
}

void DisplayOptionsDockWidget::on_checkBoxMarkerIds_clicked()
{
	Settings::getInstance()->set("TrialDrawMarkerIds", dock->checkBoxMarkerIds->isChecked());
	MainWindow::getInstance()->redrawGL();
}

void DisplayOptionsDockWidget::on_checkBoxEpipolar_clicked()
{
	Settings::getInstance()->set("TrialDrawEpipolar", dock->checkBoxEpipolar->isChecked());
	MainWindow::getInstance()->redrawGL();
}

void DisplayOptionsDockWidget::on_checkBoxRigidBodyConstellation_clicked()
{
	Settings::getInstance()->set("TrialDrawRigidBodyConstellation", dock->checkBoxRigidBodyConstellation->isChecked());
	MainWindow::getInstance()->redrawGL();
}

void DisplayOptionsDockWidget::on_checkBoxRigidBodyMeshmodels_clicked()
{
	Settings::getInstance()->set("TrialDrawRigidBodyMeshmodels", dock->checkBoxRigidBodyMeshmodels->isChecked());
	MainWindow::getInstance()->redrawGL();
}

void DisplayOptionsDockWidget::on_checkBox_DrawFiltered_clicked()
{
	Settings::getInstance()->set("TrialDrawFiltered", dock->checkBox_DrawFiltered->isChecked());
	MainWindow::getInstance()->redrawGL();
}

void DisplayOptionsDockWidget::on_checkBox_VisualFilter_clicked()
{
	Settings::getInstance()->set("VisualFilterEnabled", dock->checkBox_VisualFilter->isChecked());
	Project::getInstance()->reloadTextures();
	MainWindow::getInstance()->redrawGL();
}

void DisplayOptionsDockWidget::on_toolButton_VisualFilter_clicked()
{
	VisualFilterDialog::getInstance()->exec();
}

void DisplayOptionsDockWidget::toggleHideAll()
{
	dock->checkBoxHide->setChecked(!Settings::getInstance()->getBoolSetting("TrialDrawHideAll"));
	Settings::getInstance()->set("TrialDrawHideAll", dock->checkBoxHide->isChecked());
	toggleEnabled(!dock->checkBoxHide->isChecked());
	MainWindow::getInstance()->redrawGL();
}
