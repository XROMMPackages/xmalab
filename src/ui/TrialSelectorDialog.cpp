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
///\file TrialSelectorDialog.cpp
///\author Benjamin Knorlein
///\date 07/08/2016

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/TrialSelectorDialog.h"
#include "ui_TrialSelectorDialog.h"
#include "core/Trial.h"

using namespace xma;

TrialSelectorDialog::TrialSelectorDialog(std::vector<Trial*> trials, QWidget* parent) :
	QDialog(parent),
	diag(new Ui::TrialSelectorDialog)
{
	diag->setupUi(this);
	//diag->listWidget->addItem()
	for (std::vector<Trial*>::const_iterator trial_it = trials.begin(); trial_it != trials.end(); ++trial_it)
	{
		m_trials.push_back(*trial_it);
		diag->listWidget->addItem((*trial_it)->getName());
	}
}

TrialSelectorDialog::~TrialSelectorDialog()
{
	m_trials.clear();
	delete diag;
}

std::vector<Trial *> TrialSelectorDialog::getTrials()
{
	std::vector<Trial*> out_trials;
	QList<QListWidgetItem *> items = diag->listWidget->selectedItems();
	for (std::vector<Trial*>::const_iterator trial_it = m_trials.begin(); trial_it != m_trials.end(); ++trial_it)
	{
		for (int i = 0; i < items.size(); i++)
		{
			if (items[i]->text() == (*trial_it)->getName())
			{
				//std::cerr << (*trial_it)->getName().toAscii().data() << std::endl;
				out_trials.push_back((*trial_it));

			}
		}
	}
	return out_trials;
}


void TrialSelectorDialog::on_pushButton_OK_clicked()
{
	this->accept();
}

void TrialSelectorDialog::on_pushButton_Cancel_clicked()
{
	this->reject();
}

