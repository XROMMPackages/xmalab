#ifndef CALIBRATIONINFOFRAME_H_
#define CALIBRATIONINFOFRAME_H_

#include <QFrame>

namespace Ui {
	class CalibrationInfoFrame;
}

namespace xma{
	class Camera;

	class CalibrationInfoFrame : public QFrame{

		Q_OBJECT

	public:
		virtual ~CalibrationInfoFrame();
		CalibrationInfoFrame(QWidget *parent = 0);

		void update(Camera * camera);
		void updateFrame(Camera * camera);

	private:
		Ui::CalibrationInfoFrame *frame;
		void getCameraInfo(Camera * camera, QString & CameraCenter, QString & FocalLength, QString & FramesCalibrated, QString & ErrorAllDist, QString & ErrorAllUndist);
		void getInfoFrame(Camera * camera, int frame, QString & ErrorCurrentDist, QString &  ErrorCurrentUndist, QString &  RotationVector, QString & TranslationVector);
		QString getInfoInlier(Camera * camera, int frame);
	};
}


#endif /* CALIBRATIONINFOFRAME_H_ */
