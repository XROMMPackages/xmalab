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
///\file VisualFilterDialog.h
///\author Benjamin Knorlein
///\date 4/26/2019

#ifndef VISUALFILTERDIALOG_H_
#define VISUALFILTERDIALOG_H_

#include <QDialog>
#include <QString>

namespace Ui
{
	class VisualFilterDialog;
}

namespace xma
{
	class VisualFilterDialog : public QDialog
	{
		Q_OBJECT

	private:
		Ui::VisualFilterDialog* diag;
		static VisualFilterDialog* instance;

	protected:
		VisualFilterDialog(QWidget* parent = 0);

	public:
		virtual ~VisualFilterDialog();
		static VisualFilterDialog* getInstance();


	public slots:

	void on_spinBox_krad_valueChanged(int value);
	void on_doubleSpinBox_gsigma_valueChanged(double value);
	void on_doubleSpinBox_img_wt_valueChanged(double value);
	void on_doubleSpinBox_blur_wt_valueChanged(double value);
	void on_doubleSpinBox_gamma_valueChanged(double value);
	};
}

#endif /* VISUALFILTERDIALOG_H_ */

