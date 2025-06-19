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
///\file PointImportExportDialog.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef POINTIMPORTEXPORTDIALOG_H_
#define POINTIMPORTEXPORTDIALOG_H_

#include <QDialog>

namespace Ui
{
	class PointImportExportDialog;
}

namespace xma
{
	enum ImportExportType
	{
		IMPORT2D = 0,
		EXPORT2D = 1,
		EXPORT3D = 2,
		EXPORTTRANS = 3
	};

	class PointImportExportDialog : public QDialog
	{
		Q_OBJECT

	private:
		Ui::PointImportExportDialog* diag;

		ImportExportType m_type;

	public:
		explicit PointImportExportDialog(ImportExportType type, QWidget* parent = nullptr);
		~PointImportExportDialog() override;

	public slots:

		void on_radioButton_Single_toggled(bool value);
		void on_radioButton_Multi_toggled(bool value);
		void on_radioButton_Count0_toggled(bool value);
		void on_radioButton_Count1_toggled(bool value);
		void on_radioButton_Header_toggled(bool value);
		void on_radioButton_NoHeader_toggled(bool value);
		void on_radioButton_YDown_toggled(bool value);
		void on_radioButton_YUp_toggled(bool value);
		void on_radioButton_Distorted_toggled(bool value);
		void on_radioButton_Undistorted_toggled(bool value);
		void on_radioButton_NoCols_toggled(bool value);
		void on_radioButton_OffsetCols_toggled(bool value);
		void on_radioButton_Unfiltered_toggled(bool value);
		void on_radioButton_Filtered_toggled(bool value);
		void on_radioButton_Set_toggled(bool value);
		void on_radioButton_Tracked_toggled(bool value);


		void on_pushButton_Cancel_clicked();
		void on_pushButton_OK_clicked();
	};
}


#endif /* APOINTIMPORTEXPORTDIALOG_H_ */

