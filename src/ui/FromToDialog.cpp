//  ----------------------------------
//  XMALab -- Copyright � 2015, Brown University, Providence, RI.
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
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file FromToDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/FromToDialog.h"
#include "ui_FromToDialog.h"

using namespace xma;

FromToDialog::FromToDialog(int from, int to, int max, bool withFormat, bool withFilter, QWidget* parent) :
	QDialog(parent),
	diag(new Ui::FromToDialog)
{
	diag->setupUi(this);

	if (!withFormat)diag->frameFormat->hide();
	if (!withFilter)diag->frameFilter->hide();

	diag->spinBoxFrom->setMinimum(1);
	diag->spinBoxFrom->setMaximum(max);
	diag->spinBoxFrom->setValue(from);

	diag->spinBoxTo->setMinimum(1);
	diag->spinBoxTo->setMaximum(max);
	diag->spinBoxTo->setValue(to);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

FromToDialog::~FromToDialog()
{
	delete diag;
}

int FromToDialog::getFrom() const
{
	return diag->spinBoxFrom->value();
}

int FromToDialog::getTo() const
{
	return diag->spinBoxTo->value();
}

QString FromToDialog::getFormat() const
{
	if (diag->radioButtonJPG->isChecked())
	{
		return "jpg";
	}
	else if (diag->radioButtonTIF->isChecked())
	{
		return "tif";
	} 
	else
	{
		return "avi";
	}
}

bool FromToDialog::getFiltered() const
{
	return diag->radioFiltered->isChecked();
}

void FromToDialog::on_pushButton_OK_clicked()
{
	this->accept();
}

void FromToDialog::on_pushButton_Cancel_clicked()
{
	this->reject();
}

