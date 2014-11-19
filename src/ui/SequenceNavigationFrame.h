/*
 * ProgressDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef SEQUENCENAVIGATIONFRAME_H_
#define SEQUENCENAVIGATIONFRAME_H_

#include <QFrame>


namespace Ui {
	class SequenceNavigationFrame;
}

class SequenceNavigationFrame : public QFrame{

	Q_OBJECT

	private:
		Ui::SequenceNavigationFrame *frame;
		int maxFrame;

	protected:
		
	public:
		~SequenceNavigationFrame();
		SequenceNavigationFrame(QWidget *parent = 0);
		void setNbImages(int nbImages);
		
	public slots:
		void activeFrameChanged(int activeFrame);

		void on_horizontalSlider_valueChanged(int value);
		void on_spinBox_valueChanged(int value);
		void on_toolButtonNext_clicked();
		void on_toolButtonPrev_clicked();

};



#endif /* SEQUENCENAVIGATIONFRAME_H_ */
