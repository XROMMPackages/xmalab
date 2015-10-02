#ifndef UPDATETRIALFRAME_H
#define UPDATETRIALFRAME_H


#include "processing/ThreadedProcessing.h"

#include <QFutureWatcher>
#include <QObject>

namespace xma{
	class Trial;

	class UpdateTrialFrame : public ThreadedProcessing{

		Q_OBJECT;

	public:
		UpdateTrialFrame(Trial* trial, int frame);
		virtual ~UpdateTrialFrame();

	protected:
		void process() override;
		void process_finished() override;

	private:
		int m_frame;
		Trial* m_trial;

	
	};
}
#endif  // MARKERTRACKING_H