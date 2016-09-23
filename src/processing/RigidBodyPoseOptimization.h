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
///\file RigidBodyPoseOptimization.h
///\author Benjamin Knorlein
///\date 09/20/2016

#ifndef RIGIDBODYPOSEOPTIMIZATION_H_
#define RIGIDBODYPOSEOPTIMIZATION_H_

#include <opencv/cv.h>
#include <QMutex>

namespace xma
{
	class RigidBody;

	class RigidBodyPoseOptimization
	{

	public:
		RigidBodyPoseOptimization(RigidBody * body, int frame);
		virtual ~RigidBodyPoseOptimization();

		void optimizeRigidBodySetup();

		int nbPoints;

		double* p;
		double* x;
		void projError(int n, double* p, double* x);
		void projErrorJac(int n, int m, double* p, double* Jac);

	private:
		void setOutput();

		static QMutex mutex;

		int m_iterations;
		double m_initial;

		std::vector<int> cameraIdx;
		std::vector<cv::Point2d> Pts2D;
		std::vector<cv::Point3d> Pts3D;
		std::vector<cv::Mat> cameraRotationVector;
		std::vector<cv::Mat> cameraTranslationVector;
		std::vector<cv::Mat> cameraMatrix;
		std::vector<cv::Mat> distortion;
		cv::Vec3d chessRotationVector;
		cv::Vec3d chessTranslationVector;

		RigidBody * m_body;
		int m_frame;
	};
}
#endif // RIGIDBODYPOSEOPTIMIZATION_H_


