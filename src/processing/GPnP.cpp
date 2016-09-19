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
///\file GPnP.cpp
///\author Benjamin Knorlein
///\date 09/14/2016



#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/GPnP.h" 
#include "core/Project.h" 
#include "core/Trial.h" 
#include "core/Camera.h" 
#include "core/CalibrationImage.h" 
#include <unsupported/Eigen/Polynomials>

#include <fstream>

namespace GPnP{

	std::vector<std::vector<double> > computeK(std::vector<std::vector<double> > plucker, std::vector<double> dist){
		double Q11 = plucker[0][0];
		double Q12 = plucker[0][1];
		double Q13 = plucker[0][2];
		double Q14 = plucker[0][3];
		double Q15 = plucker[0][4];
		double Q16 = plucker[0][5];
		
		double Q21 = plucker[1][0];
		double Q22 = plucker[1][1];
		double Q23 = plucker[1][2];
		double Q24 = plucker[1][3];
		double Q25 = plucker[1][4];
		double Q26 = plucker[1][5];

		double Q31 = plucker[2][0];
		double Q32 = plucker[2][1];
		double Q33 = plucker[2][2];
		double Q34 = plucker[2][3];
		double Q35 = plucker[2][4];
		double Q36 = plucker[2][5];

		double dist12 = dist[0];
		double dist13 = dist[1];
		double dist23 = dist[2];

		std::vector<std::vector<double> > A0;
		for (int i = 0; i < 3; i++){
			A0.push_back(std::vector<double>(6));
		}
		double t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18, t19, t20, t21, t22;
#include "processing/GPnPFuncs/computeCoefficiants.h" 

		return A0;
	}

	std::vector<double> computeCoefficiantsL3(std::vector<std::vector<double> > k_coeff){
		std::vector<double> A0(9);
		A0[0] = computeCoefficiantsL3_1(k_coeff);
		A0[1] = computeCoefficiantsL3_2(k_coeff);
		A0[2] = computeCoefficiantsL3_3(k_coeff);
		A0[3] = computeCoefficiantsL3_4(k_coeff);
		A0[4] = computeCoefficiantsL3_5(k_coeff);
		A0[5] = computeCoefficiantsL3_6(k_coeff);
		A0[6] = computeCoefficiantsL3_7(k_coeff);
		A0[7] = computeCoefficiantsL3_8(k_coeff);
		A0[8] = computeCoefficiantsL3_9(k_coeff);

		return A0;
	}


	std::vector<double> solvePolynomial(std::vector<double> coeffs)
	{
		std::vector<double> roots;
		//const int deg = coeffs.size() - 1;
		Eigen::PolynomialSolver<double, 8> solver;
		Eigen::VectorXd coeff(coeffs.size() - 1);

		for (int i = 1; i < coeffs.size(); i++)
			coeff[i - 1] = coeffs[coeffs.size() - i] / coeffs[0];


		std::cout << "In : ";
		for (int i = 0; i<coeff.rows(); ++i)
		{
			std::cout << coeff[i] << " , ";
		}
		std::cout << std::endl;

		solver.compute(coeff);
		
		const Eigen::PolynomialSolver<double, 8>::RootsType & r = solver.roots();
		
		std::cout << "Out : ";
		for (int i = 0; i<r.rows(); ++i)
		{
			//if (r[i].)
			//roots.push_back(r[i]);
			std::cout << r[i] << " , ";
		}
		std::cout << std::endl;

		return roots;
	}

	std::vector<cv::Point3d> compute3Dpoints(xma::Trial * trial, std::vector<int> cameras, std::vector<cv::Point2d> points2D, std::vector<cv::Point3d> points3D)
	{
		int trialreferenceCalibration = trial->getReferenceCalibrationImage();
		std::vector<std::vector<double> > plucker;
		//compute Plucker lines
		for (int i = 0; i < 3; i++){
			cv::Mat Rot;
			Rot.create(3, 3, CV_64F);
			cv::Rodrigues(xma::Project::getInstance()->getCameras()[cameras[i]]->getCalibrationImages()[trialreferenceCalibration]->getRotationVector(),Rot);
			cv::Mat trans = xma::Project::getInstance()->getCameras()[cameras[i]]->getCalibrationImages()[trialreferenceCalibration]->getTranslationVector();
			cv::Mat K_inv = xma::Project::getInstance()->getCameras()[cameras[i]]->getCameraMatrix().inv();
			cv::Point3d pt3 = cv::Point3d(points2D[i].x, points2D[i].y, 1);
			cv::Mat pt3_mat = cv::Mat(pt3, true);
			cv::Mat pt3_world = K_inv*pt3_mat;
			pt3_world = pt3_world / cv::norm(pt3_world);
			Rot = Rot.inv();
			
			cv::Mat q2 = Rot * -trans;
			cv::Mat q1 = Rot * pt3_world;
			q1 = q1 / norm(q1);
			std::vector<double> pluck;
			for (int i = 0; i < 3; i++)
				pluck.push_back(q1.at<double>(i, 0));
			for (int i = 0; i < 3; i++)
				pluck.push_back(q2.at<double>(i, 0));

			plucker.push_back(pluck);
		}

		//compute distances;
		std::vector<double> dist;
		dist.push_back(norm(points3D[0] - points3D[1]));
		dist.push_back(norm(points3D[0] - points3D[2]));
		dist.push_back(norm(points3D[1] - points3D[2]));

		//compute Ks
		std::vector<std::vector<double> > k_coeff = computeK(plucker, dist);

		//for (int i = 0; i < dist.size(); i++){
		//	std::cerr << "pt3 " << i << " : ";
		//	std::cerr << points3D[i].x << " , " << points3D[i].y << " , " << points3D[i].z;
		//	std::cerr << std::endl;
		//}

		//for (int i = 0; i < dist.size(); i++){
		//	std::cerr << "dist " << i << " : ";
		//	std::cerr << dist[i];
		//	std::cerr << std::endl;
		//}

		//for (int i = 0; i < plucker.size(); i++){
		//	std::cerr << "plucker " << i << " : ";
		//	for (int k = 0; k < plucker[i].size(); k++)
		//		std::cerr << plucker[i][k] << ", ";
		//	std::cerr << std::endl;
		//}


		for (int i = 0; i < k_coeff.size(); i++){
			std::cerr << "k_coeff " << i << " : ";
				for (int k = 0; k < k_coeff[i].size(); k++)
					std::cerr << k_coeff[i][k] << ", ";
				std::cerr << std::endl;
		}


		//compute Coefficiants for L3
		std::vector<double> l3_coeff = computeCoefficiantsL3(k_coeff);

		/*std::ofstream outfile_Points("coefs.txt");
		outfile_Points.precision(150);
		outfile_Points << "a = [";
		for (unsigned int j = 0; j < l3_coeff.size(); j++)
		{
			outfile_Points << l3_coeff[j];
			if (j != l3_coeff.size() - 1)outfile_Points << " , ";
		}
		outfile_Points << "]";
		outfile_Points.close();*/

		//std::cerr << "L3_coeff " << " : ";
		//for (int k = 0; k < l3_coeff.size(); k++)
		//	std::cerr << l3_coeff[k] << ", ";
		//std::cerr << std::endl;
		

		solvePolynomial(l3_coeff);
		return points3D;
	}
}