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
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file Marker.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Marker.h"
#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/HelperFunctions.h"

#include "processing/ButterworthLowPassFilter.h" //should move this dependency
#include "processing/cubic.h" //should move this dependency

#include <fstream>
#include "Settings.h"

using namespace xma;

Marker::Marker(int nbCameras, int size, Trial* _trial)
{
	init(nbCameras, size);
	trial = _trial;
	meanSize = -1;
	thresholdOffset = Settings::getInstance()->getIntSetting("DefaultMarkerThreshold");
	sizeOverride = -1;
	maxPenalty = 50;
	point3D_ref_set = false;
	method = 0;
	//interpolation = 0;
	requiresRecomputation = true;
	hasInterpolation = false;
}

Marker::~Marker()
{
	clear();
}

void Marker::setDescription(QString _description)
{
	description = _description;
}

QString Marker::getDescription()
{
	return description;
}

const std::vector<std::vector<markerStatus> >& Marker::getStatus2D()
{
	return status2D;
}

std::vector<std::vector<cv::Point2d> >& Marker::getPoints2D()
{
	return points2D;
}

std::vector<std::vector<cv::Point2d> >& Marker::getPoints2D_projected()
{
	return points2D_projected;
}

std::vector<std::vector<double> >& Marker::getError2D()
{
	return error2D;
}

std::vector<cv::Point3d>& Marker::getPoints3D()
{
	return points3D;
}

void Marker::setReference3DPoint(double x, double y, double z)
{
	point3D_ref.x = x;
	point3D_ref.y = y;
	point3D_ref.z = z;
	point3D_ref_set = true;
}

void Marker::loadReference3DPoint(QString filename)
{
	std::ifstream fin(filename.toStdString());
	std::istringstream in;
	std::string line;
	littleHelper::safeGetline(fin, line);
	in.str(line);
	std::vector<double> tmp;
	for (double value; in >> value; littleHelper::comma(in))
	{
		tmp.push_back(value);
	}
	fin.close();

	if (tmp.size() == 3)
	{
		setReference3DPoint(tmp[0], tmp[1], tmp[2]);
	}
	tmp.clear();
}

void Marker::saveReference3DPoint(QString filename)
{
	std::ofstream outfile(filename.toStdString());
	outfile.precision(12);
	outfile << point3D_ref.x << "," << point3D_ref.y << "," << point3D_ref.z << std::endl;
	outfile.close();
}

cv::Point3d Marker::getReference3DPoint()
{
	return point3D_ref;
}

bool Marker::Reference3DPointSet()
{
	return point3D_ref_set;
}

const std::vector<markerStatus>& Marker::getStatus3D()
{
	return status3D;
}

void Marker::setPoint(int camera, int activeFrame, double x, double y, markerStatus status)
{
	points2D[camera][activeFrame].x = x;
	points2D[camera][activeFrame].y = y;
	status2D[camera][activeFrame] = status;

	reconstruct3DPoint(activeFrame);
}

//void Marker::movePoint(int camera, int activeFrame, double x, double y)
//{
//	points2D[camera][activeFrame].x = x;
//	points2D[camera][activeFrame].y = y;
//	status2D[camera][activeFrame] = SET;
//	 
//	reconstruct3DPoint(activeFrame);
//}
//
//void Marker::setTrackedPoint(int camera, int activeFrame, double x, double y)
//{
//	points2D[camera][activeFrame].x = x;
//	points2D[camera][activeFrame].y = y;
//	status2D[camera][activeFrame] = TRACKED;
//
//	reconstruct3DPoint(activeFrame);
//}

int Marker::getMarkerPrediction(int camera, int frame, double& x, double& y, bool forward)
{
	if (frame < 0 || frame >= (int) points2D[0].size())
		return 0;

	int dir = forward ? 1 : -1;

	if (frame - 1 * dir >= 0 && frame - 1 * dir < (int) points2D[0].size() && status2D[camera][frame - 1 * dir] > 0)
	{
		if (frame - 2 * dir >= 0 && frame - 2 * dir < (int) points2D[0].size() && status2D[camera][frame - 2 * dir] > 0)
		{
			//linear position
			x = 2 * points2D[camera][frame - 1 * dir].x - points2D[camera][frame - 2 * dir].x;
			y = 2 * points2D[camera][frame - 1 * dir].y - points2D[camera][frame - 2 * dir].y;
			return 2;
		}
		else
		{
			//previous position
			x = points2D[camera][frame - 1 * dir].x;
			y = points2D[camera][frame - 1 * dir].y;
			return 1;
		}
	}
	else
	{
		return 0;
	}
}

void Marker::reconstruct3DPoint(int frame, bool updateAll)
{
	status3D[frame] = UNDEFINED;

	if (Project::getInstance()->getCalibration() == NO_CALIBRATION)
		return;

	switch (Settings::getInstance()->getIntSetting("TriangulationMethod"))
	{
	default:
	case 0:
		reconstruct3DPointZisserman(frame);
		break;
	case 1:
		reconstruct3DPointZissermanIncremental(frame);
		break;
	case 2:
		reconstruct3DPointZissermanMatlab(frame);
		break;
	case 3:
		reconstruct3DPointZissermanIncrementalMatlab(frame);
		break;
	case 4:
		reconstruct3DPointRayIntersection(frame);
		break;
	}

	if (!updateAll)trial->resetRigidBodyByMarker(this, frame);
}

void Marker::reconstruct3DPointZisserman(int frame)
{
	int count = 0;
	for (unsigned int i = 0; i < status2D.size(); i++)
	{
		if (status2D[i][frame] > 0) count++;
	}

	if (count >= 2)
	{
		markerStatus status = MANUAL_AND_OPTIMIZED;
		cv::Mat f;
		f.create(4, 1, CV_64F);

		cv::Mat A;
		A.create(2 * count, 4, CV_64F);

		count = 0;
		for (unsigned int i = 0; i < status2D.size(); i++)
		{
			if (status2D[i][frame] > 0)
			{
				status = status2D[i][frame] < status ? status2D[i][frame] : status;

				double x, y;
				cv::Point2d pt_trans = Project::getInstance()->getCameras()[i]->undistortPoint(points2D[i][frame], true);
				x = pt_trans.x;
				y = pt_trans.y;

				cv::Mat projMatrs = Project::getInstance()->getCameras()[i]->getProjectionMatrix(trial->getReferenceCalibrationImage());

				for (int k = 0; k < 4; k++)
				{
					A.at<double>(count * 2 + 0, k) = x * projMatrs.at<double>(2, k) - projMatrs.at<double>(0, k);
					A.at<double>(count * 2 + 1, k) = y * projMatrs.at<double>(2, k) - projMatrs.at<double>(1, k);
				}
				count++;
			}
		}

		cv::SVD::solveZ(A, f);
		double w = f.at<double>(3, 0);
		if (w != 0.0)
		{
			points3D[frame].x = f.at<double>(0, 0) / w;
			points3D[frame].y = f.at<double>(1, 0) / w;
			points3D[frame].z = f.at<double>(2, 0) / w;
			status3D[frame] = status;

			reprojectPoint(frame);
		}
	}
}


void Marker::reconstruct3DPointZissermanIncremental(int frame)
{
	int count = 0;
	for (unsigned int i = 0; i < status2D.size(); i++)
	{
		if (status2D[i][frame] > 0) count++;
	}

	if (count >= 2)
	{
		markerStatus status = MANUAL_AND_OPTIMIZED;
		cv::Mat f;
		f.create(3, 1, CV_64F);

		cv::Mat A;
		A.create(2 * count, 4, CV_64F);

		cv::Mat w;
		w.create(count, 1, CV_64F);
		w = cv::Mat::ones(count, 1, CV_64F);

		double w_old = 1;
		bool stop;
		double eps = 0.0000001;

		for (int j = 0; j < 10; j++)
		{
			count = 0;
			stop = true;
			for (unsigned int i = 0; i < status2D.size(); i++)
			{
				if (status2D[i][frame] > 0)
				{
					status = status2D[i][frame] < status ? status2D[i][frame] : status;

					double x, y;
					cv::Point2d pt_trans;

					pt_trans = Project::getInstance()->getCameras()[i]->undistortPoint(points2D[i][frame], true);

					x = pt_trans.x;
					y = pt_trans.y;
					cv::Mat projMatrs = Project::getInstance()->getCameras()[i]->getProjectionMatrix(trial->getReferenceCalibrationImage());

					if (j != 0)
					{
						w_old = w.at<double>(count, 0);
						w.at<double>(count, 0) = f.at<double>(0, 0) * projMatrs.at<double>(2, 0) + f.at<double>(1, 0) * projMatrs.at<double>(2, 1) + f.at<double>(2, 0) * projMatrs.at<double>(2, 2) + f.at<double>(3, 0) * projMatrs.at<double>(2, 3);
						stop = (fabs(w.at<double>(count, 0) - w_old) < eps) ? stop : false;
						//std::cerr << fabs(w.at<double>(count, 0) - w_old) << " " << w.at<double>(count, 0) << " " << w_old << std::endl;
					}
					else
					{
						stop = false;
					}

					for (int k = 0; k < 4; k++)
					{
						A.at<double>(count * 2 + 0, k) = (x * projMatrs.at<double>(2, k) - projMatrs.at<double>(0, k)) / w.at<double>(count, 0);
						A.at<double>(count * 2 + 1, k) = (y * projMatrs.at<double>(2, k) - projMatrs.at<double>(1, k)) / w.at<double>(count, 0);
					}
					count++;
				}
			}

			if (stop)
			{
				break;
			}

			cv::SVD::solveZ(A, f);
			//std::cerr << j << " " << f << std::endl;
		}

		if (f.at<double>(3, 0) != 0.0)
		{
			points3D[frame].x = f.at<double>(0, 0) / f.at<double>(3, 0);
			points3D[frame].y = f.at<double>(1, 0) / f.at<double>(3, 0);
			points3D[frame].z = f.at<double>(2, 0) / f.at<double>(3, 0);
			status3D[frame] = status;

			reprojectPoint(frame);
		}
	}
}


void Marker::reconstruct3DPointZissermanMatlab(int frame)
{
	int count = 0;
	for (unsigned int i = 0; i < status2D.size(); i++)
	{
		if (status2D[i][frame] > 0) count++;
	}

	if (count >= 2)
	{
		markerStatus status = MANUAL_AND_OPTIMIZED;
		cv::Mat f;
		f.create(3, 1, CV_64F);

		cv::Mat A;
		A.create(2 * count, 3, CV_64F);
		cv::Mat B;
		B.create(2 * count, 1, CV_64F);

		count = 0;
		for (unsigned int i = 0; i < status2D.size(); i++)
		{
			if (status2D[i][frame] > 0)
			{
				status = status2D[i][frame] < status ? status2D[i][frame] : status;

				double x, y;
				cv::Point2d pt_trans;
				pt_trans = Project::getInstance()->getCameras()[i]->undistortPoint(points2D[i][frame], true);

				x = pt_trans.x;
				y = pt_trans.y;
				cv::Mat projMatrs = Project::getInstance()->getCameras()[i]->getProjectionMatrix(trial->getReferenceCalibrationImage());

				projMatrs /= projMatrs.at<double>(2, 3);

				for (int k = 0; k < 3; k++)
				{
					A.at<double>(count * 2, k) = x * projMatrs.at<double>(2, k) - projMatrs.at<double>(0, k);
					A.at<double>(count * 2 + 1, k) = y * projMatrs.at<double>(2, k) - projMatrs.at<double>(1, k);
				}
				B.at<double>(count * 2, 0) = projMatrs.at<double>(0, 3) - x;
				B.at<double>(count * 2 + 1, 0) = projMatrs.at<double>(1, 3) - y;

				count++;
			}
		}

		cv::solve(A, B, f, cv::DECOMP_SVD);

		points3D[frame].x = f.at<double>(0, 0);
		points3D[frame].y = f.at<double>(1, 0);
		points3D[frame].z = f.at<double>(2, 0);
		status3D[frame] = status;

		reprojectPoint(frame);
	}
}

void Marker::reconstruct3DPointZissermanIncrementalMatlab(int frame)
{
	int count = 0;
	for (unsigned int i = 0; i < status2D.size(); i++)
	{
		if (status2D[i][frame] > 0) count++;
	}

	if (count >= 2)
	{
		markerStatus status = MANUAL_AND_OPTIMIZED;
		cv::Mat f;
		f.create(3, 1, CV_64F);

		cv::Mat A;
		A.create(2 * count, 3, CV_64F);
		cv::Mat B;
		B.create(2 * count, 1, CV_64F);
		cv::Mat w;
		w.create(count, 1, CV_64F);
		w = cv::Mat::ones(count, 1, CV_64F);
		double w_old = 1;
		bool stop;
		double eps = 0.0000001;
		for (int j = 0; j < 10; j++)
		{
			count = 0;
			stop = true;
			for (unsigned int i = 0; i < status2D.size(); i++)
			{
				if (status2D[i][frame] > 0)
				{
					status = status2D[i][frame] < status ? status2D[i][frame] : status;

					double x, y;
					cv::Point2d pt_trans;

					pt_trans = Project::getInstance()->getCameras()[i]->undistortPoint(points2D[i][frame], true);

					x = pt_trans.x;
					y = pt_trans.y;
					cv::Mat projMatrs = Project::getInstance()->getCameras()[i]->getProjectionMatrix(trial->getReferenceCalibrationImage());
					projMatrs /= projMatrs.at<double>(2, 3);

					if (j != 0)
					{
						w_old = w.at<double>(count, 0);
						w.at<double>(count, 0) = f.at<double>(0, 0) * projMatrs.at<double>(2, 0) + f.at<double>(1, 0) * projMatrs.at<double>(2, 1) + f.at<double>(2, 0) * projMatrs.at<double>(2, 2) + projMatrs.at<double>(2, 3);
						stop = (fabs(w.at<double>(count, 0) - w_old) < eps) ? stop : false;
						//std::cerr << fabs(w.at<double>(count, 0) - w_old) << " " << w.at<double>(count, 0) << " " << w_old << std::endl;
					}
					else
					{
						stop = false;
					}

					for (int k = 0; k < 3; k++)
					{
						A.at<double>(count * 2, k) = (x * projMatrs.at<double>(2, k) - projMatrs.at<double>(0, k)) / w.at<double>(count, 0);
						A.at<double>(count * 2 + 1, k) = (y * projMatrs.at<double>(2, k) - projMatrs.at<double>(1, k)) / w.at<double>(count, 0);
					}
					B.at<double>(count * 2, 0) = (projMatrs.at<double>(0, 3) - x) / w.at<double>(count, 0);
					B.at<double>(count * 2 + 1, 0) = (projMatrs.at<double>(1, 3) - y) / w.at<double>(count, 0);

					count++;
				}
			}

			if (stop)
			{
				break;
			}

			cv::solve(A, B, f, cv::DECOMP_SVD);
			//std::cerr << j << " " << f << std::endl;
		}

		points3D[frame].x = f.at<double>(0, 0);
		points3D[frame].y = f.at<double>(1, 0);
		points3D[frame].z = f.at<double>(2, 0);
		status3D[frame] = status;

		reprojectPoint(frame);
	}
}

void Marker::reconstruct3DPointRayIntersection(int frame)
{
	int count = 0;
	for (unsigned int i = 0; i < status2D.size(); i++)
	{
		if (status2D[i][frame] > 0) count++;
	}

	if (count >= 2)
	{
		markerStatus status = MANUAL_AND_OPTIMIZED;
		cv::Mat f;
		f.create(3, 1, CV_64F);

		cv::Mat A;
		A.create(3, 3, CV_64F);
		A = cv::Mat::zeros(3, 3, CV_64F);

		cv::Mat B;
		B.create(3, 1, CV_64F);
		B = cv::Mat::zeros(3, 1, CV_64F);

		cv::Mat P_inv;
		P_inv.create(4, 4, CV_64F);

		cv::Mat tmp1 = (cv::Mat_<double>(1, 4) << 0,0,0,1);
		cv::Mat tmp2 = (cv::Mat_<double>(4, 1) << 0,0,0,1);
		cv::Mat tmp3;
		cv::Mat origin;
		origin.create(1, 4, CV_64F);
		cv::Mat dir;
		dir.create(1, 4, CV_64F);

		count = 0;
		for (unsigned int i = 0; i < status2D.size(); i++)
		{
			if (status2D[i][frame] > 0)
			{
				status = status2D[i][frame] < status ? status2D[i][frame] : status;

				double x, y;
				cv::Point2d pt_trans;
				pt_trans = Project::getInstance()->getCameras()[i]->undistortPoint(points2D[i][frame], true);

				x = pt_trans.x;
				y = pt_trans.y;

				cv::vconcat(Project::getInstance()->getCameras()[i]->getProjectionMatrix(trial->getReferenceCalibrationImage()), tmp1, P_inv);
				cv::invert(P_inv, P_inv);
				tmp3 = (cv::Mat_<double>(4, 1) << x,y,1,1);

				origin = P_inv * tmp2;
				dir = P_inv * tmp3 - origin;
				dir = dir / cv::norm(dir);

				double t11 = 1 - dir.at<double>(0, 0) * dir.at<double>(0, 0);
				double t22 = 1 - dir.at<double>(0, 1) * dir.at<double>(0, 1);
				double t33 = 1 - dir.at<double>(0, 2) * dir.at<double>(0, 2);
				double t12 = - dir.at<double>(0, 0) * dir.at<double>(0, 1);
				double t13 = - dir.at<double>(0, 0) * dir.at<double>(0, 2);
				double t23 = - dir.at<double>(0, 1) * dir.at<double>(0, 2);

				A.at<double>(0, 0) += t11;
				A.at<double>(0, 1) += t12;
				A.at<double>(0, 2) += t13;

				A.at<double>(1, 0) += t12;
				A.at<double>(1, 1) += t22;
				A.at<double>(1, 2) += t23;

				A.at<double>(2, 0) += t13;
				A.at<double>(2, 1) += t23;
				A.at<double>(2, 2) += t33;

				B.at<double>(0, 0) += t11 * origin.at<double>(0, 0) + t12 * origin.at<double>(0, 1) + t13 * origin.at<double>(0, 2);
				B.at<double>(0, 1) += t12 * origin.at<double>(0, 0) + t22 * origin.at<double>(0, 1) + t23 * origin.at<double>(0, 2);
				B.at<double>(0, 2) += t13 * origin.at<double>(0, 0) + t23 * origin.at<double>(0, 1) + t33 * origin.at<double>(0, 2);

				count++;
			}
		}
		cv::solve(A, B, f, cv::DECOMP_SVD);

		points3D[frame].x = f.at<double>(0, 0);
		points3D[frame].y = f.at<double>(1, 0);
		points3D[frame].z = f.at<double>(2, 0);
		status3D[frame] = status;

		reprojectPoint(frame);
	}
}

double Marker::getSize()
{
	return meanSize;
}

double Marker::getSizeRange()
{
	return sizeRange;
}

void Marker::setSize(int camera, int frame, double size_value)
{
	markerSize[camera][frame] = size_value;
	updateMeanSize();
}

void Marker::save(QString points_filename, QString status_filename, QString markersize_filename, QString pointsWorld_filename)
{
	if (!points_filename.isEmpty())
	{
		std::ofstream outfile(points_filename.toStdString());
		outfile.precision(12);
		for (unsigned int j = 0; j < points2D[0].size(); j++)
		{
			for (unsigned int i = 0; i < points2D.size(); i++)
			{
				outfile << points2D[i][j].x << "," << points2D[i][j].y;
				if (i != points2D.size() - 1) outfile << ",";
			}
			outfile << std::endl;
		}
		outfile.close();
	}

	if (!status_filename.isEmpty())
	{
		std::ofstream outfile_status(status_filename.toStdString());
		outfile_status.precision(12);
		for (unsigned int j = 0; j < status2D[0].size(); j++)
		{
			for (unsigned int i = 0; i < status2D.size(); i++)
			{
				outfile_status << status2D[i][j];
				if (i != status2D.size() - 1) outfile_status << ",";
			}
			outfile_status << std::endl;
		}
		outfile_status.close();
	}

	if (!markersize_filename.isEmpty())
	{
		std::ofstream outfile_size(markersize_filename.toStdString());
		outfile_size.precision(12);
		for (unsigned int j = 0; j < markerSize[0].size(); j++)
		{
			for (unsigned int i = 0; i < markerSize.size(); i++)
			{
				outfile_size << markerSize[i][j];
				if (i != markerSize.size() - 1) outfile_size << ",";
			}
			outfile_size << std::endl;
		}
		outfile_size.close();
	}

	if (!pointsWorld_filename.isEmpty())
	{
		std::ofstream outfile_world(pointsWorld_filename.toStdString());
		outfile_world.precision(12);
		for (unsigned int j = 0; j < points3D.size(); j++)
		{
			if (status3D[j] <= 0)
			{
				outfile_world << "NaN" << "," << "NaN" << "," << "NaN" << std::endl;
			}
			else
			{
				outfile_world << points3D[j].x << "," << points3D[j].y << "," << points3D[j].z << std::endl;
			}
		}
		outfile_world.close();
	}
}

void Marker::load(QString points_filename, QString status_filename, QString markersize_filename)
{
	std::ifstream fin;
	fin.open(points_filename.toStdString());
	std::istringstream in;
	std::string line;
	//read first line 
	int linecount = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0)
		{
			if ((int) points2D[0].size() <= linecount) addFrame();
			for (unsigned int i = 0; i < points2D.size(); i ++)
			{
				points2D[i][linecount].x = tmp[2 * i];
				points2D[i][linecount].y = tmp[2 * i + 1];
			}
		}
		line.clear();
		linecount++;
	}
	fin.close();

	fin.open(status_filename.toStdString());
	linecount = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<int> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0)
		{
			for (unsigned int i = 0; i < status2D.size(); i++)
			{
				status2D[i][linecount] = markerStatus(tmp[i]);
			}
		}
		line.clear();
		linecount++;
	}
	fin.close();

	fin.open(markersize_filename.toStdString());
	linecount = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0)
		{
			for (unsigned int i = 0; i < markerSize.size(); i++)
			{
				markerSize[i][linecount] = tmp[i];
			}
		}
		line.clear();
		linecount++;
	}
	fin.close();

	updateMeanSize();
}

void Marker::save3DPoints(QString points_filename, QString status_filename)
{
	if (!points_filename.isEmpty())
	{
		std::ofstream outfile_world(points_filename.toStdString());
		outfile_world.precision(12);
		for (unsigned int j = 0; j < points3D.size(); j++)
		{
			outfile_world << points3D[j].x << "," << points3D[j].y << "," << points3D[j].z << std::endl;
		}
		outfile_world.close();
	}

	if (!status_filename.isEmpty())
	{
		std::ofstream outfile_status(status_filename.toStdString());
		outfile_status.precision(12);
		for (unsigned int i = 0; i < status3D.size(); i++)
		{
			outfile_status << status3D[i] << std::endl;
		}
		outfile_status.close();
	}
}

void Marker::load3DPoints(QString points_filename, QString status_filename)
{
	std::ifstream fin;
	fin.open(points_filename.toStdString());
	std::istringstream in;
	std::string line;
	//read first line 
	int linecount = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0)
		{
			points3D[linecount].x = tmp[0];
			points3D[linecount].y = tmp[1];
			points3D[linecount].z = tmp[2];
		}
		line.clear();
		linecount++;
	}
	fin.close();

	fin.open(status_filename.toStdString());
	linecount = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<int> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0)
		{
			status3D[linecount] = markerStatus(tmp[0]);
		}
		line.clear();
		linecount++;
	}
	fin.close();
}

void Marker::resetMultipleFrames(int camera, int frameStart, int frameEnd, bool toggleUntrackable)
{
	//fprintf(stderr, "Delete %d - from %d  to %d\n", camera, frameStart, frameEnd);
	for (int i = frameStart; i <= frameEnd; i++)
	{
		if (camera == -1)
		{
			for (unsigned int cam = 0; cam < points2D.size(); cam++)
			{
				points2D[cam][i].x = -2;
				points2D[cam][i].y = -2;
				points2D[cam][i].x = -2;
				points2D_projected[cam][i].y = -2;			
				status2D[cam][i] = toggleUntrackable ? UNTRACKABLE : UNDEFINED;
				error2D[cam][i] = 0.0;
				markerSize[cam][i] = -1.0;
			}
			points3D[i].x = -1000;
			points3D[i].y = -1000;
			points3D[i].z = -1000;
			status3D[i] = UNDEFINED;
			error3D[i] = 0.0;
			reconstruct3DPoint(i);
		}
		else
		{
			points2D[camera][i].x = -2;
			points2D[camera][i].y = -2;
			points2D[camera][i].x = -2;
			points2D_projected[camera][i].y = -2;
			status2D[camera][i] = toggleUntrackable ? UNTRACKABLE : UNDEFINED;
			error2D[camera][i] = 0.0;
			markerSize[camera][i] = -1.0;

			points3D[i].x = -1000;
			points3D[i].y = -1000;
			points3D[i].z = -1000;
			status3D[i] = UNDEFINED;
			error3D[i] = 0.0;

			reconstruct3DPoint(i);
		}
	}

	updateMeanSize();
}

void Marker::setInterpolation(int frame, interpolationMethod method)
{
	interpolation[frame] = method;
}

interpolationMethod Marker::getInterpolation(int frame)
{
	return interpolation[frame];
}

bool Marker::getHasInterpolation()
{
	return hasInterpolation;
}

void Marker::updateHasInterpolation()
{
	hasInterpolation = false;
	for (std::vector<interpolationMethod>::iterator it = interpolation.begin(); it < interpolation.end(); ++it)
	{
		if (*it > NONE)
		{
			hasInterpolation = true;
			return;
		}
	}
}

void Marker::saveInterpolation(QString filename)
{
	if (!filename.isEmpty())
	{
		std::ofstream outfile(filename.toStdString());
		outfile.precision(12);		
		for (unsigned int i = 0; i < interpolation.size(); i++)
		{
			outfile << interpolation[i] << std::endl;
		}		
		outfile.close();
	}
}

void Marker::loadInterpolation(QString filename)
{
	std::ifstream fin;
	fin.open(filename.toStdString());
	std::istringstream in;
	std::string line; 
	int linecount = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<int> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0)
		{
			interpolation[linecount] = interpolationMethod(tmp[0]);
		}
		line.clear();
		linecount++;
	}
	fin.close();

	updateHasInterpolation();
}

void Marker::update(bool updateAll)
{
	if (requiresRecomputation)
	{
		for (unsigned int i = 0; i < points2D[0].size(); i++)
		{
			reconstruct3DPoint(i, updateAll);
		}
		requiresRecomputation = false;
	}
	else
	{
		for (unsigned int i = 0; i < points2D[0].size(); i++)
		{
			reprojectPoint(i);
		}
	}
}

bool Marker::isValid(int camera, int frame)
{
	//check Size Min Max
	if (markerSize[camera][frame] < 1)
	{
		reset(camera, frame);
		return false;
	}

	if (markerSize[camera][frame] > 50)
	{
		reset(camera, frame);
		return false;
	}

	if (fabs(markerSize[camera][frame] - meanSize) > 3 * sizeRange)
	{
		reset(camera, frame);
		return false;
	}

	if (points2D[camera][frame].x < 0 || points2D[camera][frame].x >= Project::getInstance()->getCameras()[camera]->getWidth())
	{
		reset(camera, frame);
		return false;
	}
	if (points2D[camera][frame].y < 0 || points2D[camera][frame].y >= Project::getInstance()->getCameras()[camera]->getHeight())
	{
		reset(camera, frame);
		return false;
	}
	if (error2D[camera][frame] > Settings::getInstance()->getFloatSetting("MaximumReprojectionError"))
	{
		for (unsigned int c = 0; c < Project::getInstance()->getCameras().size(); c++) {
			if (status2D[c][frame] < SET)
				reset(c, frame);
		}
		return false;
	}
	return true;
}

void Marker::reset(int camera, int frame)
{
	status2D[camera][frame] = UNDEFINED;
	points2D[camera][frame].x = -2;
	points2D[camera][frame].y = -2;
	points2D_projected[camera][frame].x = -2;
	points2D_projected[camera][frame].y = -2;
	markerSize[camera][frame] = -1;
	error2D[camera][frame] = 0.0;

	reconstruct3DPoint(frame);
	updateMeanSize();
}

int Marker::getSizeOverride()
{
	return sizeOverride;
}

void Marker::setSizeOverride(int value)
{
	sizeOverride = value;
}

int Marker::getThresholdOffset()
{
	return thresholdOffset;
}

void Marker::setThresholdOffset(int value)
{
	thresholdOffset = value;
}

int Marker::getMaxPenalty()
{
	return maxPenalty;
}

void Marker::setMaxPenalty(int value)
{
	maxPenalty = value;
	if (maxPenalty == 125) {// adjustement of erronous default values
		maxPenalty = 50; 
		return;
	}
	if (maxPenalty > 100) maxPenalty = 100;
}

int Marker::getMethod()
{
	return method;
}

void Marker::setMethod(int value)
{
	method = value;
}

Trial* Marker::getTrial()
{
	return trial;
}

bool Marker::getRequiresRecomputation()
{
	return requiresRecomputation;
}

void Marker::setRequiresRecomputation(bool value)
{
	requiresRecomputation = value;
}

bool Marker::filterMarker(double cutoffFrequency, const std::vector <cv::Point3d> &marker_in, const std::vector <markerStatus>& status_in
	, std::vector <cv::Point3d> &marker_out, std::vector <markerStatus>& status_out)
{
	for (unsigned int i = 0; i < status_in.size(); i++)
	{
		cv::Point3d p3(-1000, -1000, -1000);
		marker_out.push_back(p3);
		status_out.push_back(UNDEFINED);
	}

	if (cutoffFrequency > 0 && trial->getRecordingSpeed() > 0 &&
		0 < (cutoffFrequency / (trial->getRecordingSpeed() * 0.5)) &&
		(cutoffFrequency / (trial->getRecordingSpeed() * 0.5)) < 1)
	{
		std::vector<int> idx;
		for (int i = 0; i < trial->getNbImages(); i++)
		{
			if (status_in[i] > 0)
			{
				idx.push_back(i);
			}
			else
			{
				if (idx.size() >= 1)
				{
					filterData(idx, cutoffFrequency, marker_in, status_in, marker_out, status_out);
				}
				idx.clear();
			}

			if (i == trial->getNbImages() - 1)
			{
				if (idx.size() >= 1)
				{
					filterData(idx, cutoffFrequency, marker_in, status_in, marker_out, status_out);
				}
				idx.clear();
			}
		}

		return true;
	} 
	else
	{
		return false;
	}
}

markerStatus Marker::updateStatus12(int statusOld)
{
	switch (statusOld)
	{
		case -2:
			return DELETED;
		case -1:
			return LOST;
		case 0:
			return UNDEFINED;
		case 1:
			return PREDICTED;
		case 2:
			return PREDICTED;
		case 3:
			return PREDICTED;
		case 4:
			return TRACKED;
		case 5:
			return SET;
		case 6:
			return MANUAL;
		default:
			return UNDEFINED;
	}
}

void Marker::updateToProject12()
{
	for (unsigned int i = 0; i < status3D.size(); i++)
	{
		status3D[i] = updateStatus12(status3D[i]);
		for (unsigned int c = 0; c < status2D.size(); c++)
			status2D[c][i] = updateStatus12(status2D[c][i]);
	}

	if (method == 4){
		method = 0;
	}
	else if (method == 5)
	{
		method = 2;
	}
	else if (method == 6) {
		method = 4;
	}
}

void Marker::updateToProject13()
{
	maxPenalty = maxPenalty / 125 * 50;
}

void Marker::interpolate()
{
	//reset all interpolated
	for (unsigned int c = 0; c < status2D.size(); c++){
		for (unsigned int i = 0; i < status2D[c].size(); i++){
			if (status2D[c][i] <= INTERPOLATED)
			{
				setPoint(c, i, -2, -2, UNDEFINED);
			}
		}
	}

	bool requiresRepeat = false;
	bool requiresLinear = false;
	bool requiresCubic = false;

	for (unsigned int f = 1; f < interpolation.size(); f++){
		if (interpolation[f] == REPEAT){
			requiresRepeat = true;
		}
		else if (interpolation[f] == LINEAR)
		{
			requiresLinear = true;
		}
		else if(interpolation[f] == CUBIC)
		{
			requiresCubic = true;
		}
	}

	// first interpolate repeat
	if (requiresRepeat){
		for (unsigned int f = 1; f < interpolation.size(); f++){
			if (interpolation[f] == REPEAT)
			{
				if (!trial->getInterpolate3D()){
					//check if last was set
					for (unsigned int c = 0; c < status2D.size(); c++){
						if (status2D[c][f - 1] >= INTERPOLATED && status2D[c][f] < INTERPOLATED)
						{
							setPoint(c, f, points2D[c][f - 1].x, points2D[c][f - 1].y, INTERPOLATED);
						}
					}
				}
				else
				{
					if (status3D[f - 1] >= INTERPOLATED)
					{
						for (unsigned int c = 0; c < status2D.size(); c++){
							if (status2D[c][f] < INTERPOLATED){
								cv::Point2d pt2d = Project::getInstance()->getCameras()[c]->projectPoint(points3D[f - 1], trial->getReferenceCalibrationImage());
								setPoint(c, f, pt2d.x, pt2d.y, INTERPOLATED);
							}
						}
					}
				}
			}
		}
	}

	//then interpolate linear
	if (requiresLinear){
		if (!trial->getInterpolate3D()){
			//do each camera seperately
			for (unsigned int c = 0; c < status2D.size(); c++){
				for (unsigned int f = 1; f < interpolation.size(); f++){
					//std::cerr << "set " << f << " " << interpolation[f] << " " << status2D[c][f - 1] << std::endl;;

					if (status2D[c][f] < INTERPOLATED && status2D[c][f - 1] >= INTERPOLATED)
					{
						int start = f - 1;
						int end = f;
						//continue until either interpolation is not linear anymore or the value is set
						while (end < interpolation.size() && status2D[c][end] < INTERPOLATED)
						{
							end++;
						}
						//start and end are set and we have to check that end is defined
						if (end < interpolation.size() && status2D[c][end] >= INTERPOLATED)
						{
							while (f < end){
								if (interpolation[f] == LINEAR){
									double p = (((double)(f - start)) / (end - start));
									double x = points2D[c][start].x + p *(points2D[c][end].x - points2D[c][start].x);
									double y = points2D[c][start].y + p *(points2D[c][end].y - points2D[c][start].y);
									setPoint(c, f, x, y, INTERPOLATED);
								}
								f++;
							}
						}
					}
				}
			}
		}
		else
		{
			for (unsigned int f = 1; f < interpolation.size(); f++){
				if (status3D[f] < INTERPOLATED && status3D[f - 1] >= INTERPOLATED)
				{
					int start = f - 1;
					int end = f;
					//continue until either interpolation is not linear anymore or the value is set
					while (end < interpolation.size() && status3D[end] < INTERPOLATED)
					{
						end++;
					}
					//start and end are set and we have to check that end is defined
					if (end < interpolation.size() && status3D[end] >= INTERPOLATED)
					{
						while (f < end){
							if (interpolation[f] == LINEAR){
								cv::Point3d pt3d;
								double p = (((double)(f - start)) / (end - start));
								pt3d.x = points3D[start].x + p *(points3D[end].x - points3D[start].x);
								pt3d.y = points3D[start].y + p *(points3D[end].y - points3D[start].y);
								pt3d.z = points3D[start].z + p *(points3D[end].z - points3D[start].z);
								for (unsigned int c = 0; c < status2D.size(); c++){
									if (status2D[c][f] < INTERPOLATED){
										cv::Point2d pt2d = Project::getInstance()->getCameras()[c]->projectPoint(pt3d, trial->getReferenceCalibrationImage());
										setPoint(c, f, pt2d.x, pt2d.y, INTERPOLATED);
									}
								}
							}
							f++;
						}
					}
				}
			}
		}
	}

	//// finally do cubic spline
	if (requiresCubic)
	{
		if (!trial->getInterpolate3D())
		{
			for (unsigned int c = 0; c < status2D.size(); c++){
				std::vector<double> YX1;
				std::vector<double> YY1;
				std::vector<double> X1;

				std::vector<double> YX2;
				std::vector<double> YY2;
				std::vector<double> X2;

				for (unsigned int f = 0; f < status2D[c].size(); f++){
					if (status2D[c][f] >= INTERPOLATED)
					{
						YX1.push_back(points2D[c][f].x);
						YY1.push_back(points2D[c][f].y);
						X1.push_back(f);
					} 
				}

				if (X1.size() < 4)
					return;

				for (unsigned int f = X1[0]; f < X1[X1.size() - 1]; f++){
					if (interpolation[f] == CUBIC && status2D[c][f - 1] >= INTERPOLATED)
					{
						int start = f - 1;
						int end = f;
						//continue until either interpolation is not linear anymore or the value is set
						while (end < interpolation.size() && interpolation[end] == CUBIC && status2D[c][end] < INTERPOLATED)
						{
							end++;
						}
						//start and end are set and we have to check that end is defined
						if (end < interpolation.size() && status2D[c][end] >= INTERPOLATED)
						{
							while (f < end){
								X2.push_back(f);
								f++;
							}
						}
					}
				}

				if (X2.size() == 0) 
					return;
				
				if(!Maths::Interpolation::ispline(X1,YX1,X2,YX2))
					return;
				if (!Maths::Interpolation::ispline(X1,YY1,X2,YY2))
					return;

				for (unsigned int k = 0; k < X2.size(); k++){
					setPoint(c, X2[k], YX2[k], YY2[k], INTERPOLATED);
				}
			}
		}
		else
		{

			std::vector<double> YX1;
			std::vector<double> YY1;
			std::vector<double> YZ1;
			std::vector<double> X1;

			std::vector<double> YX2;
			std::vector<double> YY2;
			std::vector<double> YZ2;
			std::vector<double> X2;

			for (unsigned int f = 0; f < status3D.size(); f++){
				if (status3D[f] >= INTERPOLATED)
				{
					YX1.push_back(points3D[f].x);
					YY1.push_back(points3D[f].y);
					YZ1.push_back(points3D[f].z);
					X1.push_back(f);
				}
			}

			if (X1.size() < 4)
				return;

			for (unsigned int f = X1[0]; f < X1[X1.size() - 1]; f++){
				if (interpolation[f] == CUBIC && status3D[f - 1] >= INTERPOLATED)
				{
					int start = f - 1;
					int end = f;
					//continue until either interpolation is not linear anymore or the value is set
					while (end < interpolation.size() && interpolation[end] == CUBIC && status3D[end] < INTERPOLATED)
					{
						end++;
					}
					//start and end are set and we have to check that end is defined
					if (end < interpolation.size() && status3D[end] >= INTERPOLATED)
					{
						while (f < end){
							X2.push_back(f);
							f++;
						}
					}
				}
			}

			if (X2.size() == 0)
				return;

			if (!Maths::Interpolation::ispline(X1, YX1, X2, YX2))
				return;
			if (!Maths::Interpolation::ispline(X1, YY1, X2, YY2))
				return;
			if (!Maths::Interpolation::ispline(X1, YZ1, X2, YZ2))
				return;

			for (unsigned int k = 0; k < X2.size(); k++){
				cv::Point3d pt3d;
				pt3d.x = YX2[k];
				pt3d.y = YY2[k];
				pt3d.z = YZ2[k];
				for (unsigned int c = 0; c < status2D.size(); c++){
					if (status2D[c][X2[k]] < INTERPOLATED)
					{
						cv::Point2d pt2d = Project::getInstance()->getCameras()[c]->projectPoint(pt3d, trial->getReferenceCalibrationImage());
						setPoint(c, X2[k], pt2d.x, pt2d.y, INTERPOLATED);
					}
				}
			}
		}
	}
}


QColor Marker::getStatusColor(int camera, int frame)
{
	switch (status2D[camera][frame])
	{
		case UNTRACKABLE:
			return QColor(Settings::getInstance()->getQStringSetting("ColorUntrackable"));
		case DELETED:
			return QColor(Settings::getInstance()->getQStringSetting("ColorUndefined"));
		case LOST:
			return QColor(Settings::getInstance()->getQStringSetting("ColorUndefined"));
		case UNDEFINED:
			return QColor(Settings::getInstance()->getQStringSetting("ColorUndefined"));
		case PREDICTED:
			return QColor(Settings::getInstance()->getQStringSetting("ColorUndefined"));
		case INTERPOLATED:
			return QColor(Settings::getInstance()->getQStringSetting("ColorInterpolated"));
		case TRACKED:
			return QColor(Settings::getInstance()->getQStringSetting("ColorTracked"));
		case TRACKED_AND_OPTIMIZED:
			return QColor(Settings::getInstance()->getQStringSetting("ColorTrackedAndOpt"));
		case SET:
			return QColor(Settings::getInstance()->getQStringSetting("ColorSet"));
		case SET_AND_OPTIMIZED:
			return QColor(Settings::getInstance()->getQStringSetting("ColorSetAndOpt"));
		case  MANUAL:
			return QColor(Settings::getInstance()->getQStringSetting("ColorManual"));
		case MANUAL_AND_OPTIMIZED:
			return QColor(Settings::getInstance()->getQStringSetting("ColorManualAndOpt"));
	}
	return QColor(Settings::getInstance()->getQStringSetting("ColorUndefined"));
}

void Marker::filterData(std::vector<int> idx, double cutoffFrequency, const std::vector<cv::Point3d>& marker_in, const std::vector<markerStatus>& status_in, std::vector<cv::Point3d>& marker_out, std::vector<markerStatus>& status_out)
{
	if (idx.size() <= 12)
	{
		for (unsigned int i = 0; i < idx.size(); i++)
		{
			marker_out[idx[i]].x = marker_in[idx[i]].x;
			marker_out[idx[i]].y = marker_in[idx[i]].y;
			marker_out[idx[i]].z = marker_in[idx[i]].z;

			status_out[idx[i]]= status_in[idx[i]];
		}
	}
	else
	{
		std::vector<double> x;
		std::vector<double> y;
		std::vector<double> z;

		for (unsigned int i = 0; i < idx.size(); i++)
		{
			x.push_back(marker_in[idx[i]].x);
			y.push_back(marker_in[idx[i]].y);
			z.push_back(marker_in[idx[i]].z);
		}

		std::vector<double> x_out;
		std::vector<double> y_out;
		std::vector<double> z_out;

		ButterworthLowPassFilter* filter = new ButterworthLowPassFilter(4, cutoffFrequency, trial->getRecordingSpeed());

		filter->filter(x, x_out);
		filter->filter(y, y_out);
		filter->filter(z, z_out);

		for (unsigned int i = 0; i < idx.size(); i++)
		{
			marker_out[idx[i]].x = x_out[i];
			marker_out[idx[i]].y = y_out[i];
			marker_out[idx[i]].z = z_out[i];

			status_out[idx[i]] = status_in[idx[i]];
		}

		delete filter;

		x_out.clear();
		y_out.clear();
		z_out.clear();
		x.clear();
		y.clear();
		z.clear();
	}
}



void Marker::reprojectPoint(int frame)
{
	if (Project::getInstance()->getCalibration() == NO_CALIBRATION)
	{
		for (unsigned int i = 0; i < points2D.size(); i++)
		{
			points2D_projected[i][frame] = points2D[i][frame];
		}
	}
	else{
		for (unsigned int i = 0; i < points2D.size(); i++)
		{
			points2D_projected[i][frame] = Project::getInstance()->getCameras()[i]->projectPoint(points3D[frame], trial->getReferenceCalibrationImage());
		}
	}

	updateError(frame);
}

int Marker::getFirstTrackedFrame(int cam)
{
	if (cam == -1){
		for (int i = 0; i < status2D[0].size(); i++)
		{
			for (int c = 0; c < status2D.size(); c++)
			{
				if (status2D[c][i] >= TRACKED) return i + 1;
			}
		}
	}else
	{
		for (int i = 0; i < status2D[0].size(); i++)
		{		
			if (status2D[cam][i] >= TRACKED) return i + 1;
		}
	}
	return -1;
}

int Marker::getLastTrackedFrame(int cam)
{
	if (cam == -1){
		for (int i = status2D[0].size() - 1; i >= 0; i--)
		{
			for (int c = 0; c < status2D.size(); c++)
			{
				if (status2D[c][i] >= TRACKED) return i + 1;
			}
		}
	}
	else
	{
		for (int i = status2D[0].size() - 1; i >= 0; i--)
		{		
			if (status2D[cam][i] >= TRACKED) return i + 1;
		}
	}
	return -1;
}

int Marker::getFramesTracked(int cam)
{
	int count = 0;
	bool tracked;
	if (cam == -1){
		for (int i = 0; i < status3D.size(); i++)
		{		
			if (status3D[i] >= TRACKED)count++;
		}
	}
	else
	{
		for (int i = 0; i < status2D[0].size(); i++)
		{
			if (status2D[cam][i] >= TRACKED)  count++;
		}
	}

	return count;
}

double Marker::getReprojectionError(double * sd, int start, int end)
{
	if (start == -1) start = 0;
	if (end == -1) end = status3D.size();

	int count = 0;
	double error = 0;
	for (int i = start; i < end; i++)
	{
		if (status3D[i] >= TRACKED) {
			for (int c = 0; c < status2D.size(); c++)
			{
				count++;
				error += error2D[c][i];
			}
		}
	}
	if (count != 0) error /= count;

	if (sd != nullptr)
	{
		double sd_tmp = 0;
		count = 0;
		for (int i = start; i < end; i++)
		{
			if (status3D[i] >= TRACKED) {
				for (int c = 0; c < status2D.size(); c++)
				{
					count++;
					sd_tmp += pow(error2D[c][i] - error, 2);
				}
			}
		}
		*sd = (count <= 1) ? 0 : sqrt(sd_tmp / (count - 1));
	}

	return error;
}

void Marker::updateError(int frame)
{
	if (status3D[frame] > 0)
	{
		for (unsigned int i = 0; i < points2D.size(); i++)
		{
			if (status2D[i][frame] > 0)
			{
				error2D[i][frame] = sqrt((points2D[i][frame].x - points2D_projected[i][frame].x) * (points2D[i][frame].x - points2D_projected[i][frame].x) +
					(points2D[i][frame].y - points2D_projected[i][frame].y) * (points2D[i][frame].y - points2D_projected[i][frame].y));
			}
		}
	}
}

void Marker::updateMeanSize()
{
	double mean = 0;
	int count = 0;
	for (unsigned int i = 0; i < markerSize.size(); i++)
	{
		for (unsigned int j = 0; j < markerSize[i].size(); j++)
		{
			if (markerSize[i][j] > 1 && markerSize[i][j] < 50)
			{
				mean += markerSize[i][j];
				count++;
			}
		}
	}
	if (count > 0) meanSize = mean / count;

	sizeRange = 0;
	for (unsigned int i = 0; i < markerSize.size(); i++)
	{
		for (unsigned int j = 0; j < markerSize[i].size(); j++)
		{
			if (fabs(markerSize[i][j] - mean) > sizeRange)
			{
				sizeRange = fabs(markerSize[i][j] - mean);
			}
		}
	}
}

std::vector<cv::Point2d> Marker::getEpipolarLine(int cameraOrigin, int CameraDestination, int frame)
{
	std::vector<cv::Point2d> epiline;

	if (Project::getInstance()->getCalibration() == NO_CALIBRATION)
		return epiline;

	cv::Mat proj;
	cv::Mat proj1;
	cv::Mat proj2;

	cv::Mat tmp;
	tmp.create(1, 4, CV_64F);
	tmp.at<double>(0, 0) = 0;
	tmp.at<double>(0, 1) = 0;
	tmp.at<double>(0, 2) = 0;
	tmp.at<double>(0, 3) = 1.0;

	cv::vconcat(Project::getInstance()->getCameras()[cameraOrigin]->getProjectionMatrix(trial->getReferenceCalibrationImage()), tmp, proj1);
	cv::vconcat(Project::getInstance()->getCameras()[CameraDestination]->getProjectionMatrix(trial->getReferenceCalibrationImage()), tmp, proj2);

	proj = proj2 * proj1.inv();

	cv::Point2d pt_origin(points2D[cameraOrigin][frame].x, points2D[cameraOrigin][frame].y);
	cv::Point2d pt_origin_trans;

	pt_origin_trans = Project::getInstance()->getCameras()[cameraOrigin]->undistortPoint(pt_origin, true);

	cv::Mat in;
	in.create(4, 1, CV_64F);
	in.at<double>(0, 0) = pt_origin_trans.x;
	in.at<double>(1, 0) = pt_origin_trans.y;
	in.at<double>(2, 0) = 1;
	in.at<double>(3, 0) = 1;

	cv::Mat out = proj * in;

	cv::Point2d p1(out.at<double>(0, 0) / out.at<double>(2, 0),
	               out.at<double>(1, 0) / out.at<double>(2, 0));

	in.at<double>(0, 0) = 100 * pt_origin_trans.x;
	in.at<double>(1, 0) = 100 * pt_origin_trans.y;
	in.at<double>(2, 0) = 100;
	out = proj * in;

	cv::Point2d p2(out.at<double>(0, 0) / out.at<double>(2, 0),
	               out.at<double>(1, 0) / out.at<double>(2, 0));

	double denom = (p2.x - p1.x);
	double m = (denom != 0.0) ? (p2.y - p1.y) / denom : 0;
	double y0 = p2.y - p2.x * m;
	cv::Point2d line_pt(m, y0);

	if (!Project::getInstance()->getCameras()[CameraDestination]->hasUndistortion())
	{
		cv::Point2d start(0, line_pt.y);
		double p = Project::getInstance()->getCameras()[CameraDestination]->getWidth();
		cv::Point2d end(p, line_pt.x * p + line_pt.y);
		epiline.push_back(start);
		epiline.push_back(end);
	}
	else
	{
		//compute start and end

		double height = ((double)Project::getInstance()->getCameras()[CameraDestination]->getWidth());
		cv::Point2d pt_prev(-10000, -10000);
		double dist = Settings::getInstance()->getIntSetting("EpipolarLinePrecision");
		dist *= dist;

		for (double p = 0.0; p <= ((double)Project::getInstance()->getCameras()[CameraDestination]->getWidth()); p += 0.1)
		{
			cv::Point2d pt(p, line_pt.x * p + line_pt.y);
			if (pt.y > 0 && pt.y < height && ((pt_prev.x - pt.x) * (pt_prev.x - pt.x) + (pt_prev.y - pt.y) * (pt_prev.y - pt.y)) > dist)
			{
				cv::Point2d pt_trans = Project::getInstance()->getCameras()[CameraDestination]->undistortPoint(pt, false);
				if (pt_trans.x != pt.x && pt_trans.y != pt.y)
				{
					epiline.push_back(pt_trans);
					pt_prev.x = pt.x;
					pt_prev.y = pt.y;
				}
			}
		}

		//set Last Point
		for (double p = ((double)Project::getInstance()->getCameras()[CameraDestination]->getWidth()); p >= 0.0; p -= 0.1)
		{
			cv::Point2d pt(p, line_pt.x * p + line_pt.y);
			if (pt.y > 0 && pt.y < height)
			{
				cv::Point2d pt_trans = Project::getInstance()->getCameras()[CameraDestination]->undistortPoint(pt, false);
				if (pt_trans.x != pt.x && pt_trans.y != pt.y)
				{
					epiline.push_back(pt_trans);
					break;
				}
			}
		}
	}

	return epiline;
}

void Marker::clear()
{
	for (std::vector<std::vector<cv::Point2d> >::iterator pt_it = points2D.begin(); pt_it != points2D.end(); ++pt_it)
	{
		pt_it->clear();
	}

	for (std::vector<std::vector<cv::Point2d> >::iterator pt_it = points2D_projected.begin(); pt_it != points2D_projected.end(); ++pt_it)
	{
		pt_it->clear();
	}

	for (std::vector<std::vector<markerStatus> >::iterator status_it = status2D.begin(); status_it != status2D.end(); ++status_it)
	{
		status_it->clear();
	}
	for (std::vector<std::vector<double> >::iterator error_it = error2D.begin(); error_it != error2D.end(); ++error_it)
	{
		error_it->clear();
	}

	for (std::vector<std::vector<double> >::iterator size_it = markerSize.begin(); size_it != markerSize.end(); ++size_it)
	{
		size_it->clear();
	}

	points2D.clear();
	points2D_projected.clear();
	status2D.clear();
	error2D.clear();
	markerSize.clear();
	interpolation.clear();
	points3D.clear();
	status3D.clear();
	error3D.clear();
}

void Marker::init(int nbCameras, int size)
{
	clear();

	for (int c = 0; c < nbCameras; c++)
	{
		std::vector<cv::Point2d> c_points2D(size, cv::Point2d(-2, -2));
		std::vector<cv::Point2d> c_points2D_projected(size, cv::Point2d(-2, -2));
		std::vector<markerStatus> c_status2D(size, UNDEFINED);
		std::vector<double> c_error2D(size, 0.0);
		std::vector<double> c_size(size, -1.0);

		points2D.push_back(c_points2D);
		points2D_projected.push_back(c_points2D_projected);
		status2D.push_back(c_status2D);
		error2D.push_back(c_error2D);
		markerSize.push_back(c_size);
	}

	points3D.resize(size, cv::Point3d(-1000, -1000, -1000));
	status3D.resize(size, UNDEFINED);
	error3D.resize(size, 0.0);
	interpolation.resize(size, NONE);
}

void Marker::addFrame()
{
	for (unsigned int c = 0; c < points2D.size(); c++)
	{
		points2D[c].push_back(cv::Point2d(-2, -2));
		points2D_projected[c].push_back(cv::Point2d(-2, -2));
		status2D[c].push_back(UNDEFINED);
		error2D[c].push_back(0.0);
		markerSize[c].push_back(-1.0);
	}

	points3D.push_back(cv::Point3d(-1000, -1000, -1000));
	status3D.push_back(UNDEFINED);
	error3D.push_back(0);
	interpolation.push_back(NONE);
}

