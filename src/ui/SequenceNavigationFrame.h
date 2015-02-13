#ifndef SEQUENCENAVIGATIONFRAME_H_
#define SEQUENCENAVIGATIONFRAME_H_

#include <QFrame>
#include <QTimer>
#include "ui/State.h"

namespace Ui {
	class SequenceNavigationFrame;
}

namespace xma{
	class SequenceNavigationFrame : public QFrame{

		Q_OBJECT

	private:
		SequenceNavigationFrame(QWidget *parent = 0);
		static SequenceNavigationFrame* instance;

		Ui::SequenceNavigationFrame *frame;
		int maxFrame;
		int startFrame;
		int endFrame;
		void changeFrame(int frame);

		int play_tag;
		QTimer *play_timer;

		bool updating;
	protected:

	public:
		static SequenceNavigationFrame* getInstance();
		virtual ~SequenceNavigationFrame();
		void setNbImages(int nbImages);
		void setStartEndSequence(int start, int end);

		public slots:
		void activeFrameChanged(int activeFrame);
		void activeTrialChanged(int activeTrial);
		void workspaceChanged(work_state workspace);

		void on_horizontalSlider_valueChanged(int value);
		void on_spinBoxFrame_valueChanged(int value);
		void on_toolButtonNext_clicked();
		void on_toolButtonPrev_clicked();
		void on_toolButtonFrameStart_clicked();
		void on_toolButtonFrameEnd_clicked();
		void on_toolButtonPlay_clicked();
		void on_toolButtonPlayBackward_clicked();
		void on_toolButtonStop_clicked();
		void play_update();
	};
}


#endif /* SEQUENCENAVIGATIONFRAME_H_ */
