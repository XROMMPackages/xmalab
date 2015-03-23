#ifndef RIGIDBODYDIALOG_H_
#define RIGIDBODYDIALOG_H_

#include <QDialog>

namespace Ui {
	class RigidBodyDialog;
}

class QLabel;
class QComboBox;

namespace xma{
	class RigidBody;


	class RigidBodyDialog : public QDialog{

		Q_OBJECT

	private:
		Ui::RigidBodyDialog *diag;

		RigidBody * m_body;

		std::vector <QLabel *> label_RefPoint;
		std::vector <QComboBox *> comboBox;
		std::vector <QLabel *> label_DescPoint;

		void updateLabels();
		void updateIcon();
		bool isComplete();
		bool setRigidBodyIdxByDialog();
		void updateColorButton();
	public:
		explicit RigidBodyDialog(RigidBody* body, QWidget *parent = 0);
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
	};
}


#endif /* AboutDialog_H_ */
