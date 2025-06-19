//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
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
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file VisualFilterDialog.cpp
///\author Benjamin Knorlein
///\date 4/26/2019

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/VisualFilterDialog.h"
#include "ui_VisualFilterDialog.h"
#include "core/Settings.h"
#include "core/Project.h"
#include "MainWindow.h"

using namespace xma;

VisualFilterDialog* VisualFilterDialog::instance = NULL;

VisualFilterDialog::VisualFilterDialog(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::VisualFilterDialog)
{
	diag->setupUi(this);
	diag->spinBox_krad->setValue(Settings::getInstance()->getIntSetting("VisualFilter_krad"));
	diag->doubleSpinBox_gsigma->setValue(Settings::getInstance()->getFloatSetting("VisualFilter_gsigma"));
	diag->doubleSpinBox_img_wt->setValue(Settings::getInstance()->getFloatSetting("VisualFilter_img_wt"));
	diag->doubleSpinBox_blur_wt->setValue(Settings::getInstance()->getFloatSetting("VisualFilter_blur_wt"));
	diag->doubleSpinBox_gamma->setValue(Settings::getInstance()->getFloatSetting("VisualFilter_gamma"));

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

VisualFilterDialog::~VisualFilterDialog()
{
	delete diag;
	instance = NULL;
}

VisualFilterDialog* VisualFilterDialog::getInstance()
{
	if (!instance)
	{
		instance = new VisualFilterDialog();
	}
	return instance;
}

void VisualFilterDialog::on_spinBox_krad_valueChanged(int value)
{
	Settings::getInstance()->set("VisualFilter_krad", value);
	Project::getInstance()->reloadTextures();
	MainWindow::getInstance()->redrawGL();
}

void VisualFilterDialog::on_doubleSpinBox_gsigma_valueChanged(double value)
{
	Settings::getInstance()->set("VisualFilter_gsigma", (float) value);
	Project::getInstance()->reloadTextures();
	MainWindow::getInstance()->redrawGL();
}

void VisualFilterDialog::on_doubleSpinBox_img_wt_valueChanged(double value)
{

	Settings::getInstance()->set("VisualFilter_img_wt", (float)value);
	Project::getInstance()->reloadTextures();
	MainWindow::getInstance()->redrawGL();
}


void VisualFilterDialog::on_doubleSpinBox_blur_wt_valueChanged(double value)
{

	Settings::getInstance()->set("VisualFilter_blur_wt", (float)value);
	Project::getInstance()->reloadTextures();
	MainWindow::getInstance()->redrawGL();
}

void VisualFilterDialog::on_doubleSpinBox_gamma_valueChanged(double value)
{

	Settings::getInstance()->set("VisualFilter_gamma", (float)value);
	Project::getInstance()->reloadTextures();
	MainWindow::getInstance()->redrawGL();
}
