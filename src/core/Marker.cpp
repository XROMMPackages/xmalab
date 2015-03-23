#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Marker.h"
#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/UndistortionObject.h"

#include <fstream>

using namespace xma;

Marker::Marker(int nbCameras, int size, Trial* _trial){
	init (nbCameras, size);
	trial = _trial;
	meanSize = -1;
	thresholdOffset = 8;
	sizeOverride = -1;
	maxPenalty = 125;
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

			//fprintf(stderr, "Point %lf %lf %lf\n", points3D[frame].x, points3D[frame].y, points3D[frame].z);

			reprojectPoint(frame);
		}
	}

	trial->resetRigidBodyByMarker(this,frame);
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

std::istream& Marker::comma(std::istream& in)
{
	if ((in >> std::ws).peek() != std::char_traits<char>::to_int_type(',')) {
		in.setstate(std::ios_base::failbit);
	}
	return in.ignore();
}

std::istream &Marker::getline(std::istream &is, std::string &s) {
	char ch;

	s.clear();

	while (is.get(ch) && ch != '\n' && ch != '\r')
		s += ch;
	return is;
}

void Marker::load(QString points_filename, QString status_filename, QString markersize_filename)
{
	std::ifstream fin;
	fin.open(points_filename.toAscii().data());
	std::istringstream in;
	std::string line;
	//read first line 
	int linecount = 0;
	for (; getline(fin, line);)
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; comma(in)) {
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
	for (; getline(fin, line);)
	{
		in.clear();
		in.str(line);
		std::vector<int> tmp;
		for (double value; in >> value; comma(in)) {
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
	for (; getline(fin, line);)
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; comma(in)) {
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

	if (markerSize[camera][frame] >50)
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
		for (double p = 0.0; p <= ((double) Project::getInstance()->getCameras()[CameraDestination]->getWidth()); p+= 50.0)
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