#ifndef OPTIMIZATIONDIALOG_H_
#define OPTIMIZATIONDIALOG_H_

#include <QDialog>
#include <QString>

namespace Ui {
	class OptimizationDialog;
}

namespace xma{
	class OptimizationDialog : public QDialog{

		Q_OBJECT

	private:
		Ui::OptimizationDialog *diag;

	protected:
		
	public:
		virtual ~OptimizationDialog();
		OptimizationDialog(QWidget *parent = 0);

		int getIterations();
		int getMethod();
		double getInitial();

		public slots:
		void on_pushButton_OK_clicked();
		void on_pushButton_Cancel_clicked();
	};
}


#endif /* CONFIRMATIONDIALOG_H_ */
