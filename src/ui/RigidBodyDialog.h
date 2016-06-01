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
///\file RigidBodyDialog.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef RIGIDBODYDIALOG_H_
#define RIGIDBODYDIALOG_H_

#include <QDialog>

namespace Ui
{
	class RigidBodyDialog;
}

class QLabel;
class QComboBox;

namespace xma
{
	class RigidBody;


	class RigidBodyDialog : public QDialog
	{
		Q_OBJECT

	private:
		Ui::RigidBodyDialog* diag;

		RigidBody* m_body;

		std::vector<QLabel *> label_RefPoint;
		std::vector<QComboBox *> comboBox;
		std::vector<QLabel *> label_DescPoint;

		std::vector<QLabel *> label_Dummy;


		void updateLabels();
		void updateIcon();
		bool isComplete();
		bool setRigidBodyIdxByDialog();
		void updateColorButton();
		void reloadDummyPoints();


	public:
		explicit RigidBodyDialog(RigidBody* body, QWidget* parent = 0);
		virtual ~RigidBodyDialog();

	public slots:

		//Footer buttons
		void on_pushButton_OK_clicked();
		void on_pushButton_Cancel_clicked();
		void on_pushButton_setFromFile_clicked();
		void currentIndexChanged(int idx);

		void on_checkBox_Draw_clicked();
		void on_toolButton_Color_clicked();

		void on_checkBoxCutoffOverride_clicked();
		void on_doubleSpinBoxCutoff_valueChanged(double value);

		void on_pushButton_AddDummy_clicked();
		void on_pushButton_DeleteDummy_clicked();
		void on_pushButton_Reset_clicked();
	};
}


#endif /* AboutDialog_H_ */

