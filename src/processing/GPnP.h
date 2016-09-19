//  ----------------------------------
//  XMALab -- Copyright © 2015, Brown University, Providence, RI.
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
//  PROVIDED “AS IS”, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file GPnP.h
///\author Benjamin Knorlein
///\date 09/14/2016

#ifndef GPnP_H
#define GPnP_H

#include <opencv/cv.h>
namespace xma{
	class Trial;
}

namespace GPnP
{
	std::vector<std::vector<double> > computeK(std::vector<std::vector<double> > plucker, std::vector<double> dist);
	std::vector<double> computeCoefficiantsL3(std::vector<std::vector<double> > k_coeff);
	std::vector<double> solvePolynomial(std::vector<double> coeffs);
	std::vector<cv::Point3d> compute3Dpoints(xma::Trial * trial, std::vector<int> cameras, std::vector<cv::Point2d> points2D, std::vector<cv::Point3d> points3D);

	double computeCoefficiantsL3_1(std::vector<std::vector<double> > k_coeff);
	double computeCoefficiantsL3_2(std::vector<std::vector<double> > k_coeff);
	double computeCoefficiantsL3_3(std::vector<std::vector<double> > k_coeff);
	double computeCoefficiantsL3_4(std::vector<std::vector<double> > k_coeff);
	double computeCoefficiantsL3_5(std::vector<std::vector<double> > k_coeff);
	double computeCoefficiantsL3_6(std::vector<std::vector<double> > k_coeff);
	double computeCoefficiantsL3_7(std::vector<std::vector<double> > k_coeff);
	double computeCoefficiantsL3_8(std::vector<std::vector<double> > k_coeff);
	double computeCoefficiantsL3_9(std::vector<std::vector<double> > k_coeff);
}
#endif // GPnP_H


