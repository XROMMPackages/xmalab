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
///\file StatusColorDialog.cpp
///\author Benjamin Knorlein
///\date 8/12/2013

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/StatusColorDialog.h"
#include "ui_StatusColorDialog.h"
#include "core/Settings.h"
#include "ui/MainWindow.h"

using namespace xma;

StatusColorDialog::StatusColorDialog(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::StatusColorDialog)
{
	diag->setupUi(this);
	updateColors();
}

StatusColorDialog::~StatusColorDialog()
{
	delete diag;
}

void StatusColorDialog::updateColors()
{
	QPixmap pix1(16, 16);
	pix1.fill(QColor(Settings::getInstance()->getQStringSetting("ColorInterpolated")));
	diag->toolButton_Interpolated->setIcon(pix1);

	QPixmap pix2(16, 16);
	pix2.fill(QColor(Settings::getInstance()->getQStringSetting("ColorManual")));
	diag->toolButton_Manual->setIcon(pix2);

	QPixmap pix3(16, 16);
	pix3.fill(QColor(Settings::getInstance()->getQStringSetting("ColorManualAndOpt")));
	diag->toolButton_ManualAndOpt->setIcon(pix3);

	QPixmap pix4(16, 16);
	pix4.fill(QColor(Settings::getInstance()->getQStringSetting("ColorSet")));
	diag->toolButton_Set->setIcon(pix4);

	QPixmap pix5(16, 16);
	pix5.fill(QColor(Settings::getInstance()->getQStringSetting("ColorSetAndOpt")));
	diag->toolButton_SetAndOpt->setIcon(pix5);

	QPixmap pix6(16, 16);
	pix6.fill(QColor(Settings::getInstance()->getQStringSetting("ColorTracked")));
	diag->toolButton_Tracked->setIcon(pix6);

	QPixmap pix7(16, 16);
	pix7.fill(QColor(Settings::getInstance()->getQStringSetting("ColorTrackedAndOpt")));
	diag->toolButton_TrackedAndOpt->setIcon(pix7);

	QPixmap pix8(16, 16);
	pix8.fill(QColor(Settings::getInstance()->getQStringSetting("ColorUndefined")));
	diag->toolButton_Undefined->setIcon(pix8);
}

void StatusColorDialog::setColor(QString name)
{
    QColor color = QColorDialog::getColor(QColor(Settings::getInstance()->getQStringSetting(name)), this);
    
    Settings::getInstance()->set(name, color.name());

    updateColors();
    
    MainWindow::getInstance()->redrawGL();
}

void StatusColorDialog::on_toolButton_Interpolated_clicked()
{
	setColor("ColorInterpolated");
}

void StatusColorDialog::on_toolButton_Manual_clicked()
{
	setColor("ColorManual");
}

void StatusColorDialog::on_toolButton_ManualAndOpt_clicked()
{
	setColor("ColorManualAndOpt");
}

void StatusColorDialog::on_toolButton_Set_clicked()
{
	setColor("ColorSet");
}

void StatusColorDialog::on_toolButton_SetAndOpt_clicked()
{
	setColor("ColorSetAndOpt");
}

void StatusColorDialog::on_toolButton_Tracked_clicked()
{
	setColor("ColorTracked");
}

void StatusColorDialog::on_toolButton_TrackedAndOpt_clicked()
{
	setColor("ColorTrackedAndOpt");
}

void StatusColorDialog::on_toolButton_Undefined_clicked()
{
	setColor("ColorUndefined");
}