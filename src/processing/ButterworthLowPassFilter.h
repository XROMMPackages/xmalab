#ifndef BUTTERWORTHLOWPASSFILTER_H
#define BUTTERWORTHLOWPASSFILTER_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

namespace xma{
	class ButterworthLowPassFilter{

	public:
		ButterworthLowPassFilter(int order, double cutOffFrequency, double recordingFrequency);
		virtual ~ButterworthLowPassFilter();
		void filter(std::vector <double> &in, std::vector <double> &out);

	private:
		int n;            // filter order
		double *dcof;     // d coefficients
		int *ccof;        // c coefficients
		double sf;        // scaling factor
	};
}
#endif  // BUTTERWORTHLOWPASSFILTER_H