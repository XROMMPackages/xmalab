#ifndef CAMERABOX_H_
#define CAMERABOX_H_

#include <QWidget>

namespace Ui {
	class CameraBox;
}
namespace xma{
	class CameraBox : public QWidget{

		Q_OBJECT

	private:
		QStringList imageFileNames;

	public:
		explicit CameraBox(QWidget *parent = 0);
		virtual ~CameraBox();

		Ui::CameraBox *widget;

		bool isComplete();

		const QStringList& getImageFileNames(){ return imageFileNames; }
		const bool hasUndistortion();
		const QString getUndistortionGridFileName();
		const QString getCameraName();
		void setCameraName(QString name);
		bool isLightCamera();

		public slots:

		void on_toolButtonImages_clicked();
		void on_toolButtonUndistortionGrid_clicked();

		void on_radioButtonXRay_clicked();
		void on_radioButtonLightCamera_clicked();

		void on_checkBoxUndistortionGrid_stateChanged(int state);
	};
}

#endif /* CAMERABOX_H_ */
