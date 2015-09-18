#ifndef IMPORTEXPORTPOINTSDIALOG_H
#define IMPORTEXPORTPOINTSDIALOG_H

#include <QDialog>

namespace Ui {
	class ImportExportPointsDialog;
}

namespace xma{
	class ImportExportPointsDialog : public QDialog{

		Q_OBJECT

	public:
		explicit ImportExportPointsDialog(QWidget *parent = 0);
		virtual ~ImportExportPointsDialog();
		Ui::ImportExportPointsDialog *diag;

	protected:

	private:
		void switchGroups();
		bool importCSV();
		bool importXMA();
		bool copyFromTrial();

	public slots:
		void on_radioButtonImportCSV_clicked(bool checked);
		void on_radioButtonImportXMA_clicked(bool checked);
		void on_radioButtonTrial_clicked(bool checked);

		void on_toolButtonMarkersXMA_clicked();
		void on_toolButtonMarkersCSV_clicked();

		void on_pushButtonCancel_clicked();
		void on_pushButtonOK_clicked();
	};
}

#endif  // IMPORTEXPORTPOINTSDIALOG_H