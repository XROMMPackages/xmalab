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
///\file TrialSelectorDialog.h
///\author Benjamin Knorlein
///\date 07/08/2016

#ifndef TRIALSELECTORDIALOG_H_
#define TRIALSELECTORDIALOG_H_

#include <QDialog>

namespace Ui
{
	class TrialSelectorDialog;
}

namespace xma
{
	class Trial;
	class TrialSelectorDialog : public QDialog
	{
		Q_OBJECT

	private:
		Ui::TrialSelectorDialog* diag;
		std::vector<Trial*> m_trials;
	public:
		TrialSelectorDialog(std::vector<Trial*> trials, QWidget* parent = 0);
		virtual ~TrialSelectorDialog();
		std::vector<Trial *> getTrials();
	public slots:
		void on_pushButton_OK_clicked();
		void on_pushButton_Cancel_clicked();
	};
}


#endif /* TRIALSELECTORDIALOG_H_ */

