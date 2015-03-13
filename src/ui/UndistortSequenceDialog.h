#ifndef UNDISTORTSEQUENCEDIALOG_H
#define UNDISTORTSEQUENCEDIALOG_H

#include <QDialog>
#include <QString>
#include <QDir>

#include <opencv/cv.h>

namespace Ui {
	class UndistortSequenceDialog;
}

namespace xma{
	class UndistortSequenceDialog : public QDialog{

		Q_OBJECT

	public:
		explicit UndistortSequenceDialog(QWidget *parent = 0);
		virtual ~UndistortSequenceDialog();
		Ui::UndistortSequenceDialog *diag;
	protected:

	private:
		QStringList fileNames;
		QString outputfolder;
		QString commonPrefixString;
		QString commonPostfixString;

		QString commonPrefix(QStringList fileNames);
		QString commonPostfix(QStringList fileNames);
		int getNumber(QStringList fileNames);
		QString getFilename(QFileInfo fileinfo, int number);
		bool overwriteFile(QString filename, bool &overwrite);
		void updatePreview();

		public slots:
		void on_toolButton_Input_clicked();
		void on_toolButton_OutputFolder_clicked();
		void on_pushButton_clicked();
		void on_lineEdit_pattern_textChanged(QString text);
		void on_spinBox_NumberStart_valueChanged(int i);
		void on_spinBox_NumberLength_valueChanged(int i);
	};
}

#endif  // UNDISTORTSEQUENCEDIALOG_H
