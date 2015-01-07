#ifndef AboutDialog_H_
#define AboutDialog_H_

#include <QDialog>

namespace Ui {
	class AboutDialog;
}

namespace xma{
	class AboutDialog : public QDialog{

		Q_OBJECT

	private:
		Ui::AboutDialog *diag;

	public:
		explicit AboutDialog(QWidget *parent = 0);
		virtual ~AboutDialog();

		public slots:
	};
}


#endif /* AboutDialog_H_ */
