#ifndef CAMERABOXTRIAL_H_
#define CAMERABOXTRIAL_H_

#include <QWidget>

namespace Ui {
	class CameraBoxTrial;
}

namespace xma{
	class CameraBoxTrial : public QWidget{

		Q_OBJECT

	private:
		QStringList imageFileNames;

	public:
		explicit CameraBoxTrial(QWidget *parent = 0);
		virtual ~CameraBoxTrial();

		Ui::CameraBoxTrial *widget;

		bool isComplete();

		const QStringList& getImageFileNames(){ return imageFileNames; }
		void setCameraName(QString name);
		const QString getCameraName();

		public slots:

		void on_toolButton_clicked();
	};
}
#endif /* CAMERABOXTRIAL_H_ */
