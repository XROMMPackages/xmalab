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
///\file ErrorDialog.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef ERRORDIALOG_H_
#define ERRORDIALOG_H_

#include <QDialog>
#include <QString>

namespace Ui
{
	class ErrorDialog;
}

namespace xma
{
	class ErrorDialog : public QDialog
	{
		Q_OBJECT

	private:
		Ui::ErrorDialog* diag;
		static ErrorDialog* instance;

	protected:
		ErrorDialog(QWidget* parent = 0);

	public:
		virtual ~ErrorDialog();
		static ErrorDialog* getInstance();

		void showErrorDialog(QString message);

	public slots:
	};
}

#endif /* ErrorDialogDIALOG_H_ */

