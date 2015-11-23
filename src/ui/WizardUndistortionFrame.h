//  ----------------------------------
//  XMA Lab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMA Lab software is provided under the terms of the GNU General Public License version 3 
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
///\file WizardUndistortionFrame.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef WIZARDUNDISTORTIONFRAME_H_
#define WIZARDUNDISTORTIONFRAME_H_

#include <QFrame>
#include "ui/State.h"

namespace Ui
{
	class WizardUndistortionFrame;
}

namespace xma
{
	class WizardUndistortionFrame : public QFrame
	{
		Q_OBJECT

	public:
		virtual ~WizardUndistortionFrame();
		WizardUndistortionFrame(QWidget* parent = 0);

		bool checkForPendingChanges();

	private:
		Ui::WizardUndistortionFrame* frame;

	public slots:
		void undistortionChanged(undistortion_state undistortion);
		void on_pushButton_clicked();

		void on_comboBoxImage_currentIndexChanged(int idx);
		void on_comboBoxPoints_currentIndexChanged(int idx);
		void on_radioButtonMouseClickCenter_clicked(bool checked);
		void on_radioButtonMouseClickOutlier_clicked(bool checked);
		void on_radioButtonMouseClickNone_clicked(bool checked);
		void on_pushButtonRemoveOutlierAutomatically_clicked();
		void computeUndistortion();
		void recomputeUndistortion();
		void undistortionFinished();
	};
}


#endif /* WIZARDUNDISTORTIONFRAME_H_ */

