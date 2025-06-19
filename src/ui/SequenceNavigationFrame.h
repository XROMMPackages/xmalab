//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMALab software is provided under the terms of the GNU General Public License version 3 
//  as published by the Free Software Foundation at http://www.gnu.org/licenses/gpl-3.0.html, provided 
//  that this copyright notice appear in all copies and that the name of Brown University not be used in 
//  advertising or publicity pertaining to the use or distribution of the software without specific written 
//  prior permission from Brown University.
//  
//  See license.txt for further information.
//  
//  BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS 
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file SequenceNavigationFrame.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef SEQUENCENAVIGATIONFRAME_H_
#define SEQUENCENAVIGATIONFRAME_H_

#include <QFrame>
#include <QTimer>
#include "ui/State.h"

namespace Ui
{
	class SequenceNavigationFrame;
}

namespace xma
{
	class SequenceNavigationFrame : public QFrame
	{
		Q_OBJECT

	private:
		SequenceNavigationFrame(QWidget* parent = 0);
		static SequenceNavigationFrame* instance;

		Ui::SequenceNavigationFrame* frame;
		int maxFrame;
		int startFrame;
		int endFrame;
		void changeFrame(int frame);

		int play_tag;
		QTimer* play_timer;

		bool updating;

		void moveNFrames(int n);
		bool setNFrames();

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

		void moveNFramesForward();
		void moveNFramesBackward();
		void setAndMoveNFramesForward();
		void setAndMoveNFramesBackward();
		void moveFrameToMissingForward();
		void moveFrameToMissingBackward();
	};
}


#endif /* SEQUENCENAVIGATIONFRAME_H_ */

