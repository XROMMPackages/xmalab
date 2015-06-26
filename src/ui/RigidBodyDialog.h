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

		std::vector <QLabel *> label_Dummy;


		void updateLabels();
		void updateIcon();
		bool isComplete();
		bool setRigidBodyIdxByDialog();
		void updateColorButton();
		void reloadDummyPoints();


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

		void on_pushButton_AddDummy_clicked();
		void on_pushButton_DeleteDummy_clicked();
		void on_pushButton_Reset_clicked();


	};
}


#endif /* AboutDialog_H_ */
