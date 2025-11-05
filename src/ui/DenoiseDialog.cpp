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
///\file DenoiseDialog.cpp
///\author Benjamin Knorlein
///\date 10/06/2020

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/DenoiseDialog.h"
#include "ui_DenoiseDialog.h"

using namespace xma;

DenoiseDialog::DenoiseDialog(QWidget* parent) :
   QDialog(parent),
	diag(new Ui::DenoiseDialog)
{
	diag->setupUi(this);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

DenoiseDialog::~DenoiseDialog()
{
	delete diag;
}

int DenoiseDialog::getSearchWindowSize() const {
	return diag->spinBox_searchWindowSize->value();
}

int DenoiseDialog::getTemplateWindowSize() const {
	return diag->spinBox_templateWindowSize->value();
}

int DenoiseDialog::getTemporalWindowSize() const {
	return diag->spinBox_temporalWindowSize->value();
}

int DenoiseDialog::getFilterStrength() const {
	return diag->doubleSpinBox_FilterStrength->value();
}

bool DenoiseDialog::getRelinkTrial() const {
	return diag->checkBox_RelinkTrial->isChecked();
}

void DenoiseDialog::on_pushButtonOK_clicked()
{
	this->accept();
}

void DenoiseDialog::on_pushButtonCancel_clicked()
{
	this->reject();
}

void DenoiseDialog::on_spinBox_searchWindowSize_textChanged(const QString& value) {
	int val = value.toInt();
	if (val % 2 == 0) {
		diag->spinBox_searchWindowSize->setValue(val + 1);
	}
}

void DenoiseDialog::on_spinBox_templateWindowSize_textChanged(const QString& value) {
	int val = value.toInt();
	if (val % 2 == 0) {
		diag->spinBox_templateWindowSize->setValue(val + 1);
	}
}

void DenoiseDialog::on_spinBox_temporalWindowSize_textChanged(const QString& value) {
	int val = value.toInt();
	if (val % 2 == 0) {
		diag->spinBox_temporalWindowSize->setValue(val + 1);
	}
}