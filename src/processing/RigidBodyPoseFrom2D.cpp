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
///\file RigidBodyPoseFrom2D.cpp
///\author Benjamin Knorlein
///\date 09/22/2016

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/RigidBodyPoseFrom2D.h" 

#include "ui/MainWindow.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/RigidBody.h"
#include "core/Marker.h"
#include "core/CalibrationImage.h"
#include "core/CalibrationObject.h"

#include <math.h>
#include "../../../../libraries/Qt/4.8.6/src/gui/painting/qprinter.h"


using namespace xma;



RigidBodyPoseFrom2D::RigidBodyPoseFrom2D(RigidBody * body, int frame) : m_frame(frame), m_body(body)
{
}

RigidBodyPoseFrom2D::~RigidBodyPoseFrom2D()
{

}

void RigidBodyPoseFrom2D::findFrom2Dwith3D(std::vector<cv::Point3d> &src, std::vector <cv::Point3d> &dst)
{
	std::vector<std::vector<double> > plucker = getPluckerForMissing();

	if (plucker.size() + dst.size() < 3) return;

	std::vector<std::vector<cv::Point3d> > src_Solutions;
	std::vector<std::vector<cv::Point3d> > dst_Solutions;
	std::vector<int> idx;

	for (int i = 0; i < plucker.size(); i++)
	{
		std::vector<cv::Point3d> src_Sol;
		std::vector<cv::Point3d> dst_Sol;
		for (int r = 0; r < dst.size(); r++)
		{
			cv::vector <cv::Point3d> pts = get3DPoint(plucker[i], Pts3D[i], src[r], dst[r]);
			for (int s = 0; s < pts.size(); s++)
			{
				src_Sol.push_back(pts[s]);
			}
		}
		src_Solutions.push_back(src_Sol);
	}

	// check which solution has the best alignement
	std::vector<double> distances;
	std::vector<std::vector<int> > combinations;

	for (int i = 0; i < src_Solutions.size(); i++)
	{
		std::vector<std::vector<int> > combinations_old = combinations;
		combinations.clear();
		for (int k = 0; k < src_Solutions[i].size(); k++){			
			if (combinations_old.size() == 0)
			{
				std::vector<int> combination;
				combination.push_back(k);
				combinations.push_back(combination);
			}
			else{
				for (int p = 0; p < combinations_old.size(); p++){
					std::vector<int> combination = combinations_old[p];
					combination.push_back(k);
					combinations.push_back(combination);
				}
			}
		}
	}
	std::vector<int> best_combination;
	double best_dist = 100000;
	for (int i = 0; i < combinations.size(); i ++)
	{
		std::vector<cv::Point3d> src_tmp;
		std::vector<cv::Point3d> dst_tmp;

		//add known 3d points
		for (int k = 0; k < src.size(); k++)
		{
			src_tmp.push_back(src[k]);
			dst_tmp.push_back(dst[k]);
		}
		//add possible candidates
		for (int p = 0; p < combinations[i].size(); p++){
			src_tmp.push_back(src_Solutions[p][combinations[i][p]]);
			dst_tmp.push_back(Pts3D[p]);
		}

		double dist = computeAlignementError(src_tmp, dst_tmp);

		if (dist < best_dist)
		{
			best_dist = dist;
			best_combination = combinations[i];
		}
	}

	for (int i = 0; i < best_combination.size(); i++)
	{
		dst.push_back(Pts3D[i]);
		src.push_back(src_Solutions[i][best_combination[i]]);
	}
}

double RigidBodyPoseFrom2D::computeAlignementError(std::vector<cv::Point3d> &src, std::vector <cv::Point3d> &dst)
{
	cv::vector<cv::vector<double> > y, x;
	cv::vector<cv::vector<double> > Y, X;
	cv::vector<double> vnl_tmp(3), yg(3, 0), xg(3, 0);
	cv::vector<double> tmp;
	cv::Mat K = cv::Mat::zeros(3, 3, CV_64F);

	//set Data
	for (unsigned int i = 0; i < src.size(); i++)
	{
		vnl_tmp[0] = src[i].x;
		vnl_tmp[1] = src[i].y;
		vnl_tmp[2] = src[i].z;
		X.push_back(vnl_tmp);

		vnl_tmp[0] = dst[i].x;
		vnl_tmp[1] = dst[i].y;
		vnl_tmp[2] = dst[i].z;
		Y.push_back(vnl_tmp);
	}
	// Compute the new coordinates of the 3D points
	// relative to each coordinate frame
	for (unsigned int i = 0; i < X.size(); i++)
	{
		vnl_tmp = X[i];
		for (int m = 0; m < 3; m++)xg[m] = xg[m] + vnl_tmp[m];
		x.push_back(vnl_tmp);

		vnl_tmp = Y[i];
		for (int m = 0; m < 3; m++)yg[m] = yg[m] + vnl_tmp[m];
		y.push_back(vnl_tmp);
	}
	// Compute the gravity center
	for (int m = 0; m < 3; m++)xg[m] /= X.size();
	for (int m = 0; m < 3; m++)yg[m] /= Y.size();

	// Barycentric coordinates
	for (unsigned int i = 0; i < x.size(); i++)
	{
		for (int m = 0; m < 3; m++) x[i][m] = x[i][m] - xg[m];
		for (int m = 0; m < 3; m++) y[i][m] = y[i][m] - yg[m];
	}

	// Compute the cavariance matrix K = Sum_i( yi.xi' )
	for (unsigned int i = 0; i < x.size(); i++)
	{
		for (int j = 0; j < 3; j++)
		{
			for (int l = 0; l < 3; l++)
			{
				vnl_tmp[l] = x[i][j] * y[i][l];
				K.at<double>(l, j) = vnl_tmp[l] + K.at<double>(l, j);
			}
		}
	}

	cv::SVD svd;
	cv::Mat W;
	cv::Mat U;
	cv::Mat VT;

	svd.compute(K, W, U, VT);
	cv::Mat V = VT.t();
	double detU = cv::determinant(U);
	double detV = cv::determinant(V);

	cv::Mat Sd = cv::Mat::zeros(3, 3, CV_64F);
	Sd.at<double>(0, 0) = 1.0;
	Sd.at<double>(1, 1) = 1.0;
	Sd.at<double>(2, 2) = detU * detV;

	cv::Mat rotMatTmp;
	rotMatTmp = U * Sd * VT;

	cv::Mat xgmat = cv::Mat(xg, true);
	cv::Mat ygmat = cv::Mat(yg, true);
	cv::Mat t = ygmat - rotMatTmp * xgmat;
	cv::Vec3d translationvector = cv::Vec3d(t);

	double dist = 0;
	for (unsigned int i = 0; i < dst.size(); i++)
	{
		cv::Mat xmat = cv::Mat(dst[i], true);
		cv::Mat tmp_mat;

		
		tmp_mat = rotMatTmp.t() * ((xmat)-cv::Mat(translationvector));
		
		cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
				tmp_mat.at<double>(1, 0),
				tmp_mat.at<double>(2, 0));

		cv::Point3d diff = src[i] - pt3d;
		dist += cv::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
	}
	return dist;
}

std::vector< cv::Point3d> RigidBodyPoseFrom2D::get3DPoint(std::vector<double> plucker, cv::Point3d reference, cv::Point3d known3D, cv::Point3d knownReference)
{
	std::vector<cv::Point3d> points;
	double Q1 = plucker[0];
	double Q2 = plucker[1];
	double Q3 = plucker[2];
	double Q4 = plucker[3];
	double Q5 = plucker[4];
	double Q6 = plucker[5];
	double dist12 = cv::norm(reference - knownReference);

	double pt3d1 = known3D.x;
	double pt3d2 = known3D.y;
	double pt3d3 = known3D.z;

	double t2 = Q1*Q1;
	double t3 = Q2*Q2;
	double t4 = Q3*Q3;
	double t5 = Q4*Q4;
	double t6 = Q6*Q6;
	double t7 = Q5*Q5;
	double t8 = dist12*dist12;
	double t9 = pt3d1*pt3d1;
	double t10 = pt3d3*pt3d3;
	double t11 = pt3d2*pt3d2;
	double t12 = t2 + t3 + t4;
	if (t12 == 0)return points;
	double t13 = 1.0 / t12;
	double t14 = t2*t8;
	double t15 = t3*t8;
	double t16 = t4*t8;
	double t17 = Q4*pt3d1*t3*2.0;
	double t18 = Q5*pt3d2*t2*2.0;
	double t19 = Q4*pt3d1*t4*2.0;
	double t20 = Q6*pt3d3*t2*2.0;
	double t21 = Q5*pt3d2*t4*2.0;
	double t22 = Q6*pt3d3*t3*2.0;
	double t23 = Q1*Q2*Q4*Q5*2.0;
	double t24 = Q1*Q3*Q4*Q6*2.0;
	double t25 = Q2*Q3*Q5*Q6*2.0;
	double t26 = Q1*Q2*pt3d1*pt3d2*2.0;
	double t27 = Q1*Q3*pt3d1*pt3d3*2.0;
	double t28 = Q2*Q3*pt3d2*pt3d3*2.0;
	double t29 = t14 + t15 + t16 + t17 + t18 + t19 + t20 + t21 + t22 + t23 + t24 + t25 + t26 + t27 + t28 - t2*t6 - t3*t5 - t2*t7 - t3*t6 - t4*t5 - t4*t7 - t2*t10 - t3*t9 - t2*t11 - t3*t10 - t4*t9 - t4*t11 - Q1*Q2*Q4*pt3d2*2.0 - Q1*Q2*Q5*pt3d1*2.0 - Q1*Q3*Q4*pt3d3*2.0 - Q1*Q3*Q6*pt3d1*2.0 - Q2*Q3*Q5*pt3d3*2.0 - Q2*Q3*Q6*pt3d2*2.0;
	
	double t31 = Q1*pt3d1;
	double t32 = Q2*pt3d2;
	double t33 = Q3*pt3d3; 
	if (t29 > 0){
		double t30 = sqrt(t29);
		double l1 = t13*(t30 + t31 + t32 + t33 - Q1*Q4 - Q2*Q5 - Q3*Q6);
		cv::Point3d pt1;
		pt1.x = Q4 + Q1*l1;
		pt1.y = Q5 + Q2*l1;
		pt1.z = Q6 + Q3*l1;
		if(l1 > 0) points.push_back(pt1);

		double l2 = -t13*(t30 - t31 - t32 - t33 + Q1*Q4 + Q2*Q5 + Q3*Q6);
		cv::Point3d pt2;
		pt2.x = Q4 + Q1*l2;
		pt2.y = Q5 + Q2*l2;
		pt2.z = Q6 + Q3*l2;
		if (l2 > 0) points.push_back(pt2);
	} 
	else
	{
		std::cerr << t29 << std::endl;
		//SET CLOSEST;
		double l1 = t13*(t31 + t32 + t33 - Q1*Q4 - Q2*Q5 - Q3*Q6);
		cv::Point3d pt1;
		pt1.x = Q4 + Q1*l1;
		pt1.y = Q5 + Q2*l1;
		pt1.z = Q6 + Q3*l1;
		if (l1 > 0) points.push_back(pt1);
	}

	return points;
}

std::vector<std::vector<double > > RigidBodyPoseFrom2D::getPluckerForMissing()
{
	std::vector<std::vector<double> > plucker;
	//compute Plucker lines
	for (int c = 0; c < Project::getInstance()->getCameras().size(); c++)
	{
		for (int p = 0; p < m_body->getPointsIdx().size(); p++){
			if (m_body->getTrial()->getMarkers()[m_body->getPointsIdx()[p]]->getStatus2D()[c][m_frame] > UNDEFINED &&
				m_body->getTrial()->getMarkers()[m_body->getPointsIdx()[p]]->getStatus3D()[m_frame] <= UNDEFINED)
			{
				cv::Mat Rot;
				Rot.create(3, 3, CV_64F);
				cv::Rodrigues(xma::Project::getInstance()->getCameras()[c]->getCalibrationImages()[m_body->getTrial()->getReferenceCalibrationImage()]->getRotationVector(), Rot);
				cv::Mat trans = xma::Project::getInstance()->getCameras()[c]->getCalibrationImages()[m_body->getTrial()->getReferenceCalibrationImage()]->getTranslationVector();
				cv::Mat K_inv = xma::Project::getInstance()->getCameras()[c]->getCameraMatrix().inv();

				cv::Point2d pt2d = Project::getInstance()->getCameras()[c]->undistortPoint(m_body->getTrial()->getMarkers()[m_body->getPointsIdx()[p]]->getPoints2D()[c][m_frame], true);
				cv::Point3d pt3 = cv::Point3d(pt2d.x, pt2d.y, 1);
				cv::Mat pt3_mat = cv::Mat(pt3, true);
				cv::Mat pt3_world = K_inv*pt3_mat;
				pt3_world = pt3_world / cv::norm(pt3_world);
				Rot = Rot.inv();

				cv::Mat q2 = Rot * -trans;
				cv::Mat q1 = Rot * pt3_world;

				q1 = q1 / cv::norm(q1);
				std::vector<double> pluck;
				for (int i = 0; i < 3; i++)
					pluck.push_back(q1.at<double>(i, 0));
				for (int i = 0; i < 3; i++)
					pluck.push_back(q2.at<double>(i, 0));
				plucker.push_back(pluck);

				Pts3D.push_back(m_body->getReferencePoints()[p]);
			}
		}
	}
	return plucker;
}