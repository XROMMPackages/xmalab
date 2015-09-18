#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Marker.h"
#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/HelperFunctions.h"

#include <fstream>
#include "Settings.h"

using namespace xma;

Marker::Marker(int nbCameras, int size, Trial* _trial){
	init (nbCameras, size);
	trial = _trial;
	meanSize = -1;
	thresholdOffset = 8;
	sizeOverride = -1;
	maxPenalty = 125;
	point3D_ref_set = false;
	method = 0;
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
	std::ifstream fin(filename.toAscii().data());
	std::istringstream in;
	std::string line;
	littleHelper::safeGetline(fin, line);
	in.str(line);
	std::vector<double> tmp;
	for (double value; in >> value; littleHelper::comma(in)) {
		tmp.push_back(value);
	}
	fin.close();

	if (tmp.size() == 3){
		setReference3DPoint(tmp[0], tmp[1], tmp[2]);
	}
	tmp.clear();
}

void Marker::saveReference3DPoint(QString filename)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	outfile << point3D_ref.x << " , " << point3D_ref.y << " , " << point3D_ref.z << std::endl;
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

bool Marker::getMarkerPrediction(int camera, int frame, double &x, double &y, bool forward)
{
	if (frame < 0 || frame >= points2D[0].size()) 
		return false;

	int dir = forward ? 1 : -1;

	if (frame - 1 * dir >= 0 && frame - 1 * dir < points2D[0].size() && status2D[camera][frame - 1 * dir] > 0)
	{
		if (frame - 2 * dir >= 0 && frame - 2 * dir < points2D[0].size() && status2D[camera][frame - 2 * dir] > 0)
		{
			//linear position
			x = 2 * points2D[camera][frame - 1 * dir].x - points2D[camera][frame - 2 * dir].x;
			y = 2 * points2D[camera][frame - 1 * dir].y - points2D[camera][frame - 2 * dir].y;
			return true;
		}
		else
		{
			//previous position
			x = points2D[camera][frame - 1 * dir].x;
			y = points2D[camera][frame - 1 * dir].y;
			return true;
		}
	}
	else
	{
		return false;
	}

}

void Marker::reconstruct3DPoint(int frame)
{
	status3D[frame] = UNDEFINED;
	switch (Settings::getInstance()->getIntSetting("TriangulationMethod")){
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
}

void Marker::reconstruct3DPointZisserman(int frame)
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
		A.create(2 * count, 4, CV_64F);

		count = 0;
		for (int i = 0; i < status2D.size(); i++)
		{
			if (status2D[i][frame] > 0){

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
		if (w != 0.0){
			points3D[frame].x = f.at<double>(0, 0) / w;
			points3D[frame].y = f.at<double>(1, 0) / w;
			points3D[frame].z = f.at<double>(2, 0) / w;
			status3D[frame] = status;

			reprojectPoint(frame);
		}
	}

	trial->resetRigidBodyByMarker(this, frame);
}


void Marker::reconstruct3DPointZissermanIncremental(int frame)
{
	int count = 0;
	for (int i = 0; i < status2D.size(); i++)
	{
		if (status2D[i][frame] > 0) count++;
	}

	if (count >= 2){
		markerStatus status = MANUAL_REFINED;
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

		for (int j = 0; j < 10; j++){
			count = 0;
			stop = true;
			for (int i = 0; i < status2D.size(); i++)
			{
				if (status2D[i][frame] > 0){

					status = status2D[i][frame] < status ? status2D[i][frame] : status;

					double x, y;
					cv::Point2d pt_trans;
					
					pt_trans = Project::getInstance()->getCameras()[i]->undistortPoint(points2D[i][frame], true);
					
					x = pt_trans.x;
					y = pt_trans.y;
					cv::Mat projMatrs = Project::getInstance()->getCameras()[i]->getProjectionMatrix(trial->getReferenceCalibrationImage());
					
					if (j != 0) {
						w_old = w.at<double>(count, 0);
						w.at<double>(count, 0) = f.at<double>(0, 0) * projMatrs.at<double>(2, 0) + f.at<double>(1, 0) * projMatrs.at<double>(2, 1) + f.at<double>(2, 0) * projMatrs.at<double>(2, 2) + f.at<double>(3, 0) *projMatrs.at<double>(2, 3);
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

			if (stop){ break; }

			cv::SVD::solveZ(A, f);
			//std::cerr << j << " " << f << std::endl;
		}

		if (f.at<double>(3, 0) != 0.0){
			points3D[frame].x = f.at<double>(0, 0) / f.at<double>(3, 0);
			points3D[frame].y = f.at<double>(1, 0) / f.at<double>(3, 0);
			points3D[frame].z = f.at<double>(2, 0) / f.at<double>(3, 0);
			status3D[frame] = status;

			reprojectPoint(frame);
		}

		reprojectPoint(frame);
	}

	trial->resetRigidBodyByMarker(this, frame);
}


void Marker::reconstruct3DPointZissermanMatlab(int frame)
{
	int count = 0;
	for (int i = 0; i < status2D.size(); i++)
	{
		if (status2D[i][frame] > 0) count++;
	}

	if (count >= 2){
		markerStatus status = MANUAL_REFINED;
		cv::Mat f;
		f.create(3, 1, CV_64F);

		cv::Mat A;
		A.create(2 * count, 3, CV_64F);
		cv::Mat B;
		B.create(2 * count, 1, CV_64F);

		count = 0;
		for (int i = 0; i < status2D.size(); i++)
		{
			if (status2D[i][frame] > 0){

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

	trial->resetRigidBodyByMarker(this, frame);
}

void Marker::reconstruct3DPointZissermanIncrementalMatlab(int frame)
{
	int count = 0;
	for (int i = 0; i < status2D.size(); i++)
	{
		if (status2D[i][frame] > 0) count++;
	}

	if (count >= 2){
		markerStatus status = MANUAL_REFINED;
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
		for (int j = 0; j < 10; j++){
			count = 0;
			stop = true;
			for (int i = 0; i < status2D.size(); i++)
			{
				if (status2D[i][frame] > 0){

					status = status2D[i][frame] < status ? status2D[i][frame] : status;

					double x, y;
					cv::Point2d pt_trans;

					pt_trans = Project::getInstance()->getCameras()[i]->undistortPoint(points2D[i][frame], true);

					x = pt_trans.x;
					y = pt_trans.y;
					cv::Mat projMatrs = Project::getInstance()->getCameras()[i]->getProjectionMatrix(trial->getReferenceCalibrationImage());
					projMatrs /= projMatrs.at<double>(2, 3);

					if (j != 0) {
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

			if (stop){ break; }

			cv::solve(A, B, f, cv::DECOMP_SVD);
			//std::cerr << j << " " << f << std::endl;
		}

		points3D[frame].x = f.at<double>(0, 0);
		points3D[frame].y = f.at<double>(1, 0);
		points3D[frame].z = f.at<double>(2, 0);
		status3D[frame] = status;

		reprojectPoint(frame);
	}

	trial->resetRigidBodyByMarker(this, frame);
}

void Marker::reconstruct3DPointRayIntersection(int frame)
{
	int count = 0;
	for (int i = 0; i < status2D.size(); i++)
	{
		if (status2D[i][frame] > 0) count++;
	}

	if (count >= 2){
		markerStatus status = MANUAL_REFINED;
		cv::Mat f;
		f.create(3, 1, CV_64F);

		cv::Mat A;
		A.create(3, 3, CV_64F);
		A= cv::Mat::zeros(3, 3, CV_64F);

		cv::Mat B;
		B.create(3, 1, CV_64F);
		B = cv::Mat::zeros(3, 1, CV_64F);

		cv::Mat P_inv;
		P_inv.create(4, 4, CV_64F);

		cv::Mat tmp1 = (cv::Mat_<double>(1, 4) << 0, 0, 0, 1);
		cv::Mat tmp2 = (cv::Mat_<double>(4, 1) << 0, 0, 0, 1);
		cv::Mat tmp3;
		cv::Mat origin;
		origin.create(1, 4, CV_64F);
		cv::Mat dir;
		dir.create(1, 4, CV_64F);

		count = 0;
		for (int i = 0; i < status2D.size(); i++)
		{
			if (status2D[i][frame] > 0){

				status = status2D[i][frame] < status ? status2D[i][frame] : status;

				double x, y;
				cv::Point2d pt_trans;
				pt_trans = Project::getInstance()->getCameras()[i]->undistortPoint(points2D[i][frame], true);

				x = pt_trans.x;
				y = pt_trans.y;
				
				cv::vconcat(Project::getInstance()->getCameras()[i]->getProjectionMatrix(trial->getReferenceCalibrationImage()), tmp1, P_inv);
				cv::invert(P_inv, P_inv);
			    tmp3 = (cv::Mat_<double>(4, 1) << x, y, 1, 1);

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

	trial->resetRigidBodyByMarker(this, frame);
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
	if (!points_filename.isEmpty()){
		std::ofstream outfile(points_filename.toAscii().data());
		outfile.precision(12);
		for (unsigned int j = 0; j < points2D[0].size(); j++){
			for (unsigned int i = 0; i < points2D.size(); i++){
				outfile << points2D[i][j].x << " , " << points2D[i][j].y;
				if (i != points2D.size() - 1) outfile << " , ";
			}
			outfile << std::endl;
		}
		outfile.close();
	}

	if (!status_filename.isEmpty()){
		std::ofstream outfile_status(status_filename.toAscii().data());
		outfile_status.precision(12);
		for (unsigned int j = 0; j < status2D[0].size(); j++){
			for (unsigned int i = 0; i < status2D.size(); i++){
				outfile_status << status2D[i][j];
				if (i != status2D.size() - 1) outfile_status << " , ";
			}
			outfile_status << std::endl;
		}
		outfile_status.close();
	}

	if (!markersize_filename.isEmpty()){
		std::ofstream outfile_size(markersize_filename.toAscii().data());
		outfile_size.precision(12);
		for (unsigned int j = 0; j < markerSize[0].size(); j++){
			for (unsigned int i = 0; i < markerSize.size(); i++){
				outfile_size << markerSize[i][j];
				if (i != markerSize.size() - 1) outfile_size << " , ";
			}
			outfile_size << std::endl;
		}
		outfile_size.close();
	}

	if (!pointsWorld_filename.isEmpty()){
		std::ofstream outfile_world(pointsWorld_filename.toAscii().data());
		outfile_world.precision(12);
		for (unsigned int j = 0; j < points3D.size(); j++){
			if (status3D[j] <= 0)
			{
				outfile_world << "NaN" << " , " << "NaN" << " , " << "NaN" << std::endl;
			}
			else{
				outfile_world << points3D[j].x << " , " << points3D[j].y << " , " << points3D[j].z << std::endl;
			}
		}
		outfile_world.close();
	}
}

void Marker::load(QString points_filename, QString status_filename, QString markersize_filename)
{
	std::ifstream fin;
	fin.open(points_filename.toAscii().data());
	std::istringstream in;
	std::string line;
	//read first line 
	int linecount = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; littleHelper::comma(in)) {
			tmp.push_back(value);
		}
		if (tmp.size() > 0) {
			if (points2D[0].size() <= linecount) addFrame();
			for (unsigned int i = 0; i < points2D.size(); i ++){
				points2D[i][linecount].x = tmp[2 * i];
				points2D[i][linecount].y = tmp[2 * i+1];
			}
		}
		line.clear();
		linecount++;
	}
	fin.close();

	fin.open(status_filename.toAscii().data());
	linecount = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<int> tmp;
		for (double value; in >> value; littleHelper::comma(in)) {
			tmp.push_back(value);
		}
		if (tmp.size() > 0) {
			for (unsigned int i = 0; i < status2D.size(); i++){
				status2D[i][linecount] = markerStatus(tmp[i]);
			}
		}
		line.clear();
		linecount++;
	}
	fin.close();

	fin.open(markersize_filename.toAscii().data());
	linecount = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; littleHelper::comma(in)) {
			tmp.push_back(value);
		}
		if (tmp.size() > 0) {
			for (unsigned int i = 0; i < markerSize.size(); i++){
				markerSize[i][linecount] = tmp[i];
			}
		}
		line.clear();
		linecount++;
	}
	fin.close();

	updateMeanSize();
}

void Marker::resetMultipleFrames(int camera, int frameStart, int frameEnd)
{
	//fprintf(stderr, "Delete %d - from %d  to %d\n", camera, frameStart, frameEnd);
	for (int i = frameStart; i <= frameEnd; i++)
	{
		if (camera == -1)
		{
			for (int cam = 0; cam < points2D.size(); cam++)
			{
				points2D[cam][i].x = -2;
				points2D[cam][i].y = -2;
				points2D[cam][i].x = -2;
				points2D_projected[cam][i].y = -2;
				status2D[cam][i] = UNDEFINED;
				error2D[cam][i] = 0.0;
				markerSize[cam][i] = -1.0;
			}
			points3D[i].x = -1000;
			points3D[i].y = -1000;
			points3D[i].z = -1000;
			status3D[i] = UNDEFINED;
			error3D[i] = 0.0;
		}
		else
		{
			points2D[camera][i].x = -2;
			points2D[camera][i].y = -2;
			points2D[camera][i].x = -2;
			points2D_projected[camera][i].y = -2;
			status2D[camera][i] = UNDEFINED;
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

void Marker::update()
{
	for (int i = 0; i < points2D[0].size(); i++)
	{
		reconstruct3DPoint(i);
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
		for (int c = 0; c < Project::getInstance()->getCameras().size(); c ++)	reset(c, frame);
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
}

int Marker::getMethod()
{
	return method;
}

void Marker::setMethod(int value)
{
	method = value;
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

			points2D_projected[i][frame] = Project::getInstance()->getCameras()[i]->undistortPoint(pt_trans, false);		
		}
	}

	updateError(frame);
}

void Marker::updateError(int frame)
{
	if (status3D[frame] > 0){
		for (int i = 0; i < points2D.size(); i++)
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
	for (int i = 0; i < markerSize.size(); i++)
	{
		for (int j = 0; j < markerSize[i].size(); j++)
		{
			if (markerSize[i][j] > 1 && markerSize[i][j] < 50){
				mean += markerSize[i][j];
				count++;
			}
		}
	}
	if (count > 0) meanSize = mean / count;

	sizeRange = 0;
	for (int i = 0; i < markerSize.size(); i++)
	{
		for (int j = 0; j < markerSize[i].size(); j++)
		{
			if (fabs(markerSize[i][j] - mean) > sizeRange){
				sizeRange = fabs(markerSize[i][j] - mean);
			}
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

		for (double p = 0.0; p <= ((double) Project::getInstance()->getCameras()[CameraDestination]->getWidth()); p+= 0.1)
		{
			cv::Point2d pt(p, line_pt.x * p + line_pt.y);
			if (pt.y > 0 && pt.y < height && ((pt_prev.x - pt.x)*(pt_prev.x - pt.x) + (pt_prev.y - pt.y)*(pt_prev.y - pt.y)) > dist) {
				cv::Point2d pt_trans = Project::getInstance()->getCameras()[CameraDestination]->undistortPoint(pt, false);
				if (pt_trans.x != pt.x && pt_trans.y != pt.y){
					epiline.push_back(pt_trans);
					pt_prev.x = pt.x;
					pt_prev.y = pt.y;
				}
			}
		}

		//set Last Point
		for (double p = ((double)Project::getInstance()->getCameras()[CameraDestination]->getWidth()) ; p >= 0.0; p -= 0.1)
		{
			cv::Point2d pt(p, line_pt.x * p + line_pt.y);
			if (pt.y > 0 && pt.y < height) {
				cv::Point2d pt_trans = Project::getInstance()->getCameras()[CameraDestination]->undistortPoint(pt, false);
				if (pt_trans.x != pt.x && pt_trans.y != pt.y){
					epiline.push_back(pt_trans);
					break;
				}
			}
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

	for (std::vector< std::vector <double> >::iterator size_it = markerSize.begin(); size_it != markerSize.end(); ++size_it){
		size_it->clear();
	}

	points2D.clear();
	points2D_projected.clear();
	status2D.clear();
	error2D.clear();
	markerSize.clear();

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
		std::vector <double> c_size;

		for(int i = 0; i < size; i++){	
			cv::Point2d p(-2,-2);
			c_points2D.push_back(p);
			cv::Point2d p_projected(-2, -2);
			c_points2D_projected.push_back(p_projected);

			c_status2D.push_back(UNDEFINED);
			c_error2D.push_back(0.0);
			c_size.push_back(-1.0);
		}
		points2D.push_back(c_points2D);
		points2D_projected.push_back(c_points2D_projected);
		status2D.push_back(c_status2D);
		error2D.push_back(c_error2D);
		markerSize.push_back(c_size);
	}

	for(int i = 0; i < size; i++){
		cv::Point3d p3(-1000,-1000,-1000);
		points3D.push_back(p3);
		status3D.push_back(UNDEFINED);
		error3D.push_back(0);
	}
}

void Marker::addFrame()
{
	for (int c = 0; c < points2D.size(); c++){
		points2D[c].push_back(cv::Point2d(-2, -2));
		points2D_projected[c].push_back(cv::Point2d(-2, -2));
		status2D[c].push_back(UNDEFINED);
		error2D[c].push_back(0.0);
		markerSize[c].push_back(-1.0);
	}

	points3D.push_back(cv::Point3d(-1000, -1000, -1000));
	status3D.push_back(UNDEFINED);
	error3D.push_back(0);
}