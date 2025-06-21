//  ----------------------------------
//  XMALab -- Copyright � 2015, Brown University, Providence, RI.
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
//  PROVIDED AS IS, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file LocalUndistortion.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/LocalUndistortion.h" 

#include "ui/ProgressDialog.h"
#include "ui/MainWindow.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Image.h"
#include "core/UndistortionObject.h"
#include "core/Settings.h"

#include <QtCore>
#include <QtConcurrent/QtConcurrent>
#include <stdlib.h>
#include <math.h>

using namespace xma;

int LocalUndistortion::nbInstances = 0;

LocalUndistortion::LocalUndistortion(int camera): QObject()
{
	m_camera = camera;
	nbInstances++;
}

LocalUndistortion::~LocalUndistortion()
{
}

void LocalUndistortion::computeUndistortion(bool recompute)
{
	hasReferences = recompute;

	if (hasReferences)
		Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getGridPoints(tmpPoints_distorted, tmpPoints_references, tmpPoints_inlier);

	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL( finished() ), this, SLOT( localUndistortion_threadFinished() ));

	QFuture<void> future = QtConcurrent::run([this]() { localUndistortion_thread(); });
	m_FutureWatcher->setFuture(future);

	ProgressDialog::getInstance()->showProgressbar(0, 0, "Local Undistortion");
}

void LocalUndistortion::localUndistortion_threadFinished()
{
	detectedPoints.clear();
	tmpPoints_distorted.clear();
	tmpPoints_references.clear();

	map_x.release();
	map_y.release();
	controlPts.release();
	_A.release();
	_B.release();
	_radii.release();

	controlPts_inverse.release();
	A_inverse.release();
	B_inverse.release();
	radii_inverse.release();

	delete m_FutureWatcher;
	nbInstances--;
	if (nbInstances == 0)
	{
		ProgressDialog::getInstance()->closeProgressbar();
		MainWindow::getInstance()->redrawGL();
		emit localUndistortion_finished();
	}
	delete this;
}

void LocalUndistortion::localUndistortion_thread()
{
	if (!hasReferences) setupCorrespondances();

	setPointsByInlier(tmpPoints_distorted, controlPts_inverse);
	setPointsByInlier(tmpPoints_references, controlPts);

	int gridSize = computeLWM(controlPts_inverse, controlPts, _A, _B, _radii);
	createLookupTable(controlPts, _A, _B, _radii, map_x, map_y, gridSize);

	computeLWM(controlPts, controlPts_inverse, A_inverse, B_inverse, radii_inverse);

	Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->setComputed(true);
	if (!hasReferences)
		Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->setGridPoints(tmpPoints_distorted, tmpPoints_references, tmpPoints_inlier);
	Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->setMaps(map_x, map_y);
	Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->setLWMMatrices(_A, _B, _radii, controlPts, A_inverse, B_inverse, radii_inverse, controlPts_inverse);
	Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->undistortPoints();
	Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->setRecalibrationRequired(0);
	Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->setUpdateInfoRequired(true);

	Project::getInstance()->getCameras()[m_camera]->undistort();
}

void LocalUndistortion::setupCorrespondances()
{
	Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getDetectedPoints(detectedPoints);

	tmpPoints_distorted.clear();
	tmpPoints_references.clear();
	tmpPoints_inlier.clear();

	std::vector<cv::Point2d> center;
	if (Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->isCenterSet())
	{
		findNClosestPoint(1, Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getCenter(), center, detectedPoints, false);
	}
	else
	{
		cv::Point2d imageCenter(0.5 * Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getWidth(), 0.5 * Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getHeight());
		findNClosestPoint(1, imageCenter, center, detectedPoints, false);
	}

	double orient = getHexagonalGridOrientation(center[0], detectedPoints) / 180 * M_PI;

	double dY = getHexagonalGridSize(center[0]);

	//rotate the centroids for processing (they will be returned to
	//original location after "true" grid is determined)
	//move rota13.749562tion point to center of the image
	//NOTE could be done as matrix
	double tmpx;
	for (unsigned int i = 0; i < detectedPoints.size(); i ++)
	{
		detectedPoints[i].x -= center[0].x; //translate center to cp
		detectedPoints[i].y -= center[0].y;

		//rotate centroids to match true orientation
		tmpx = sin(orient) * detectedPoints[i].y + cos(orient) * detectedPoints[i].x;
		detectedPoints[i].y = cos(orient) * detectedPoints[i].y - sin(orient) * detectedPoints[i].x;
		detectedPoints[i].x = tmpx;

		//move back to original image coordinate system
		detectedPoints[i].x += center[0].x; //translate center to cp
		detectedPoints[i].y += center[0].y;
	}

	setupHexagonalGrid(center[0], dY);

	for (unsigned int i = 0; i < tmpPoints_distorted.size(); i ++)
	{
		tmpPoints_inlier.push_back(true);
	}

	//rotate back undistorted points
	for (unsigned int i = 0; i < tmpPoints_distorted.size(); i ++)
	{
		tmpPoints_distorted[i].x -= center[0].x; //translate center to cp
		tmpPoints_distorted[i].y -= center[0].y;

		//rotate centroids to match true orientation
		tmpx = sin(-orient) * tmpPoints_distorted[i].y + cos(-orient) * tmpPoints_distorted[i].x;
		tmpPoints_distorted[i].y = cos(-orient) * tmpPoints_distorted[i].y - sin(-orient) * tmpPoints_distorted[i].x;
		tmpPoints_distorted[i].x = tmpx;

		//move back to original image coordinate system
		tmpPoints_distorted[i].x += center[0].x; //translate center to cp
		tmpPoints_distorted[i].y += center[0].y;
	}

	//rotate back reference control points
	for (unsigned int i = 0; i < tmpPoints_references.size(); i ++)
	{
		tmpPoints_references[i].x -= center[0].x; //translate center to cp
		tmpPoints_references[i].y -= center[0].y;

		//rotate centroids to match true orientation
		tmpx = sin(-orient) * tmpPoints_references[i].y + cos(-orient) * tmpPoints_references[i].x;
		tmpPoints_references[i].y = cos(-orient) * tmpPoints_references[i].y - sin(-orient) * tmpPoints_references[i].x;
		tmpPoints_references[i].x = tmpx;

		//move back to original image coordinate system
		tmpPoints_references[i].x += center[0].x; //translate center to cp
		tmpPoints_references[i].y += center[0].y;
	}

	double minXDet = Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getWidth();
	double maxXDet = -Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getWidth();
	double minYDet = Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getHeight();
	double maxYDet = -Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getHeight();
	double minXCont = Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getWidth();
	double maxXCont = -Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getWidth();
	double minYCont = Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getHeight();
	double maxYCont = -Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getHeight();

	for (unsigned int i = 0; i < tmpPoints_distorted.size(); i ++)
	{
		if (tmpPoints_distorted[i].x > maxXDet) maxXDet = tmpPoints_distorted[i].x;
		if (tmpPoints_distorted[i].x < minXDet) minXDet = tmpPoints_distorted[i].x;
		if (tmpPoints_distorted[i].y > maxYDet) maxYDet = tmpPoints_distorted[i].y;
		if (tmpPoints_distorted[i].y < minYDet) minYDet = tmpPoints_distorted[i].y;

		if (tmpPoints_references[i].x > maxXCont) maxXCont = tmpPoints_references[i].x;
		if (tmpPoints_references[i].x < minXCont) minXCont = tmpPoints_references[i].x;
		if (tmpPoints_references[i].y > maxYCont) maxYCont = tmpPoints_references[i].y;
		if (tmpPoints_references[i].y < minYCont) minYCont = tmpPoints_references[i].y;
	}

	double scaleX = (maxXDet - minXDet) / (maxXCont - minXCont);
	double scaleY = (maxYDet - minYDet) / (maxYCont - minYCont);
	//apply scale
	for (unsigned int i = 0; i < tmpPoints_distorted.size(); i ++)
	{
		tmpPoints_references[i].x = scaleX * (tmpPoints_references[i].x - minXCont) + minXDet;
		tmpPoints_references[i].y = scaleY * (tmpPoints_references[i].y - minYCont) + minYDet;
	}
}

int LocalUndistortion::computeLWM(cv::Mat& detectedPts, cv::Mat& controlPts, cv::Mat& A, cv::Mat& B, cv::Mat& radii)
{
	// Check for empty input matrices (fixes OpenCV 4.11.0 compatibility)
	if (detectedPts.empty() || controlPts.empty()) {
		A.release();
		B.release();
		radii.release();
		return -1;
	}
	
	A.release();
	B.release();
	radii.release();

	int nbNeighbours = Settings::getInstance()->getIntSetting("LocalUndistortionNeighbours");

	A.create(cv::Size(6, detectedPts.rows),CV_64F);
	B.create(cv::Size(6, detectedPts.rows),CV_64F);
	radii.create(cv::Size(1, detectedPts.rows),CV_64F);

	cv::Mat X;
	X.create(cv::Size(nbNeighbours, 6),CV_64F);
	cv::Mat ucp;
	ucp.create(cv::Size(nbNeighbours, 1),CV_64F);
	cv::Mat vcp;
	vcp.create(cv::Size(nbNeighbours, 1),CV_64F);
	cv::Mat tmpXY;
	std::vector<cv::Mat> tmpXY_splitted(2); // Pre-allocate with size

	cv::Mat X_inv;
	cv::Mat all_pts_Matx2;
	cv::Mat all_pts_Maty2;
	cv::Mat all_pts_Matx2y2;
	cv::Mat all_pts_MatEuclideanDistance;
	cv::Mat all_pts_Idx;
	double xcp, ycp;
	double maxRadius = 0;

	// Pre-allocate matrices to avoid repeated allocation
	tmpXY.create(controlPts.size(), CV_64FC2);
	all_pts_Matx2.create(controlPts.rows, 1, CV_64F);
	all_pts_Maty2.create(controlPts.rows, 1, CV_64F);
	all_pts_Matx2y2.create(controlPts.rows, 1, CV_64F);
	all_pts_MatEuclideanDistance.create(controlPts.rows, 1, CV_64F);
	all_pts_Idx.create(controlPts.rows, 1, CV_32S);

	for (int i = 0; i < controlPts.rows; i++)
	{
		//find closest point
		{
			//substract pt
			const double curr_x = controlPts.at<double>(i, 0);
			const double curr_y = controlPts.at<double>(i, 1);
			tmpXY = controlPts - cv::Scalar(curr_x, curr_y);

			//split channels to 2 matrices
			cv::split(tmpXY, tmpXY_splitted);
			
			//compute norm of rows - optimized
			cv::multiply(tmpXY_splitted[0], tmpXY_splitted[0], all_pts_Matx2);
			cv::multiply(tmpXY_splitted[1], tmpXY_splitted[1], all_pts_Maty2);
			cv::add(all_pts_Matx2, all_pts_Maty2, all_pts_Matx2y2);
			cv::sqrt(all_pts_Matx2y2, all_pts_MatEuclideanDistance);

			//sortIndices to find closest
			cv::sortIdx(all_pts_MatEuclideanDistance, all_pts_Idx, cv::SORT_EVERY_COLUMN + cv::SORT_ASCENDING);
		}
		
		//set radius of influence
		const double current_radius = all_pts_MatEuclideanDistance.at<double>(all_pts_Idx.at<int>(nbNeighbours - 1));
		radii.at<double>(i) = current_radius;
		if (maxRadius < current_radius) maxRadius = current_radius;
		
		//set up matrix eqn for polynomial of order=2
		//set ucp,vcp and X
		for (int j = 0; j < nbNeighbours; j++)
		{
			const int idx = all_pts_Idx.at<int>(j);
			xcp = controlPts.at<double>(idx, 0);
			ycp = controlPts.at<double>(idx, 1);

			X.at<double>(0, j) = 1.0;
			X.at<double>(1, j) = xcp;
			X.at<double>(2, j) = ycp;
			X.at<double>(3, j) = xcp * ycp;
			X.at<double>(4, j) = xcp * xcp;
			X.at<double>(5, j) = ycp * ycp;

			ucp.at<double>(j) = detectedPts.at<double>(idx, 0);
			vcp.at<double>(j) = detectedPts.at<double>(idx, 1);
		}

		cv::invert(X, X_inv, cv::DECOMP_SVD);
		A.row(i) = ucp * X_inv;
		B.row(i) = vcp * X_inv;
	}
	X.release();
	ucp.release();
	vcp.release();
	tmpXY.release();

	// Matrices are pre-allocated, so release them properly
	for (auto& mat : tmpXY_splitted)
	{
		mat.release();
	}
	tmpXY_splitted.clear();
	X_inv.release();
	all_pts_Matx2.release();
	all_pts_Maty2.release();
	all_pts_Matx2y2.release();
	all_pts_MatEuclideanDistance.release();
	all_pts_Idx.release();

	return static_cast<int>(std::ceil(maxRadius));
}

void LocalUndistortion::setPointsByInlier(std::vector<cv::Point2d>& pts, cv::Mat& ptsInlier)
{
	int count = 0;
	for (std::vector<bool>::iterator mIt = tmpPoints_inlier.begin(); mIt != tmpPoints_inlier.end();)
	{
		if (*mIt)
		{
			count++;
		}
		++mIt;
	}

	ptsInlier.release();
	ptsInlier.create(count, 1,CV_64FC2);

	int count_tmp = 0;
	std::vector<bool>::iterator mIt = tmpPoints_inlier.begin();
	for (std::vector<cv::Point2d>::iterator it = pts.begin()
	     ; it != pts.end(); ++mIt,++it)
	{
		if (*mIt)
		{
			ptsInlier.at<cv::Vec2d>(count_tmp, 0) = *it;
			count_tmp++;
		}
	}
}

void LocalUndistortion::createLookupTable(cv::Mat& controlPts, cv::Mat& A, cv::Mat& B, cv::Mat& radii, cv::Mat& outMat_x, cv::Mat& outMat_y, int gridSize)
{
	outMat_x.release();
	outMat_y.release();

	int m_width = Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getWidth();
	int m_height = Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getHeight();

	outMat_x.create(cv::Size(m_width, m_height), CV_32FC1);
	outMat_y.create(cv::Size(m_width, m_height), CV_32FC1);

	int gwidth = ceil(((double) m_width) / gridSize);
	int gheight = ceil(((double) m_height) / gridSize);

	//vector for holding points in a grid

	std::vector<std::vector<std::vector<int> >> grid_pts;
	for (int x = 0; x < gwidth; x ++)
	{
		std::vector<std::vector<int> > col_pts;
		grid_pts.push_back(col_pts);
		for (int y = 0; y < gheight; y ++)
		{
			std::vector<int> pts;
			grid_pts[x].push_back(pts);
		}
	}

	int xi, yi;
	for (int i = 0; i < controlPts.rows; i++)
	{
		xi = controlPts.at<double>(i, 0) / gridSize;
		yi = controlPts.at<double>(i, 1) / gridSize;
		grid_pts[xi][yi].push_back(i);
	}
	//variables needed
	double dx, dy, dist_to_cp_sq, Ri, Ri2, Ri3; // Use squared distance to avoid sqrt when possible
	double u, v, w;
	int i;
	double xy, x2, y2, xg, yg;
	double u_numerator;
	double v_numerator;
	double denominator;

	// Pre-calculate grid size factors to avoid repeated division
	const double inv_gridSize = 1.0 / gridSize;
	const double gridWidth = ceil(((double)m_width) / gridSize);
	const double gridHeight = ceil(((double)m_height) / gridSize);

	// Pre-allocate pointers for faster matrix access
	float* outMat_x_ptr = outMat_x.ptr<float>(0);
	float* outMat_y_ptr = outMat_y.ptr<float>(0);

	//for all pixel in output image compute the coordinates in the original image
	//we take the shift of the center already into account

	for (int y_out = 0, processedpixel = 0; y_out < m_height; y_out++)
	{
		// Cache y_out calculations
		const double y_out_d = static_cast<double>(y_out);
		const double y2_cached = y_out_d * y_out_d;
		
		for (int x_out = 0; x_out < m_width; x_out++, processedpixel++)
		{
			/* precalculate factors */
			const double x_out_d = static_cast<double>(x_out);
			xy = x_out_d * y_out_d;
			x2 = x_out_d * x_out_d;
			y2 = y2_cached; // Use cached value

			//compute the UV coordinates with all points
			u_numerator = 0.0;
			v_numerator = 0.0;
			denominator = 0.0;			// Optimize grid bounds calculation - but keep original loop variable types for exact compatibility
			const double xg_min = (std::max)(floor(x_out_d * inv_gridSize) - 1, 0.0);
			const double xg_max = (std::min)(ceil(x_out_d * inv_gridSize) + 1, gridWidth);
			const double yg_min = (std::max)(floor(y_out_d * inv_gridSize) - 1, 0.0);
			const double yg_max = (std::min)(ceil(y_out_d * inv_gridSize) + 1, gridHeight);

			for (double xg = xg_min; xg < xg_max; xg++)
			{
				for (double yg = yg_min; yg < yg_max; yg++)
				{
					const std::vector<int>& grid_cell = grid_pts[static_cast<int>(xg)][static_cast<int>(yg)];
					for (size_t p = 0; p < grid_cell.size(); p++)
					{
						i = grid_cell[p];
						//without ptrs
						dx = x_out_d - controlPts.at<double>(i, 0);
						dy = y_out_d - controlPts.at<double>(i, 1);

						dist_to_cp_sq = dx * dx + dy * dy;
						const double radius_i = radii.at<double>(i);
						
						// Use squared comparison to avoid sqrt
						if (dist_to_cp_sq < radius_i * radius_i)
						{
							const double dist_to_cp = sqrt(dist_to_cp_sq);
							Ri = dist_to_cp / radius_i;
							Ri2 = Ri * Ri;
							Ri3 = Ri * Ri2;
							w = 1.0 - 3.0 * Ri2 + 2.0 * Ri3; /* weight for ControlPoint i */

							//without ptrs - use cached values
							u = A.at<double>(i, 0) + A.at<double>(i, 1) * x_out_d + A.at<double>(i, 2) * y_out_d +
								A.at<double>(i, 3) * xy + A.at<double>(i, 4) * x2 + A.at<double>(i, 5) * y2;

							v = B.at<double>(i, 0) + B.at<double>(i, 1) * x_out_d + B.at<double>(i, 2) * y_out_d +
								B.at<double>(i, 3) * xy + B.at<double>(i, 4) * x2 + B.at<double>(i, 5) * y2;

							u_numerator += w * u;
							v_numerator += w * v;
							denominator += w;
						}
					}
				}
			}

			if (denominator != 0.0)
			{
				//set uv and apply shift
				//qDebug("pt %i %i to %lf %lf",x_out,y_out,u_numerator/denominator + center_detectedPts.x,v_numerator/denominator + center_detectedPts.y );
				//without ptrs

				outMat_x_ptr[processedpixel] = static_cast<float>(u_numerator / denominator);
				outMat_y_ptr[processedpixel] = static_cast<float>(v_numerator / denominator);
			}
			else
			{
				/*
				 * no control points influence this (x,y)
				 * issue warning, and set warning flag to warn user that there
				 * are one or more such points
				 */

				outMat_x_ptr[processedpixel] = -1.0f;				outMat_y_ptr[processedpixel] = -1.0f;
			}
		}
	}

	//vector for holding points in a grid
	for (int x = 0; x < gwidth; x ++)
	{
		for (int y = 0; y < gheight; y ++)
		{
			grid_pts[x][y].clear();
		}
		grid_pts[x].clear();
	}
	grid_pts.clear();
}

bool LocalUndistortion::findNClosestPoint(int numberPoints, cv::Point2d pt, std::vector<cv::Point2d>& closest_pts, std::vector<cv::Point2d> all_pts, bool skipfirst)
{
	// Check if input vector is empty (fixes OpenCV 4.11.0 compatibility)
	if (all_pts.empty()) {
		closest_pts.clear();
		return false;
	}
	
	cv::Mat all_pts_Mat(all_pts, true);
	//substract pt
	all_pts_Mat = all_pts_Mat - cv::Scalar(pt.x, pt.y);

	//split channels to 2 matrices
	std::vector<cv::Mat> all_pts_Matsplitted;
	cv::split(all_pts_Mat, all_pts_Matsplitted);

	//compute norm of rows
	cv::Mat all_pts_Matx2;
	cv::Mat all_pts_Maty2;
	cv::Mat all_pts_Matx2y2;
	cv::Mat all_pts_MatEuclideanDistance;	cv::multiply(all_pts_Matsplitted.at(0), all_pts_Matsplitted.at(0), all_pts_Matx2);
	cv::multiply(all_pts_Matsplitted.at(1), all_pts_Matsplitted.at(1), all_pts_Maty2);
	cv::add(all_pts_Matx2, all_pts_Maty2, all_pts_Matx2y2);
	cv::sqrt(all_pts_Matx2y2, all_pts_MatEuclideanDistance);

	//sort by lowest Distance
	cv::Mat all_pts_Idx;
	cv::sortIdx(all_pts_MatEuclideanDistance, all_pts_Idx, cv::SORT_EVERY_COLUMN + cv::SORT_ASCENDING);

	closest_pts.clear();

	for (int j = (skipfirst ? 1 : 0); j < (skipfirst ? (numberPoints + 1) : numberPoints); j++)
	{
		closest_pts.push_back(all_pts[all_pts_Idx.at<int>(j)]);
	}

	//cleanUp
	all_pts_Mat.release();
	for (unsigned int j = 0; j < all_pts_Matsplitted.size(); j++)
	{
		all_pts_Matsplitted.at(0).release();
	}
	all_pts_Matsplitted.clear();

	all_pts_Matx2.release();
	all_pts_Maty2.release();
	all_pts_Matx2y2.release();
	all_pts_MatEuclideanDistance.release();
	all_pts_Idx.release();

	return closest_pts.size() > 0;
}

double LocalUndistortion::getHexagonalGridSize(cv::Point2d center)
{
	//set the cell center to center distance
	std::vector<cv::Point2d> neigh = get6adjCells(center, detectedPoints);
	double dY = 0;
	for (unsigned int j = 0; j < neigh.size(); j++)
	{
		dY += sqrt((neigh[j].x - center.x) * (neigh[j].x - center.x) + (neigh[j].y - center.y) * (neigh[j].y - center.y));
	}
	dY /= 6.0;

	return dY;
}

double LocalUndistortion::getHexagonalGridOrientation(cv::Point2d center, std::vector<cv::Point2d> all_pts)
{
	std::vector<cv::Point2d> closestToCenter;
	findNClosestPoint(6, center, closestToCenter, all_pts, true);

	cv::Mat angles = cv::Mat::zeros(1, 6,CV_64F);
	cv::Mat perfectlyPlacedGrid = cv::Mat::zeros(1, 6,CV_64F);
	for (int i = 0; i < 6; i++)
	{
		angles.at<double>(i) = (180.0 / M_PI) * (atan2(closestToCenter[i].y - center.y, closestToCenter[i].x - center.x));
	}

	cv::Mat tmpMat;
	cv::Mat tmpMatIdx;

	// reorder six
	// angles in radians from the center point to each of the six points
	tmpMat = abs(angles + cv::Scalar(90));
	cv::sortIdx(tmpMat, tmpMatIdx, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
	perfectlyPlacedGrid.at<double>(tmpMatIdx.at<int>(0)) = 90;

	tmpMat = abs(angles + cv::Scalar(30));
	cv::sortIdx(tmpMat, tmpMatIdx, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
	perfectlyPlacedGrid.at<double>(tmpMatIdx.at<int>(0)) = 30;

	tmpMat = abs(angles + cv::Scalar(-30));
	cv::sortIdx(tmpMat, tmpMatIdx, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
	perfectlyPlacedGrid.at<double>(tmpMatIdx.at<int>(0)) = -30;

	tmpMat = abs(angles + cv::Scalar(-90));
	cv::sortIdx(tmpMat, tmpMatIdx, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
	perfectlyPlacedGrid.at<double>(tmpMatIdx.at<int>(0)) = -90;

	tmpMat = abs(angles + cv::Scalar(-150));
	cv::sortIdx(tmpMat, tmpMatIdx, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
	perfectlyPlacedGrid.at<double>(tmpMatIdx.at<int>(0)) = -150;

	tmpMat = abs(angles + cv::Scalar(150));
	cv::sortIdx(tmpMat, tmpMatIdx, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
	perfectlyPlacedGrid.at<double>(tmpMatIdx.at<int>(0)) = 150;

	cv::Scalar gridOrient = cv::mean(angles + perfectlyPlacedGrid);

	angles.release();
	perfectlyPlacedGrid.release();
	tmpMat.release();
	tmpMatIdx.release();
	closestToCenter.clear();

	return gridOrient.val[0];
}

std::vector<cv::Point2d> LocalUndistortion::get6adjCells(cv::Point2d center, std::vector<cv::Point2d> all_pts)
{
	//returns the 6 closest cells to cp
	//[note: dist is for all centroids, not just neighborhood of cp.]
	std::vector<cv::Point2d> closestToCenter;
	findNClosestPoint(6, center, closestToCenter, all_pts, true);

	cv::Mat angles = cv::Mat::zeros(1, 6,CV_64F);
	std::vector<cv::Point2d> sixN;

	for (int i = 0; i < 6; i++)
	{
		angles.at<double>(i) = (180.0 / M_PI) * (atan2(closestToCenter[i].y - center.y, closestToCenter[i].x - center.x));
	}
	cv::Mat tmpMat;
	cv::Mat tmpMatIdx;

	// reorder six
	// angles in radians from the center point to each of the six points
	tmpMat = abs(angles + cv::Scalar(90));
	cv::sortIdx(tmpMat, tmpMatIdx, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
	sixN.push_back(closestToCenter[tmpMatIdx.at<int>(0)]);

	tmpMat = abs(angles + cv::Scalar(30));
	cv::sortIdx(tmpMat, tmpMatIdx, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
	sixN.push_back(closestToCenter[tmpMatIdx.at<int>(0)]);

	tmpMat = abs(angles + cv::Scalar(-30));
	cv::sortIdx(tmpMat, tmpMatIdx, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
	sixN.push_back(closestToCenter[tmpMatIdx.at<int>(0)]);

	tmpMat = abs(angles + cv::Scalar(-90));
	cv::sortIdx(tmpMat, tmpMatIdx, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
	sixN.push_back(closestToCenter[tmpMatIdx.at<int>(0)]);

	tmpMat = abs(angles + cv::Scalar(150));
	cv::sortIdx(tmpMat, tmpMatIdx, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
	sixN.push_back(closestToCenter[tmpMatIdx.at<int>(0)]);

	tmpMat = abs(angles + cv::Scalar(-150));
	cv::sortIdx(tmpMat, tmpMatIdx, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
	sixN.push_back(closestToCenter[tmpMatIdx.at<int>(0)]);

	angles.release();
	tmpMat.release();
	tmpMatIdx.release();
	closestToCenter.clear();

	return sixN;
}

bool LocalUndistortion::contains(cv::Point2d centercont, std::vector<cv::Point2d>& pts)
{
	for (auto pt : pts)
	{
		if (pt == centercont) return true;
	}

	return false;
}

void LocalUndistortion::checkAndAddPoint(cv::Point2d centerdet, cv::Point2d ptA1cont, cv::Point2d ptA1det, double thresh, std::vector<double>& dy_vec, bool & newPoint)
{
	std::vector<cv::Point2d> ptB1det;
	if (findNClosestPoint(1, ptA1det, ptB1det, detectedPoints, false) &&
		//Control point not yet present
		!contains(ptA1cont, tmpPoints_references) &&
		//Detected point not yet present
		!contains(ptB1det[0], tmpPoints_distorted) &&
		//detected point close to expected one
		(sqrt((ptA1det.x - ptB1det[0].x) * (ptA1det.x - ptB1det[0].x) + (ptA1det.y - ptB1det[0].y) * (ptA1det.y - ptB1det[0].y)) < thresh))
	{
		tmpPoints_references.push_back(cv::Point2d(ptA1cont.x, ptA1cont.y));
		tmpPoints_distorted.push_back(cv::Point2d(ptB1det[0].x, ptB1det[0].y));
		dy_vec.push_back(sqrt((centerdet.x - ptB1det[0].x) * (centerdet.x - ptB1det[0].x) + (centerdet.y - ptB1det[0].y) * (centerdet.y - ptB1det[0].y)));
		newPoint = true;
	}

}

bool LocalUndistortion::addNeighbours(cv::Point2d centercont, cv::Point2d centerdet, double dY, double dX, double dOffY, double dYdist, std::vector<double>& dy_vec)
{
	bool newPoint = false;
	double dXdist = sin(M_PI / 3.0) * dYdist;
	double dOffYdist = 0.5 * dYdist;
	double thresh = dOffYdist;

	cv::Point2d ptA1cont;
	cv::Point2d ptA1det;
	std::vector<cv::Point2d> ptB1det;

	//Test 1st Point dY
	ptA1cont = cv::Point2d(centercont.x, centercont.y + dY);
	ptA1det = cv::Point2d(centerdet.x, centerdet.y + dYdist);
	checkAndAddPoint(centerdet, ptA1cont, ptA1det, thresh, dy_vec, newPoint);

	//Test 2nd Point -dY
	ptA1cont = cv::Point2d(centercont.x, centercont.y - dY);
	ptA1det = cv::Point2d(centerdet.x, centerdet.y - dYdist);
	checkAndAddPoint(centerdet, ptA1cont, ptA1det, thresh, dy_vec, newPoint);

	//Test 3rd Point dX dOffY  
	ptA1cont = cv::Point2d(centercont.x + dX, centercont.y + dOffY);
	ptA1det = cv::Point2d(centerdet.x + dXdist, centerdet.y + dOffYdist);
	checkAndAddPoint(centerdet, ptA1cont, ptA1det, thresh, dy_vec, newPoint);

	//Test 4th Point -dX dOffY  
	ptA1cont = cv::Point2d(centercont.x - dX, centercont.y + dOffY);
	ptA1det = cv::Point2d(centerdet.x - dXdist, centerdet.y + dOffYdist);
	checkAndAddPoint(centerdet, ptA1cont, ptA1det, thresh, dy_vec, newPoint);

	//Test 5th Point dX -dOffY  
	ptA1cont = cv::Point2d(centercont.x + dX, centercont.y - dOffY);
	ptA1det = cv::Point2d(centerdet.x + dXdist, centerdet.y - dOffYdist);
	checkAndAddPoint(centerdet, ptA1cont, ptA1det, thresh, dy_vec, newPoint);

	//Test 6th Point -dX -dOffY  
	ptA1cont = cv::Point2d(centercont.x - dX, centercont.y - dOffY);
	ptA1det = cv::Point2d(centerdet.x - dXdist, centerdet.y - dOffYdist);
	checkAndAddPoint(centerdet, ptA1cont, ptA1det, thresh, dy_vec, newPoint);

	return newPoint;
}

void LocalUndistortion::setupHexagonalGrid(cv::Point2d center, double dY)
{
	qDebug("Setup hexagonal grid correspondances Smart");

	tmpPoints_distorted.clear();
	tmpPoints_references.clear();

	//tmpCheck.clear();
	std::vector<double> dy_vec;

	//set first point to the point closest to the center of the image
	tmpPoints_distorted.push_back(center);
	tmpPoints_references.push_back(center);
	dy_vec.push_back(dY);

	double dX = sin(M_PI / 3.0) * dY;
	double dOffY = 0.5 * dY;

	for (unsigned int i = 0; i < dy_vec.size(); i++)
	{
		addNeighbours(tmpPoints_references[i], tmpPoints_distorted[i], dY, dX, dOffY, dy_vec[i], dy_vec);
	}
	dy_vec.clear();
}

