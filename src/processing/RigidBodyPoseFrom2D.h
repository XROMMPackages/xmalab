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
///\file RigidBodyPoseFrom2D.h
///\author Benjamin Knorlein
///\date 09/22/2016

#ifndef RIGIDBODYPOSEFROM2D_H_
#define RIGIDBODYPOSEFROM2D_H_

#include <opencv/cv.h>

namespace xma
{
	class RigidBody;

	class RigidBodyPoseFrom2D
	{

	public:
		RigidBodyPoseFrom2D(RigidBody * body, int frame);
		virtual ~RigidBodyPoseFrom2D();

		void findFrom2Dwith3D(std::vector<cv::Point3d> &src, std::vector <cv::Point3d> &dst);

	private:
		std::vector<std::vector<double > > getPluckerForMissing();
		std::vector< cv::Point3d> get3DPoint(std::vector<double> plucker, cv::Point3d reference, cv::Point3d known3D, cv::Point3d knownReference);
		std::vector<cv::Point3d> Pts3D;
		double computeAlignementError(std::vector<cv::Point3d> &src, std::vector <cv::Point3d> &dst);

		RigidBody * m_body;
		int m_frame;
	};
}
#endif // RIGIDBODYPOSEFROM2D_H_


