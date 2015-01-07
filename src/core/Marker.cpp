#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Marker.h"
#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/CalibrationImage.h"
#include "core/UndistortionObject.h"

using namespace xma;

Marker::Marker(int nbCameras, int size, Trial* _trial){
	init (nbCameras, size);
	trial = _trial;
}

Marker::~Marker(){
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

const std::vector<markerStatus>& Marker::getStatus3D()
{
	return status3D;
}

void Marker::movePoint(int camera, int activeFrame, double x, double y)
{
	points2D[camera][activeFrame].x = x;
	points2D[camera][activeFrame].y = y;
	status2D[camera][activeFrame] = SET;
	 
	reconstruct3DPoint(activeFrame);
}

void Marker::reconstruct3DPoint(int frame)
{
	int count = 0;
	for (int i = 0; i < status2D.size(); i++)
	{
		if (status2D[i][frame] > 0) count++;
	}

	if (count >= 2){
		markerStatus status = MANUAL_REFINED;
		cv::Mat f;
		f.create(4, 1, CV_64F);

		cv::Mat A;
		A.create(3 * count, 4, CV_64F);

		count = 0;
		for (int i = 0; i < status2D.size(); i++)
		{
			if (status2D[i][frame] > 0){
				
				status = status2D[i][frame] < status ? status2D[i][frame] : status;

				double x, y;
				cv::Point2d pt_trans;
				if (Project::getInstance()->getCameras()[i]->hasUndistortion())
				{
					pt_trans = Project::getInstance()->getCameras()[i]->getUndistortionObject()->transformPoint(points2D[i][frame], true);
				}
				else
				{
					pt_trans = points2D[i][frame];
				}

				x = pt_trans.x;
				y = pt_trans.y;
				cv::Mat projMatrs = Project::getInstance()->getCameras()[i]->getProjectionMatrix(trial->getReferenceCalibrationImage());
				for (int k = 0; k < 4; k++)
				{
					A.at<double>(count * 3 + 0, k) = x * projMatrs.at<double>(2, k) - projMatrs.at<double>(0, k);
					A.at<double>(count * 3 + 1, k) = y * projMatrs.at<double>(2, k) - projMatrs.at<double>(1, k);
					A.at<double>(count * 3 + 2, k) = x * projMatrs.at<double>(1, k) - y * projMatrs.at<double>(0, k);
				}
				count++;
			}
		}

		cv::SVD::solveZ(A, f);
		double w = f.at<double>(3, 0);
		if (w != 0.0){
			points3D[frame].x = f.at<double>(0, 0) / w;
			points3D[frame].y = f.at<double>(1, 0) / w;
			points3D[frame].z = f.at<double>(2, 0) / w;
			status3D[frame] = status;

			fprintf(stderr, "Point 3d %lf %lf %lf\n", points3D[frame].x, points3D[frame].y, points3D[frame].z);

			reprojectPoint(frame);
		}
	}
}

void Marker::reprojectPoint(int frame)
{
	for (int i = 0; i < points2D.size(); i++)
	{
		cv::Mat projMatrs = Project::getInstance()->getCameras()[i]->getProjectionMatrix(trial->getReferenceCalibrationImage());
		cv::Mat pt;
		pt.create(4, 1, CV_64F);
		pt.at<double>(0, 0) = points3D[frame].x;
		pt.at<double>(1, 0) = points3D[frame].y;
		pt.at<double>(2, 0) = points3D[frame].z;
		pt.at<double>(3, 0) = 1;
		cv::Mat pt_out = projMatrs*pt;
		double z = pt_out.at<double>(2, 0);
		if (z != 0.0){
			cv::Point2d pt_trans;
			pt_trans.x = pt_out.at<double>(0, 0) / z;
			pt_trans.y = pt_out.at<double>(1, 0) / z;

			if (Project::getInstance()->getCameras()[i]->hasUndistortion())
			{
				points2D_projected[i][frame] = Project::getInstance()->getCameras()[i]->getUndistortionObject()->transformPoint(pt_trans, false);
			}
			else
			{
				points2D_projected[i][frame] = pt_trans;
			}

			fprintf(stderr, "Point2d %d %lf %lf\n", i, points2D_projected[i][frame].x, points2D_projected[i][frame].y);

		}
	}
}

std::vector < cv::Point2d > Marker::getEpipolarLine(int cameraOrigin, int CameraDestination, int frame)
{
	std::vector < cv::Point2d > epiline;
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
	if (Project::getInstance()->getCameras()[cameraOrigin]->hasUndistortion())
	{
		pt_origin_trans = Project::getInstance()->getCameras()[cameraOrigin]->getUndistortionObject()->transformPoint(pt_origin, true);
	}
	else
	{
		pt_origin_trans = pt_origin;
	}

	cv::Mat in;
	in.create(4, 1, CV_64F);

	in.at<double>(0, 0) = pt_origin_trans.x;
	in.at<double>(1, 0) = pt_origin_trans.y;
	in.at<double>(2, 0) = 1;
	in.at<double>(3, 0) = 1;

	cv::Mat out = proj * in;

	cv::Point2d p1(out.at<double>(0, 0) / out.at<double>(0, 2),
		out.at<double>(0, 1) / out.at<double>(0, 2));

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

	if (!Project::getInstance()->getCameras()[cameraOrigin]->hasUndistortion())
	{
		cv::Point2d start(0, line_pt.y);
		double p = Project::getInstance()->getCameras()[CameraDestination]->getWidth();
		cv::Point2d end(p, line_pt.x * p + line_pt.y);
		epiline.push_back(start);
		epiline.push_back(end);
	}
	else
	{
		for (double p = 0.0; p <= ((double) Project::getInstance()->getCameras()[CameraDestination]->getWidth()); p+= 0.5)
		{
			cv::Point2d pt(p, line_pt.x * p + line_pt.y);
			cv::Point2d pt_trans = Project::getInstance()->getCameras()[CameraDestination]->getUndistortionObject()->transformPoint(pt, false);
			epiline.push_back(pt_trans);
		}
	}

	return epiline;
}

void Marker::clear(){
	for(std::vector< std::vector <cv::Point2d> >::iterator pt_it = points2D.begin(); pt_it != points2D.end(); ++pt_it){
		pt_it->clear();
	}

	for (std::vector< std::vector <cv::Point2d> >::iterator pt_it = points2D_projected.begin(); pt_it != points2D_projected.end(); ++pt_it){
		pt_it->clear();
	}

	for(std::vector< std::vector <markerStatus> >::iterator status_it = status2D.begin(); status_it != status2D.end(); ++status_it){
		status_it->clear();
	}
	for(std::vector< std::vector <double> >::iterator error_it = error2D.begin(); error_it != error2D.end(); ++error_it){
		error_it->clear();
	}

	points2D.clear();
	points2D_projected.clear();
	status2D.clear();
	error2D.clear();

	points3D.clear();
	status3D.clear();
	error3D.clear();
}

void Marker::init(int nbCameras, int size){
	clear();

	for(int c = 0; c < nbCameras; c++){
		std::vector <cv::Point2d> c_points2D;
		std::vector <cv::Point2d> c_points2D_projected;
		std::vector <markerStatus> c_status2D;
		std::vector <double> c_error2D;

		for(int i = 0; i < size; i++){	
			cv::Point2d p(-2,-2);
			c_points2D.push_back(p);
			cv::Point2d p_projected(-2, -2);
			c_points2D_projected.push_back(p_projected);

			c_status2D.push_back(UNDEFINED);
			c_error2D.push_back(0.0);
		}
		points2D.push_back(c_points2D);
		points2D_projected.push_back(c_points2D_projected);
		status2D.push_back(c_status2D);
		error2D.push_back(c_error2D);
	}

	for(int i = 0; i < size; i++){

		cv::Point3d p3(-1000,-1000,-1000);
		points3D.push_back(p3);
		status3D.push_back(UNDEFINED);
		error3D.push_back(0);
	}
}
