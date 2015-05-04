#ifndef POINTIMPORTEXPORTDIALOG_H_
#define POINTIMPORTEXPORTDIALOG_H_

#include <QDialog>

namespace Ui {
	class PointImportExportDialog;
}

namespace xma{
	enum ImportExportType { IMPORT2D = 0, EXPORT2D = 1, EXPORT3D = 2 , EXPORTTRANS = 3};

	class PointImportExportDialog : public QDialog{

		Q_OBJECT

	private:
		Ui::PointImportExportDialog *diag;

		ImportExportType m_type;

	public:
		explicit PointImportExportDialog(ImportExportType type, QWidget *parent = 0);
		virtual ~PointImportExportDialog();

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


		void on_pushButton_Cancel_clicked();
		void on_pushButton_OK_clicked();

	};
}


#endif /* APOINTIMPORTEXPORTDIALOG_H_ */
