/* The computation of the butterworth low pass filter is taken from
* http ://www.exstrom.com/journal/sigproc/liir.c with the following Copyright
*
*                            COPYRIGHT
*
*  liir - Recursive digital filter functions
*  Copyright (C) 2007 Exstrom Laboratories LLC
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  A copy of the GNU General Public License is available on the internet at:
*
*  http://www.gnu.org/copyleft/gpl.html
*
*  or you can write to:
*
*  The Free Software Foundation, Inc.
*  675 Mass Ave
*  Cambridge, MA 02139, USA
*
*  You can contact Exstrom Laboratories LLC via Email at:
*
*  stefan(AT)exstrom.com
*
*  or you can write to:
*
*  Exstrom Laboratories LLC
*  P.O. Box 7651
*  Longmont, CO 80501, USA
*
*------------------------------------
* The filtfilt funtion is taken from http://stackoverflow.com/questions/17675053/matlabs-filtfilt-algorithm/27270420#27270420
*
*/

#ifndef BUTTERWORTHLOWPASSFILTER_H
#define BUTTERWORTHLOWPASSFILTER_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

namespace xma
{
	class ButterworthLowPassFilter
	{
	public:
		ButterworthLowPassFilter(int order, double cutOffFrequency, double recordingFrequency);
		virtual ~ButterworthLowPassFilter();
		void filter(std::vector<double>& in, std::vector<double>& out);

	private:
		int n; // filter order
		double* dcof; // d coefficients
		int* ccof; // c coefficients
		double sf; // scaling factor
	};
}
#endif // BUTTERWORTHLOWPASSFILTER_H


