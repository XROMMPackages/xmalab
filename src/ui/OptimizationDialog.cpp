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
///\file OptimizationDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/OptimizationDialog.h"
#include "ui_OptimizationDialog.h"

using namespace xma;

OptimizationDialog::OptimizationDialog(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::OptimizationDialog)
{
	diag->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

int OptimizationDialog::getIterations() const
{
	return diag->spinBox_Iterations->value();
}

int OptimizationDialog::getMethod() const
{
	if (diag->radioButton_NoDist->isChecked())
	{
		return 0;
	}
	else if (diag->radioButton_NoDistXY->isChecked())
	{
		return 1;
	}
	else if (diag->radioButton_Dist->isChecked())
	{
		return 2;
	}

	return -1;
}

OptimizationDialog::~OptimizationDialog()
{
	delete diag;
}

double OptimizationDialog::getInitial() const
{
	return diag->doubleSpinBox_Initial->value();
}

void OptimizationDialog::on_pushButton_OK_clicked()
{
	this->accept();
}

void OptimizationDialog::on_pushButton_Cancel_clicked()
{
	this->reject();
}

