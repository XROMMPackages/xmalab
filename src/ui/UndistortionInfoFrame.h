/*
 * ProgressDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef UNDISTORTIONINFOFRAME_H_
#define UNDISTORTIONINFOFRAME_H_

#include <QFrame>

namespace Ui {
	class UndistortionInfoFrame;
}

class Camera;

class UndistortionInfoFrame : public QFrame{

	Q_OBJECT
	
	public:
		~UndistortionInfoFrame();
		UndistortionInfoFrame(QWidget *parent = 0);

		void update(Camera * camera);
	private:
		Ui::UndistortionInfoFrame *frame;
		void getInfo(Camera * camera, QString & inlier, QString &Error);
};



#endif /* UNDISTORTIONINFOFRAME_H_ */
