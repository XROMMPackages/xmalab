#ifndef UNDISTORTSEQUENCEDIALOG_H
#define UNDISTORTSEQUENCEDIALOG_H

#include <QDialog>
#include <QString>
#include <QDir>

#include <opencv/cv.h>

namespace Ui {
	class UndistortSequenceDialog;
}

class UndistortSequenceDialog : public QDialog{

	Q_OBJECT



	public:
		explicit UndistortSequenceDialog(QWidget *parent = 0);
		~UndistortSequenceDialog();
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

	public slots:
		void on_toolButton_Input_clicked();
		void on_toolButton_OutputFolder_clicked();
		void on_pushButton_clicked();
		void on_lineEdit_pattern_textChanged(QString text);
};

#endif  // UNDISTORTSEQUENCEDIALOG_H
