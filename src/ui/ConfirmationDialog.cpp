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
///\file ConfirmationDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/ConfirmationDialog.h"
#include "ui_ConfirmationDialog.h"

using namespace xma;

ConfirmationDialog* ConfirmationDialog::instance = NULL;

ConfirmationDialog::ConfirmationDialog(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::ConfirmationDialog)
{
	diag->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

ConfirmationDialog::~ConfirmationDialog()
{
	delete diag;
	instance = NULL;
}

ConfirmationDialog* ConfirmationDialog::getInstance()
{
	if (!instance)
	{
		instance = new ConfirmationDialog();
	}
	return instance;
}

namespace xma {

bool ConfirmationDialog::showConfirmationDialog(const QString& message, bool NoButton)
{
	if (NoButton) {
		diag->pushButton_Cancel->setText("No");
		diag->pushButton_OK->setText("Yes");
	}
	else
	{
		diag->pushButton_Cancel->setText("Cancel");
		diag->pushButton_OK->setText("OK");
	}
	diag->message->setText(message);
	int dialogResult = this->exec();
	return (dialogResult == QDialog::Accepted);
}

} // namespace xma

void ConfirmationDialog::on_pushButton_OK_clicked()
{
	this->accept();
}

void ConfirmationDialog::on_pushButton_Cancel_clicked()
{
	this->reject();
}

