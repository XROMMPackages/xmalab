#ifndef MARKERDIALOG_H_
#define MARKERDIALOG_H_

#include <QDialog>

namespace Ui {
	class MarkerDialog;
}

namespace xma{
	class 
		Marker;


	class MarkerDialog : public QDialog{

		Q_OBJECT

	private:
		Ui::MarkerDialog *diag;

		Marker * m_marker;

		bool isComplete();

	public:
		explicit MarkerDialog(Marker* marker, QWidget *parent = 0);
		virtual ~MarkerDialog();

		public slots:

		//Footer buttons
		void on_pushButton_OK_clicked();
		void on_pushButton_Cancel_clicked();
	};
}


#endif /* AboutDialog_H_ */
