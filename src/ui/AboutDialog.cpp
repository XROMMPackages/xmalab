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
///\file AboutDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/AboutDialog.h"
#include "ui_AboutDialog.h"
#include "ui/GLSharedWidget.h"

#include <QMessageBox>

using namespace xma;

AboutDialog::AboutDialog(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::AboutDialog)
{
	diag->setupUi(this);
	diag->version_label->setText(PROJECT_VERSION);
	diag->date_label->setText(PROJECT_BUILD_TIME);
}

AboutDialog::~AboutDialog()
{
	delete diag;
}

void AboutDialog::on_pushButtonLicense_clicked()
{
	QMessageBox msgBox;
	QSpacerItem* horizontalSpacer = new QSpacerItem(800, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

	QString text = "XMALab -- Copyright \xA9 2015, Brown University, Providence, RI";
	text += "\n\nAll Rights Reserved";
	text += "\n\nUse of the XMALab software is provided under the terms of the GNU General Public License version 3 as published by the Free Software Foundation at http ://www.gnu.org/licenses/gpl-3.0.html, provided that this copyright notice appear in all copies and that the name of Brown University not be used in advertising or publicity pertaining to the use or distribution of the software without specific, written prior permission from Brown University.";
	text += "\n\nXMALab currently uses several libraries which were acquired under open source licenses as follows :";
	text += "\n   GLEW - 3 - clause BSD License";
	text += "\n   OpenCV - 3 - clause BSD License";
	text += "\n   QUAZIP - LGPL 2.0";
	text += "\n   QT4 - LGPL 2.1 (or GPL 3)";
	text += "\n   Levmar - GPL 2.0"; 
	text += "\n   QcustomPlot - GPL 3";
	text += "\n\nBROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS PROVIDED \"AS IS\", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR ANY PARTICULAR PURPOSE.IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.";
	msgBox.setText(text);

	QGridLayout* layout = (QGridLayout*)msgBox.layout();
	layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
	msgBox.exec();
}


void xma::AboutDialog::on_pushButtonOpenGLInfo_clicked()
{
	QMessageBox msgBox;
	QSpacerItem* horizontalSpacer = new QSpacerItem(800, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
	msgBox.setText(GLSharedWidget::getInstance()->getInfo());
	msgBox.setStyleSheet("QMessageBox { messagebox-text-interaction-flags: 5; }");
	QGridLayout* layout = (QGridLayout*)msgBox.layout();
	layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
	msgBox.exec();
}
