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
///\file TrialImportDeleteDialog.cpp
///\author Benjamin Knorlein
///\date 05/30/2018

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/TrialImportDeleteDialog.h"
#include "ui_TrialImportDeleteDialog.h"
#include <iostream>

using namespace xma;

TrialImportDeleteDialog::TrialImportDeleteDialog(QStringList items, bool deleteDialog, QWidget* parent) :
	QDialog(parent),
	diag(new Ui::TrialImportDeleteDialog)
{
	diag->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	diag->listWidget->addItems(items);

	if (deleteDialog)
	{
		this->setWindowTitle("Select the trials to delete");
		diag->pushButton->setText("Delete");
	} else
	{
		this->setWindowTitle("Select the trials to import");
		diag->pushButton->setText("Import");
	}

}

TrialImportDeleteDialog::~TrialImportDeleteDialog()
{
	delete diag;
}

void TrialImportDeleteDialog::on_pushButton_clicked()
{
	for (auto &q : diag->listWidget->selectedItems()){
		list << q->text();
	}
	this->accept();
}