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
///\file DenoiseDialog.h
///\author Benjamin Knorlein
///\date 10/06/2020

#ifndef DENOISEDIALOG_H_
#define DENOISEDIALOG_H_

#include <QDialog>
#include <QString>

namespace Ui
{
	class DenoiseDialog;
}

namespace xma
{
	class DenoiseDialog : public QDialog
	{
		Q_OBJECT

	private:
		Ui::DenoiseDialog* diag;

	public:
		explicit DenoiseDialog(QWidget* parent = nullptr);
		~DenoiseDialog() override;

		int getSearchWindowSize() const;
		int getTemplateWindowSize() const;
		int getTemporalWindowSize() const;
		int getFilterStrength() const;
		bool getRelinkTrial() const;

	public slots:
		void on_pushButtonOK_clicked();
		void on_pushButtonCancel_clicked();
		void on_spinBox_searchWindowSize_textChanged(const QString& value);
		void on_spinBox_templateWindowSize_textChanged(const QString& value);
		void on_spinBox_temporalWindowSize_textChanged(const QString& value);
	};
}


#endif /* CONFIRMATIONDIALOG_H_ */

