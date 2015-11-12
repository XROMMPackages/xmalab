#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/RigidBody.h"
#include "core/Trial.h"
#include "core/Camera.h"
#include "core/Marker.h"
#include "core/Project.h"
#include "core/UndistortionObject.h"
#include "core/CalibrationImage.h"
#include "core/HelperFunctions.h"

#include "processing/ButterworthLowPassFilter.h" //should move this dependency

#include <fstream>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#ifndef M_PI
	#define M_PI   3.14159265358979323846	
#endif

using namespace xma;

RigidBody::RigidBody(int size, Trial * _trial){
	expanded = false;
	initialised = false;
	trial = _trial;
	referencesSet = false;

	color.setRgb(255,0,0);
	visible = false;

	cutoffFrequency = 0;
	overrideCutoffFrequency = false;

	init(size);
}

void RigidBody::copyData(RigidBody *rb)
{
	setDescription(rb->getDescription());

	clearPointIdx();

	for (int j = 0; j < rb->getPointsIdx().size(); j++){
		addPointIdx(rb->getPointsIdx()[j]);
	}

	if (rb->isReferencesSet())
	{
		setReferencesSet(rb->isReferencesSet());

		for (int i = 0; i < rb->referenceNames.size(); i++)
		{
			referenceNames[i] = rb->referenceNames[i];
		}
		for (int i = 0; i < rb->points3D.size(); i++)
		{
			points3D[i] = rb->points3D[i];
		}

		updateCenter();
	}
}

RigidBody::~RigidBody(){
	pointsIdx.clear();
	points3D.clear();
	referenceNames.clear();

	clear();
}


void RigidBody::setDescription(QString _description)
{
	description = _description;
}

QString RigidBody::getDescription()
{
	return description;
}

const std::vector<int>& RigidBody::getPointsIdx()
{
	return pointsIdx;
}

const std::vector<int>& RigidBody::getPoseFiltered()
{
	return poseFiltered;
}

const std::vector<int>& RigidBody::getPoseComputed()
{
	return poseComputed;
}

const std::vector<QString>& RigidBody::getReferenceNames()
{
	return referenceNames;
}

Marker* RigidBody::getMarker(int idx)
{
	return trial->getMarkers()[idx];
}

void RigidBody::clearPointIdx()
{
	pointsIdx.clear();
	points3D.clear();
	referenceNames.clear();
	resetReferences();
}

void RigidBody::setPointIdx(int idx, int markerIdx)
{
	pointsIdx[idx] = markerIdx;
}

void RigidBody::addPointIdx(int idx)
{
	pointsIdx.push_back(idx);
	points3D.push_back(cv::Point3d(0, 0, 0));
	referenceNames.push_back("");
	resetReferences();
}

void RigidBody::removePointIdx(int idx)
{
	int pos = std::find(pointsIdx.begin(), pointsIdx.end(), idx) - pointsIdx.begin();
	points3D.erase(std::remove(points3D.begin(), points3D.end(), points3D[pos]), points3D.end());
	referenceNames.erase(std::remove(referenceNames.begin(), referenceNames.end(), referenceNames[pos]), referenceNames.end());
	pointsIdx.erase(std::remove(pointsIdx.begin(), pointsIdx.end(), idx), pointsIdx.end());
	resetReferences();
}

void RigidBody::resetReferences()
{
	for (int i = 0; i < referenceNames.size(); i++)
	{
		referenceNames[i] = "";
		points3D[i].x = 0;
		points3D[i].y = 0;
		points3D[i].z = 0;
	}
	setReferencesSet(0);
	initialised = false;
	setReferenceMarkerReferences();
}

bool RigidBody::allReferenceMarkerReferencesSet()
{
	bool allset = true;
	for (int i = 0; i < pointsIdx.size(); i++)
	{
		if (!trial->getMarkers()[pointsIdx[i]]->Reference3DPointSet())
		{
			allset = false;
		}
	}
	return allset;
}

void RigidBody::setReferenceMarkerReferences()
{
	if (allReferenceMarkerReferencesSet())
	{
		for (int i = 0; i < pointsIdx.size(); i++)
		{
			points3D[i].x = trial->getMarkers()[pointsIdx[i]]->getReference3DPoint().x;
			points3D[i].y = trial->getMarkers()[pointsIdx[i]]->getReference3DPoint().y;
			points3D[i].z = trial->getMarkers()[pointsIdx[i]]->getReference3DPoint().z;
			referenceNames[i] = trial->getMarkers()[pointsIdx[i]]->getDescription() + " - FromMarker";
		}
		initialised = true;
		setReferencesSet(1);
		recomputeTransformations();
	}
}


void RigidBody::setExpanded(bool _expanded)
{
	expanded = _expanded;
}

bool RigidBody::isExpanded()
{
	return expanded;
}

void RigidBody::clear(){
	rotationvectors.clear();
	translationvectors.clear();
	translationvectors_filtered.clear();
	rotationvectors_filtered.clear();
	poseComputed.clear();
	poseFiltered.clear();

	errorMean2D.clear();
	errorSd2D.clear();
	errorMean3D.clear();
	errorSd3D.clear();

	errorMean2D_filtered.clear();
	errorSd2D_filtered.clear();
	errorMean3D_filtered.clear();
	errorSd3D_filtered.clear();
}


void RigidBody::addFrame()
{
	rotationvectors.push_back(cv::Vec3d());
	translationvectors.push_back(cv::Vec3d());
	rotationvectors_filtered.push_back(cv::Vec3d());
	translationvectors_filtered.push_back(cv::Vec3d());

	poseComputed.push_back(0);
	poseFiltered.push_back(0);

	errorMean2D.push_back(0);
	errorSd2D.push_back(0);
	errorMean3D.push_back(0);
	errorSd3D.push_back(0);

	errorMean2D_filtered.push_back(0);
	errorSd2D_filtered.push_back(0);
	errorMean3D_filtered.push_back(0);
	errorSd3D_filtered.push_back(0);
}

void RigidBody::clearAllDummyPoints()
{
	dummyNames.clear();
	dummypoints.clear();
	dummypointsCoords.clear();
	dummypointsCoordsSet.clear();
}

const std::vector<QString>& RigidBody::getDummyNames()
{
	return dummyNames;
}

void RigidBody::init(int size){
	clear();

	for(int i = 0 ; i < size; i ++){
		rotationvectors.push_back(cv::Vec3d());
		translationvectors.push_back(cv::Vec3d());
		rotationvectors_filtered.push_back(cv::Vec3d());
		translationvectors_filtered.push_back(cv::Vec3d());

		poseComputed.push_back(0);
		poseFiltered.push_back(0);

		errorMean2D.push_back(0);
		errorSd2D.push_back(0);
		errorMean3D.push_back(0);
		errorSd3D.push_back(0);

		errorMean2D_filtered.push_back(0);
		errorSd2D_filtered.push_back(0);
		errorMean3D_filtered.push_back(0);
		errorSd3D_filtered.push_back(0);
	}
}

void RigidBody::computeCoordinateSystemAverage(){
	if (!isReferencesSet()){

		std::vector <cv::Point3d> points3D_mean;

		int count = 0;
		for (unsigned int f = 0; f < poseComputed.size(); f++){
			computeCoordinateSystem(f);
			computePose(f);
			if (poseComputed[f]){
				bool useFrame = true;
				for (unsigned int i = 0; i < pointsIdx.size(); i++){
					if (trial->getMarkers()[pointsIdx[i]]->getStatus3D()[f] <= UNDEFINED){
						useFrame = false;
					}
				}
				if (useFrame){
					count++;
					for (unsigned int i = 0; i < pointsIdx.size(); i++){
						cv::Point3d pt = trial->getMarkers()[pointsIdx[i]]->getPoints3D()[f];

						cv::Mat xmat = cv::Mat(pt, true);

						cv::Mat rotMattmp;
						cv::Rodrigues(rotationvectors[f], rotMattmp);
						cv::Mat tmp_mat = rotMattmp * (xmat) + cv::Mat(translationvectors[f]);

						pt.x = tmp_mat.at<double>(0, 0);
						pt.y = tmp_mat.at<double>(1, 0);
						pt.z = tmp_mat.at<double>(2, 0);

						if (count == 1){
							points3D_mean.push_back(pt);
						}
						else{
							points3D_mean[i] += pt;
						}
						rotMattmp.release();
					}
				}
			}
		}

		if (count == 0) return;

		for (unsigned int i = 0; i < points3D_mean.size(); i++){
			points3D_mean[i].x /= count;
			points3D_mean[i].y /= count;
			points3D_mean[i].z /= count;
		}

		cv::Point3d center = cv::Point3d(0, 0, 0);
		for (unsigned int i = 0; i < points3D_mean.size(); i++){
			center += points3D_mean[i];
		}

		if (points3D_mean.size() > 0){
			center.x /= pointsIdx.size();
			center.y /= pointsIdx.size();
			center.z /= pointsIdx.size();
		}

		if (points3D_mean.size() >= 2){
			cv::Point3d px = points3D_mean[0] - center;
			cv::Point3d pxy = points3D_mean[1] - center;

			double normx = norm(px);
			px.x /= normx;
			px.y /= normx;
			px.z /= normx;

			double normxy = norm(pxy);
			pxy.x /= normxy;
			pxy.y /= normxy;
			pxy.z /= normxy;

			cv::Point3d py = px.cross(pxy);
			double normy = norm(py);
			py.x /= normy;
			py.y /= normy;
			py.z /= normy;

			cv::Point3d pz = py.cross(px);

			cv::Mat rotationmatrix;
			rotationmatrix.create(3, 3, CV_64F);

			rotationmatrix.at<double>(0, 0) = px.x;
			rotationmatrix.at<double>(0, 1) = px.y;
			rotationmatrix.at<double>(0, 2) = px.z;

			rotationmatrix.at<double>(1, 0) = pz.x;
			rotationmatrix.at<double>(1, 1) = pz.y;
			rotationmatrix.at<double>(1, 2) = pz.z;

			rotationmatrix.at<double>(2, 0) = py.x;
			rotationmatrix.at<double>(2, 1) = py.y;
			rotationmatrix.at<double>(2, 2) = py.z;

			points3D.clear();

			for (unsigned int i = 0; i < points3D_mean.size(); i++){
				cv::Point3d pt = points3D_mean[i] - center;
				cv::Mat src(3/*rows*/, 1 /* cols */, CV_64F);
				src.at<double>(0, 0) = pt.x;
				src.at<double>(1, 0) = pt.y;
				src.at<double>(2, 0) = pt.z;

				cv::Mat dst = rotationmatrix * src;
				pt.x = dst.at<double>(0, 0);
				pt.y = dst.at<double>(1, 0);
				pt.z = dst.at<double>(2, 0);
				points3D.push_back(pt);
				//std::cerr << "RB pt" << i << " coords" << pt << std::endl;
			}
			rotationmatrix.release();
			initialised = true;
		}
		else{
			points3D.clear();
			for (unsigned int i = 0; i < points3D_mean.size(); i++){
				cv::Point3d pt = points3D_mean[i] - center;
				points3D.push_back(pt);
			}
		}
		points3D_mean.clear();

		for (unsigned int f = 0; f < poseComputed.size(); f++){
			computePose(f);
		}
	}
}

void RigidBody::computeCoordinateSystem(int Frame){
	
	if (!isReferencesSet()){
		

		int count = 0;

		for (unsigned int i = 0; i < pointsIdx.size(); i++){
			if (trial->getMarkers()[pointsIdx[i]]->getStatus3D()[Frame] <= UNDEFINED) return;
		}
		points3D.clear();
		cv::Point3d center = cv::Point3d(0, 0, 0);
		for (unsigned int i = 0; i < pointsIdx.size(); i++){
			center += trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame];
		}

		if (pointsIdx.size() > 0){
			center.x /= pointsIdx.size();
			center.y /= pointsIdx.size();
			center.z /= pointsIdx.size();
		}

		if (pointsIdx.size() >= 2){
			cv::Point3d px = trial->getMarkers()[pointsIdx[0]]->getPoints3D()[Frame] - center;
			cv::Point3d pxy = trial->getMarkers()[pointsIdx[1]]->getPoints3D()[Frame] - center;

			double normx = norm(px);
			px.x /= normx;
			px.y /= normx;
			px.z /= normx;

			double normxy = norm(pxy);
			pxy.x /= normxy;
			pxy.y /= normxy;
			pxy.z /= normxy;

			cv::Point3d py = px.cross(pxy);
			double normy = norm(py);
			py.x /= normy;
			py.y /= normy;
			py.z /= normy;

			cv::Point3d pz = py.cross(px);

			cv::Mat rotationmatrix;
			rotationmatrix.create(3, 3, CV_64F);

			rotationmatrix.at<double>(0, 0) = px.x;
			rotationmatrix.at<double>(0, 1) = px.y;
			rotationmatrix.at<double>(0, 2) = px.z;

			rotationmatrix.at<double>(1, 0) = pz.x;
			rotationmatrix.at<double>(1, 1) = pz.y;
			rotationmatrix.at<double>(1, 2) = pz.z;

			rotationmatrix.at<double>(2, 0) = py.x;
			rotationmatrix.at<double>(2, 1) = py.y;
			rotationmatrix.at<double>(2, 2) = py.z;

			for (unsigned int i = 0; i < pointsIdx.size(); i++){
				cv::Point3d pt = trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame] - center;
				cv::Mat src(3/*rows*/, 1 /* cols */, CV_64F);
				src.at<double>(0, 0) = pt.x;
				src.at<double>(1, 0) = pt.y;
				src.at<double>(2, 0) = pt.z;

				cv::Mat dst = rotationmatrix * src;
				pt.x = dst.at<double>(0, 0);
				pt.y = dst.at<double>(1, 0);
				pt.z = dst.at<double>(2, 0);
				points3D.push_back(pt);
			}
			rotationmatrix.release();
			initialised = true;
		}
		else{
			for (unsigned int i = 0; i < pointsIdx.size(); i++){
				cv::Point3d pt = trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame] - center;
				points3D.push_back(pt);
			}
		}
	}
}

void RigidBody::computePose(int Frame){
	while ( Frame >= poseComputed.size() ) addFrame();

	poseComputed[Frame] = 0;

	if (initialised){
		std::vector <cv::Point3d> src;
		std::vector <cv::Point3d> dst;
		cv::Mat out;
		cv::Mat inliers;

		for (unsigned int i = 0; i < pointsIdx.size(); i++){
			if (trial->getMarkers()[pointsIdx[i]]->getStatus3D()[Frame] > UNDEFINED){
				src.push_back(cv::Point3f(trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame].x
					, trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame].y
					, trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame].z));
				dst.push_back(cv::Point3f(points3D[i].x
					, points3D[i].y
					, points3D[i].z));
			}
		}

		for (unsigned int i = 0; i < dummypoints.size(); i++){
			if (dummypointsCoordsSet[i][Frame]){
				src.push_back(cv::Point3f(dummypointsCoords[i][Frame].x
					, dummypointsCoords[i][Frame].y
					, dummypointsCoords[i][Frame].z));
				dst.push_back(cv::Point3f(dummypoints[i].x
					, dummypoints[i].y
					, dummypoints[i].z));
			}
		}

		if (dst.size() >= 3){
			cv::vector< cv::vector<double> > y, x;
			cv::vector< cv::vector<double> > Y, X;
			cv::vector<double> vnl_tmp(3), yg(3, 0), xg(3, 0);
			cv::vector<double> tmp;
			cv::Mat K = cv::Mat::zeros(3, 3, CV_64F);

			//set Data
			for (unsigned int i = 0; i< src.size(); i++) {
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
			for (int i = 0; i<X.size(); i++)
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
			for (unsigned int i = 0; i<x.size(); i++)
			{
				for (int m = 0; m < 3; m++) x[i][m] = x[i][m] - xg[m];
				for (int m = 0; m < 3; m++) y[i][m] = y[i][m] - yg[m];
			}

			// Compute the cavariance matrix K = Sum_i( yi.xi' )
			for (unsigned int i = 0; i<x.size(); i++){
				for (int j = 0; j<3; j++){
					for (int l = 0; l<3; l++){
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
			rotMatTmp = U*Sd*VT;
			cv::Rodrigues(rotMatTmp, rotationvectors[Frame]);

			cv::Mat xgmat = cv::Mat(xg, true);
			cv::Mat ygmat = cv::Mat(yg, true);
			cv::Mat t = ygmat - rotMatTmp  * xgmat;
			translationvectors[Frame] = cv::Vec3d(t);

			poseComputed[Frame] = 1;

			
		}
	}
	updateError(Frame);

}

void RigidBody::updateError(int Frame, bool filtered)
{
	double eMean2D = 0;
	double eMean3D = 0;
	double eSD2D = 0;
	double eSD3D = 0;

	int count = 0;

	if ((poseComputed[Frame] && !filtered) || (filtered && poseFiltered[Frame])){
		count = 0;

		std::vector <double> dist;
		for (int c = 0; c < Project::getInstance()->getCameras().size(); c++){
			std::vector <cv::Point2d> image_points = projectToImage(Project::getInstance()->getCameras()[c], Frame, false, false, false, filtered);

			for (int i = 0; i < pointsIdx.size(); i++){
				if (trial->getMarkers()[pointsIdx[i]]->getStatus2D()[c][Frame] > UNDEFINED){
					cv::Point2d diff = trial->getMarkers()[pointsIdx[i]]->getPoints2D()[c][Frame] - image_points[i];
					dist.push_back(cv::sqrt(diff.x*diff.x + diff.y*diff.y));
					eMean2D += dist[count];
					count++;
				}
			}

			image_points.clear();
		}

		if (count != 0) eMean2D /= count;

		for (int i = 0; i < dist.size(); i++){
			eSD2D += pow(dist[i] - eMean2D, 2);
		}
		if (count != 0) eSD2D = sqrt(eSD2D / (count - 1));;

		count = 0;
		cv::Mat rotationMatrix;
		if (filtered){
			cv::Rodrigues(rotationvectors_filtered[Frame], rotationMatrix);
		}
		else
		{
			cv::Rodrigues(rotationvectors[Frame], rotationMatrix);
		}
		dist.clear();

		for (int i = 0; i < points3D.size(); i++){
			cv::Mat xmat = cv::Mat(points3D[i], true);
			cv::Mat tmp_mat;

			if (filtered){
				tmp_mat = rotationMatrix.t() * ((xmat)-cv::Mat(translationvectors_filtered[Frame]));
			}
			else
			{
				tmp_mat = rotationMatrix.t() * ((xmat)-cv::Mat(translationvectors[Frame]));
			}

			if (trial->getMarkers()[pointsIdx[i]]->getStatus3D()[Frame] > 0){
				
				cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
					tmp_mat.at<double>(0, 1),
					tmp_mat.at<double>(0, 2));
		
				cv::Point3d diff = trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame] - pt3d;
				dist.push_back(cv::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z));
				eMean3D += dist[count];
				count++;
			}
		}
		if (count != 0) eMean3D /= count;
		
		for (int i = 0; i < dist.size(); i++){
			eSD3D += pow(dist[i] - eMean3D, 2);
		}
		if (count != 0) eSD3D = sqrt(eSD3D  / (count - 1));;
		
		dist.clear();
		rotationMatrix.release();
	}

	if (!filtered){
		errorMean2D[Frame] = eMean2D;
		errorSd2D[Frame] = eSD2D;
		errorMean3D[Frame] = eMean3D;
		errorSd3D[Frame] = eSD3D;
	}
	else
	{
		errorMean2D_filtered[Frame] = eMean2D;
		errorSd2D_filtered[Frame] = eSD2D;
		errorMean3D_filtered[Frame] = eMean3D;
		errorSd3D_filtered[Frame] = eSD3D;
	}

	
	//std::cerr << Frame << " " << errorMean2D[Frame] << "+/-" << errorSd2D[Frame] << "  " << errorMean3D[Frame] << "+/-" << errorSd3D[Frame] << std::endl;

}

double RigidBody::fitAndComputeError(std::vector<cv::Point3d> src, std::vector<cv::Point3d> dst){
	double errorMean = 0;

	//for (unsigned int i = 0; i < src.size(); i++)
	//{
	//	fprintf(stderr, "%d %lf %lf %lf - %lf %lf %lf\n", i, dst[i].x, dst[i].y, dst[i].z, src[i].x, src[i].y, src[i].z);
	//}

	if (src.size() >= 3){
		cv::vector< cv::vector<double> > y, x;
		cv::vector< cv::vector<double> > Y, X;
		cv::vector<double> vnl_tmp(3), yg(3, 0), xg(3, 0);
		cv::vector<double> tmp;
		cv::Mat K = cv::Mat::zeros(3, 3, CV_64F);

		//set Data
		for (unsigned int i = 0; i< src.size(); i++) {
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
		for (int i = 0; i<X.size(); i++)
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
		for (unsigned int i = 0; i<x.size(); i++)
		{
			for (int m = 0; m < 3; m++) x[i][m] = x[i][m] - xg[m];
			for (int m = 0; m < 3; m++) y[i][m] = y[i][m] - yg[m];
		}

		// Compute the cavariance matrix K = Sum_i( yi.xi' )
		for (unsigned int i = 0; i<x.size(); i++){
			for (int j = 0; j<3; j++){
				for (int l = 0; l<3; l++){
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

		cv::Mat rotMat;
		rotMat = U*Sd*VT;
			
		cv::Mat xgmat = cv::Mat(xg, true);
		cv::Mat ygmat = cv::Mat(yg, true);

		cv::Mat transMat;
		transMat = ygmat - rotMat  * xgmat;

		std::vector <cv::Point3d> points3D_frame;

		for (int i = 0; i < dst.size(); i++){
			cv::Mat xmat = cv::Mat(dst[i], true);
			cv::Mat tmp_mat = rotMat.t() * ((xmat)-transMat);
		
			cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
				tmp_mat.at<double>(1, 0),
				tmp_mat.at<double>(2, 0));
		
			cv::Point3d diff = src[i] - pt3d;
			errorMean += cv::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
		}
		
		errorMean /= dst.size();
	}
	initialised = true;

	return errorMean;
}

void RigidBody::save(QString filename_referenceNames, QString filename_points3D)
{
	std::ofstream outfile_Names(filename_referenceNames.toAscii().data());
	outfile_Names.precision(12);
	for (unsigned int i = 0; i < referenceNames.size(); i++){
		outfile_Names << referenceNames[i].toAscii().data() << std::endl;
	}
	outfile_Names.close();
	std::ofstream outfile_Points(filename_points3D.toAscii().data());
	outfile_Points.precision(12);
	for (unsigned int j = 0; j < points3D.size(); j++){
		outfile_Points << points3D[j].x << " , " << points3D[j].y << " , " << points3D[j].z << std::endl;
	}
	outfile_Points.close();
}

void RigidBody::load(QString filename_referenceNames, QString filename_points3D)
{
	std::ifstream fin;
	std::istringstream in;
	std::string line;

	fin.open(filename_points3D.toAscii().data());
	points3D.clear();
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		QString tmp_coords = QString::fromStdString(line);
		QStringList coords_list = tmp_coords.split(",");
		if (coords_list.size() == 3)
		{
			points3D.push_back(cv::Point3d(coords_list.at(0).toDouble(), coords_list.at(1).toDouble(), coords_list.at(2).toDouble()));
		}
	}
	fin.close();

	fin.open(filename_referenceNames.toAscii().data());
	referenceNames.clear();
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		if (!line.empty()){
			QString tmp_Name = QString::fromStdString(line);
			referenceNames.push_back(tmp_Name);
		}
	}
	fin.close();

	setReferencesSet(2);
	initialised = true;
}

void RigidBody::recomputeTransformations()
{
	for (int i = 0; i < trial->getNbImages(); i++)
	{
		computePose(i);
	}

	makeRotationsContinous();
}

void RigidBody::makeRotationsContinous()
{
	//set rotations numerically steady
	for (int i = 1; i < trial->getNbImages(); i++)
	{
		if (poseComputed[i - 1] && poseComputed[i])
		{
			double d = rotationvectors[i - 1].dot(rotationvectors[i]);
			double diffangle = d / cv::norm(rotationvectors[i - 1]) / cv::norm(rotationvectors[i]);

			if (180 / M_PI * acos(diffangle) > 150)
			{
				double angle = cv::norm(rotationvectors[i]);
				rotationvectors[i][0] = -rotationvectors[i][0] / angle * (2 * M_PI - angle);
				rotationvectors[i][1] = -rotationvectors[i][1] / angle * (2 * M_PI - angle);
				rotationvectors[i][2] = -rotationvectors[i][2] / angle * (2 * M_PI - angle);
			}
		}
	}
}

void RigidBody::filterData(std::vector<int> idx)
{
	if (idx.size() <= 10)
	{
		for (int i = 0; i < idx.size(); i++)
		{
			translationvectors_filtered[idx[i]][0] = translationvectors[idx[i]][0];
			translationvectors_filtered[idx[i]][1] = translationvectors[idx[i]][1];
			translationvectors_filtered[idx[i]][2] = translationvectors[idx[i]][2];

			rotationvectors_filtered[idx[i]][0] = rotationvectors[idx[i]][0];
			rotationvectors_filtered[idx[i]][1] = rotationvectors[idx[i]][1];
			rotationvectors_filtered[idx[i]][2] = rotationvectors[idx[i]][2];
			poseFiltered[idx[i]] = 1;
		}
	}else{
		std::vector<double> t1;
		std::vector<double> t2;
		std::vector<double> t3;
		std::vector<double> r1;
		std::vector<double> r2;
		std::vector<double> r3;
		std::vector<double> r4;

		for (int i = 0; i < idx.size(); i++)
		{
			double angle = cv::norm(rotationvectors[idx[i]]);

			r1.push_back(rotationvectors[idx[i]][0]);
			r2.push_back(rotationvectors[idx[i]][1]);
			r3.push_back(rotationvectors[idx[i]][2]);

			t1.push_back(translationvectors[idx[i]][0]);
			t2.push_back(translationvectors[idx[i]][1]);
			t3.push_back(translationvectors[idx[i]][2]);
		}

		std::vector<double> t1_out;
		std::vector<double> t2_out;
		std::vector<double> t3_out;
		std::vector<double> r1_out;
		std::vector<double> r2_out;
		std::vector<double> r3_out;

		double cutoff = (getOverrideCutoffFrequency()) ? getCutoffFrequency() : trial->getCutoffFrequency();

		ButterworthLowPassFilter * filter = new ButterworthLowPassFilter(4, cutoff, trial->getRecordingSpeed());

		filter->filter(t1, t1_out);
		filter->filter(t2, t2_out);
		filter->filter(t3, t3_out);
		filter->filter(r1, r1_out);
		filter->filter(r2, r2_out);
		filter->filter(r3, r3_out);

		for (int i = 0; i < idx.size(); i++)
		{
			translationvectors_filtered[idx[i]][0] = t1_out[i];
			translationvectors_filtered[idx[i]][1] = t2_out[i];
			translationvectors_filtered[idx[i]][2] = t3_out[i];

			rotationvectors_filtered[idx[i]][0] = r1_out[i];
			rotationvectors_filtered[idx[i]][1] = r2_out[i];
			rotationvectors_filtered[idx[i]][2] = r3_out[i];
			poseFiltered[idx[i]] = 1;
		}

		delete filter;

		t1_out.clear();
		t2_out.clear();
		t3_out.clear();
		r1_out.clear();
		r2_out.clear();
		r3_out.clear();
		t1.clear();
		t2.clear();
		t3.clear();
		r1.clear();
		r2.clear();
		r3.clear();
	}
}

void RigidBody::filterTransformations()
{
	for (int i = 0; i < trial->getNbImages(); i++)
	{
		poseFiltered[i] = 0;
	}

	double cutoff = (getOverrideCutoffFrequency()) ? getCutoffFrequency() : trial->getCutoffFrequency();

	if (cutoff > 0 && trial->getRecordingSpeed() > 0 &&
		0 < (cutoff / (trial->getRecordingSpeed() * 0.5)) &&
		(cutoff / (trial->getRecordingSpeed() * 0.5)) < 1){
		
		std::vector<int> idx;
		for (int i = 0; i < trial->getNbImages(); i++)
		{
			if (poseComputed[i])
			{
				idx.push_back(i);
			}
			else
			{
				if (idx.size() >= 1)
				{
					filterData(idx);
				}
				idx.clear();
			}

			if (i == trial->getNbImages() - 1)
			{
				if (idx.size() >= 1)
				{
					filterData(idx);
				}
				idx.clear();
			}
		}
	}

	for (int i = 0; i < trial->getNbImages(); i++)
	{
		updateError(i, true);
	}
}

Trial* RigidBody::getTrial()
{
	return trial;
}

void RigidBody::addDummyPoint(QString name, QString filenamePointRef, QString filenamePointCoords)
{
	dummyNames.push_back(name);

	std::ifstream fin;
	std::string line;

	fin.open(filenamePointRef.toAscii().data());
	littleHelper::safeGetline(fin, line);
	littleHelper::safeGetline(fin, line);
	fin.close();
	QString tmp_coords = QString::fromStdString(line);
	QStringList coords_list = tmp_coords.split(",");
	dummypoints.push_back(cv::Point3d(coords_list.at(0).toDouble(), coords_list.at(1).toDouble(), coords_list.at(2).toDouble()));

	fin.open(filenamePointCoords.toAscii().data());
	littleHelper::safeGetline(fin, line);
	std::vector<cv::Point3d> tmpCoords;
	std::vector<bool> tmpDef;

	while (!littleHelper::safeGetline(fin, line).eof())
	{
		tmp_coords = QString::fromStdString(line);
		coords_list = tmp_coords.split(",");
		if (coords_list.size() == 3){
			if (coords_list.at(0) == "NaN" || coords_list.at(1) == "NaN" || coords_list.at(2) == "NaN")
			{
				tmpDef.push_back(false);
				tmpCoords.push_back(cv::Point3d(0, 0, 0));
			}
			else{
				tmpDef.push_back(true);
				tmpCoords.push_back(cv::Point3d(coords_list.at(0).toDouble(), coords_list.at(1).toDouble(), coords_list.at(2).toDouble()));
			}
		}
	}
	dummypointsCoords.push_back(tmpCoords);
	dummypointsCoordsSet.push_back(tmpDef);
	fin.close();
}

void RigidBody::saveDummy(int count, QString filenamePointRef, QString filenamePointCoords)
{
	std::ofstream outfileRef(filenamePointRef.toAscii().data());
	outfileRef.precision(12);
	outfileRef << "x,y,z" << std::endl;
	outfileRef << dummypoints[count].x << "," << dummypoints[count].y << "," << dummypoints[count].z << std::endl;
	outfileRef.close();

	std::ofstream outfileCoords(filenamePointCoords.toAscii().data());
	outfileCoords.precision(12);
	outfileCoords << "x,y,z" << std::endl;
	for (int i = 0; i < dummypointsCoords[count].size(); i++){
		if (dummypointsCoordsSet[count][i]){
			outfileCoords << dummypointsCoords[count][i].x << "," << dummypointsCoords[count][i].y << "," << dummypointsCoords[count][i].z << std::endl;
		}
		else
		{
			outfileCoords << "NaN,NaN,NaN" << std::endl;
		}
	}
	outfileRef.close();

}

const std::vector<cv::Vec3d>& RigidBody::getRotationVector(bool filtered)
{
	if (filtered)
	{
		return rotationvectors_filtered;
	}
	else
	{ 
		return rotationvectors;
	}
}

const std::vector<cv::Vec3d>& RigidBody::getTranslationVector(bool filtered)
{
	if (filtered)
	{
		return translationvectors_filtered;
	}
	else
	{
		return translationvectors;
	}
}

const std::vector<double>& RigidBody::getErrorMean2D()
{
	return errorMean2D;
}

const std::vector<double>& RigidBody::getErrorSd2D()
{
	return errorSd2D;
}

const std::vector<double>& RigidBody::getErrorMean3D()
{
	return errorMean3D;
}

const std::vector<double>& RigidBody::getErrorSd3D()
{
	return errorSd3D;
}

const std::vector<double>& RigidBody::getErrorMean2D_filtered()
{
	return errorMean2D_filtered;
}

const std::vector<double>& RigidBody::getErrorSd2D_filtered()
{
	return errorSd2D_filtered;
}

const std::vector<double>& RigidBody::getErrorMean3D_filtered()
{
	return errorMean3D_filtered;
}

const std::vector<double>& RigidBody::getErrorSd3D_filtered()
{
	return errorSd3D_filtered;
}
double RigidBody::getRotationEulerAngle(bool filtered, int frame, int part)
{
	cv::Mat rotMat;
	if (filtered)
	{
		cv::Rodrigues(rotationvectors_filtered[frame], rotMat);
	}
	else
	{
		cv::Rodrigues(rotationvectors[frame], rotMat);
	}
	double conv = 180 / M_PI;
	if (part == 0)
	{
		return conv * atan2(rotMat.at<double>(2, 1), rotMat.at<double>(2, 2));
	}
	else if (part == 1)
	{
		return conv * atan2(-rotMat.at<double>(2, 0), sqrt(rotMat.at<double>(2, 1) * rotMat.at<double>(2, 1) + rotMat.at<double>(2, 2)*rotMat.at<double>(2, 2)));
	}
	else if (part == 2)
	{
		return conv * atan2(rotMat.at<double>(1, 0), rotMat.at<double>(0, 0));
	}
}

void RigidBody::saveTransformations(QString filename, bool inverse, bool filtered)
{
	if (!filename.isEmpty()){
		std::ofstream outfile(filename.toAscii().data());
		outfile.precision(12);
		for (unsigned int i = 0; i < trial->getNbImages(); i++){
			if ((poseComputed[i] && !filtered) || (poseFiltered[i] && filtered)){
				double m[16];
				//inversere Rotation = transposed rotation
				//and opengl requires transposed, so we set R
				if (inverse){

					cv::Mat rotationMat;
					if (!filtered){
						cv::Rodrigues(rotationvectors[i], rotationMat);
					}
					else
					{
						cv::Rodrigues(rotationvectors_filtered[i], rotationMat);
					}
					for (unsigned int y = 0; y < 3; y++)
					{
						m[y * 4] = rotationMat.at<double>(y, 0);
						m[y * 4 + 1] = rotationMat.at<double>(y, 1);
						m[y * 4 + 2] = rotationMat.at<double>(y, 2);
						m[y * 4 + 3] = 0.0;
					}
					//inverse translation = translation rotated with inverse rotation/transposed rotation
					//R-1 * -t = R^tr * -t

					cv::Vec3d trans;
					if (!filtered){
						trans = translationvectors[i];
					}
					else
					{
						trans = translationvectors_filtered[i];
					}
					m[12] = m[0] * -trans[0] + m[4] * -trans[1] + m[8] * -trans[2];
					m[13] = m[1] * -trans[0] + m[5] * -trans[1] + m[9] * -trans[2];
					m[14] = m[2] * -trans[0] + m[6] * -trans[1] + m[10] * -trans[2];
					m[15] = 1.0;
				}
				else
				{
					cv::Mat rotationMat;
					if (!filtered){
						cv::Rodrigues(rotationvectors[i], rotationMat);
					}
					else
					{
						cv::Rodrigues(rotationvectors_filtered[i], rotationMat);
					}
					for (unsigned int y = 0; y < 3; y++)
					{
						m[y * 4] = rotationMat.at<double>(0, y);
						m[y * 4 + 1] = rotationMat.at<double>(1, y);
						m[y * 4 + 2] = rotationMat.at<double>(2, y);
						m[y * 4 + 3] = 0.0;
					}

					cv::Vec3d trans;
					if (!filtered){
						trans = translationvectors[i];
					}
					else
					{
						trans = translationvectors_filtered[i];
					}

					m[12] = trans[0];
					m[13] = trans[1];
					m[14] = trans[2];
					m[15] = 1.0;
				}

				outfile << m[0] << " , " << m[1] << " , " << m[2] << " , " << m[3] << " , ";
				outfile << m[4] << " , " << m[5] << " , " << m[6] << " , " << m[7] << " , ";
				outfile << m[8] << " , " << m[9] << " , " << m[10] << " , " << m[11] << " , ";
				outfile << m[12] << " , " << m[13] << " , " << m[14] << " , " << m[15];
			}
			else
			{
				outfile << "NaN,NaN,NaN,NaN,";
				outfile << "NaN,NaN,NaN,NaN,";
				outfile << "NaN,NaN,NaN,NaN,";
				outfile << "NaN,NaN,NaN,NaN";
			}

			outfile << std::endl;
		}
		outfile.close();
	}
}

bool RigidBody::getTransformationMatrix(int frame, bool filtered, double* trans)
{
	if ((poseComputed[frame] && !filtered) || (poseFiltered[frame] && filtered)){
		cv::Mat rotationMat;
		if (!filtered){
			cv::Rodrigues(rotationvectors[frame], rotationMat);
		}
		else
		{
			cv::Rodrigues(rotationvectors_filtered[frame], rotationMat);
		}
		for (unsigned int y = 0; y < 3; y++)
		{
			trans[y * 4] = rotationMat.at<double>(y, 0);
			trans[y * 4 + 1] = rotationMat.at<double>(y, 1);
			trans[y * 4 + 2] = rotationMat.at<double>(y, 2);
			trans[y * 4 + 3] = 0.0;
		}
		//inverse translation = translation rotated with inverse rotation/transposed rotation
		//R-1 * -t = R^tr * -t

		cv::Vec3d translation;
		if (!filtered){
			translation = translationvectors[frame];
		}
		else
		{
			translation = translationvectors_filtered[frame];
		}
		trans[12] = trans[0] * -translation[0] + trans[4] * -translation[1] + trans[8] * -translation[2];
		trans[13] = trans[1] * -translation[0] + trans[5] * -translation[1] + trans[9] * -translation[2];
		trans[14] = trans[2] * -translation[0] + trans[6] * -translation[1] + trans[10] * -translation[2];
		trans[15] = 1.0;
		return true;
	}
	else
	{
		return false;
	}

}

std::vector <cv::Point2d> RigidBody::projectToImage(Camera * cam, int Frame, bool with_center, bool dummy, bool dummy_frame, bool filtered){
	std::vector <cv::Point3d> points3D_frame;
	cv::Mat rotMatTmp;
	if (filtered){
		cv::Rodrigues(rotationvectors_filtered[Frame], rotMatTmp);
	}
	else
	{
		cv::Rodrigues(rotationvectors[Frame], rotMatTmp);
	}
	if (with_center){
		cv::Mat xmat = cv::Mat(center, true);

		cv::Mat tmp_mat;
		if (filtered){
			tmp_mat = rotMatTmp.t() * ((xmat)-cv::Mat(translationvectors_filtered[Frame]));
		}
		else
		{
			tmp_mat = rotMatTmp.t() * ((xmat)-cv::Mat(translationvectors[Frame]));
		}
		cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
			tmp_mat.at<double>(1, 0),
			tmp_mat.at<double>(2, 0));

		points3D_frame.push_back(pt3d);
	}

	if (!dummy){
		for (unsigned int i = 0; i < points3D.size(); i++){
		cv::Mat xmat = cv::Mat(points3D[i], true);

		cv::Mat tmp_mat;
		if (filtered){
			tmp_mat = rotMatTmp.t() * ((xmat)-cv::Mat(translationvectors_filtered[Frame]));
		}
		else
		{
			tmp_mat = rotMatTmp.t() * ((xmat)-cv::Mat(translationvectors[Frame]));
		}
		cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
			tmp_mat.at<double>(1, 0),
			tmp_mat.at<double>(2, 0));

		points3D_frame.push_back(pt3d);
		}
	}
	else
	{
		if (!dummy_frame){
			for (unsigned int i = 0; i < dummypoints.size(); i++){
				cv::Mat xmat = cv::Mat(dummypoints[i], true);

				cv::Mat tmp_mat;
				if (filtered){
					tmp_mat = rotMatTmp.t() * ((xmat)-cv::Mat(translationvectors_filtered[Frame]));
				}
				else
				{
					tmp_mat = rotMatTmp.t() * ((xmat)-cv::Mat(translationvectors[Frame]));
				}
				cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
					tmp_mat.at<double>(1, 0),
					tmp_mat.at<double>(2, 0));

				points3D_frame.push_back(pt3d);
			}
		}
		else
		{
			for (unsigned int i = 0; i < dummypointsCoords.size(); i++){
				if (dummypointsCoordsSet[i][Frame])
					points3D_frame.push_back(dummypointsCoords[i][Frame]);
			}
		}
	}
	rotMatTmp.release();

	CvMat* object_points = cvCreateMat(points3D_frame.size(), 3, CV_64FC1);
	CvMat* image_points = cvCreateMat(points3D_frame.size(), 2, CV_64FC1);

	// Transfer the points into the correct size matrices
	for (unsigned int i = 0; i <points3D_frame.size(); ++i){
		CV_MAT_ELEM(*object_points, double, i, 0) = points3D_frame[i].x;
		CV_MAT_ELEM(*object_points, double, i, 1) = points3D_frame[i].y;
		CV_MAT_ELEM(*object_points, double, i, 2) = points3D_frame[i].z;
	}

	// initialize camera and distortion initial guess
	CvMat* intrinsic_matrix = cvCreateMat(3, 3, CV_64FC1);
	CvMat* distortion_coeffs = cvCreateMat(5, 1, CV_64FC1);
	for (unsigned int i = 0; i < 3; ++i){
		CV_MAT_ELEM(*intrinsic_matrix, double, i, 0) = cam->getCameraMatrix().at<double>(i, 0);
		CV_MAT_ELEM(*intrinsic_matrix, double, i, 1) = cam->getCameraMatrix().at<double>(i, 1);
		CV_MAT_ELEM(*intrinsic_matrix, double, i, 2) = cam->getCameraMatrix().at<double>(i, 2);

	}
	CV_MAT_ELEM(*intrinsic_matrix, double, 0, 1) = 0;

	for (unsigned int i = 0; i < 5; ++i){
		CV_MAT_ELEM(*distortion_coeffs, double, i, 0) = 0;
	}

	CvMat* r_matrices = cvCreateMat(1, 1, CV_64FC3);
	CvMat* t_matrices = cvCreateMat(1, 1, CV_64FC3);

	for (unsigned int i = 0; i < 3; i++){
		CV_MAT_ELEM(*r_matrices, cv::Vec3d, 0, 0)[i] = cam->getCalibrationImages()[trial->getReferenceCalibrationImage()]->getRotationVector().at<double>(i, 0);
		CV_MAT_ELEM(*t_matrices, cv::Vec3d, 0, 0)[i] = cam->getCalibrationImages()[trial->getReferenceCalibrationImage()]->getTranslationVector().at<double>(i, 0);
	}

	cvProjectPoints2(object_points, r_matrices, t_matrices, intrinsic_matrix, distortion_coeffs, image_points);

	cv::Point2d pt0 = cv::Point2d(CV_MAT_ELEM(*image_points, double, 0, 0), CV_MAT_ELEM(*image_points, double, 0, 1));

	std::vector <cv::Point2d> points2D_frame;
	for (unsigned int i = 0; i < points3D_frame.size(); i++){
		cv::Point2d pt = cv::Point2d(CV_MAT_ELEM(*image_points, double, i, 0), CV_MAT_ELEM(*image_points, double, i, 1));
		points2D_frame.push_back(cam->undistortPoint(pt, false));
	}
	points3D_frame.clear();

	return points2D_frame;
}

void RigidBody::setMissingPoints(int Frame){
	if (poseComputed[Frame]){
		for (int c = 0; c < Project::getInstance()->getCameras().size(); c++){
			std::vector <cv::Point2d> image_points = projectToImage(Project::getInstance()->getCameras()[c], Frame, false);
			for (int i = 0; i < image_points.size(); i++){
				if (trial->getMarkers()[pointsIdx[i]]->getStatus2D()[Frame][c] <= PREDICTED_RIGIDBODY){
					//object->markers[pointsIdx[i]].pointsCam1[Frame].x = image_points[i].x;
					//object->markers[pointsIdx[i]].pointsCam1[Frame].y = image_points[i].y;
					//object->markers[pointsIdx[i]].statusCam1[Frame] = PREDICTED_RIGIDBODY;
				}
			}
		}
		//object->mainwindow->recompute3DCoordinates();
	}
}

int RigidBody::setReferenceFromFile(QString filename)
{
	// setup all possibilities
	QString tmp_names;
	QString tmp_coords;

	std::ifstream fin;
	fin.open(filename.toAscii().data());
	std::istringstream in;
	std::string line;
	littleHelper::safeGetline(fin, line);
	tmp_names = QString::fromStdString(line);
	littleHelper::safeGetline(fin, line);
	tmp_coords = QString::fromStdString(line);
	fin.close();

	QStringList names_list = tmp_names.split(",");
	QStringList coords_list = tmp_coords.split(",");

	if (names_list.size() != pointsIdx.size() * 3 ||
		coords_list.size() != pointsIdx.size() * 3) return -1;


	std::vector <cv::Point3d> points3D_tmp;
	std::vector <QString> referenceNames_tmp;

	for (int i = 0; i < pointsIdx.size(); i++)
	{
		referenceNames_tmp.push_back(names_list.at(3 * i));
		referenceNames_tmp[i].replace("_x", "");
		points3D_tmp.push_back(cv::Point3d(coords_list.at(3 * i).toDouble(), coords_list.at(3 * i + 1).toDouble(), coords_list.at(3 * i + 2).toDouble()));
	}

	std::vector <int> permutations;
	for (int i = 0; i < points3D_tmp.size(); i++)
		permutations.push_back(i);
	
	double error = 1000000.0;
	std::vector <int> best_permutation;
	do {
		std::vector<cv::Point3d> perm_points;
		for (int i = 0; i < permutations.size(); i++)
			perm_points.push_back(points3D_tmp[permutations[i]]);

		double error_tmp = fitAndComputeError(perm_points, points3D);

		if (error_tmp < error)
		{
			error = error_tmp;
			//fprintf(stderr, "Error %lf Perm ", error);
			best_permutation.clear();
			for (int i = 0; i < permutations.size(); i++){
				best_permutation.push_back(permutations[i]);
				//fprintf(stderr, " %d ", best_permutation[i]);
			}
			//fprintf(stderr, "\n");
		}

	} while (std::next_permutation(permutations.begin(), permutations.end()));

	if (error > 100)
	{
		return -2;
	}

	points3D.clear();
	referenceNames.clear();

	for (int i = 0; i < best_permutation.size(); i++)
	{
		points3D.push_back(points3D_tmp[best_permutation[i]]);
		referenceNames.push_back(referenceNames_tmp[best_permutation[i]]);
	}

	
	setReferencesSet(2);

	recomputeTransformations();

	return 1;
}

int RigidBody::isReferencesSet()
{
	return referencesSet;
}

void RigidBody::setReferencesSet(int value)
{
	referencesSet = value;
	updateCenter();
}

bool RigidBody::getOverrideCutoffFrequency()
{
	return overrideCutoffFrequency;
}

void RigidBody::setOverrideCutoffFrequency(bool value)
{
	overrideCutoffFrequency = value;
}

double RigidBody::getCutoffFrequency()
{
	return cutoffFrequency;
}

void RigidBody::setCutoffFrequency(double value)
{
	cutoffFrequency = value;
}

void RigidBody::updateCenter()
{
	center.x = 0.0;
	center.y = 0.0;
	center.z = 0.0;

	if (points3D.size() > 0){
		for (int i = 0; i < points3D.size(); i++)
		{
			center.x += points3D[i].x;
			center.y += points3D[i].y;
			center.z += points3D[i].z;
		}
		center.x /= points3D.size();
		center.y /= points3D.size();
		center.z /= points3D.size();
	}
}

bool RigidBody::getVisible()
{
	return visible;
}

void RigidBody::setVisible(bool value)
{
	visible = value;
}

QColor RigidBody::getColor()
{
	return color;
}

void RigidBody::setColor(QColor value)
{
	color.setRgb(value.red(), value.green(), value.blue());
}

void RigidBody::draw2D(Camera * cam, int frame)
{
	if (visible && poseComputed[frame] && isReferencesSet())
	{
		std::vector<cv::Point2d > points2D_projected = projectToImage(cam, frame, true);

		for (int i = 1; i < points2D_projected.size(); i++){
			double x = points2D_projected[i].x;
			double y = points2D_projected[i].y;

			glBegin(GL_LINES);
			glColor3ub(color.red(),color.green() ,color.blue());
			glVertex2f(points2D_projected[0].x, points2D_projected[0].y);
			glVertex2f(points2D_projected[i].x, points2D_projected[i].y);
			glEnd();
		}

		if (dummyNames.size() > 0){
			points2D_projected.clear();
			points2D_projected = projectToImage(cam, frame, true, true);

			glLineStipple(10, 0xAAAA);
			glEnable(GL_LINE_STIPPLE);
			for (int i = 1; i < points2D_projected.size(); i++){
				double x = points2D_projected[i].x;
				double y = points2D_projected[i].y;

				glBegin(GL_LINES);
				glColor3ub(color.red(), color.green(), color.blue());
				glVertex2f(points2D_projected[0].x, points2D_projected[0].y);
				glVertex2f(points2D_projected[i].x, points2D_projected[i].y);
				glEnd();
			}
			glDisable(GL_LINE_STIPPLE);

			points2D_projected.clear();
			points2D_projected = projectToImage(cam, frame, false, true, true);

			for (int i = 0; i < points2D_projected.size(); i++){
				double x = points2D_projected[i].x;
				double y = points2D_projected[i].y;

				glBegin(GL_LINES);
				glColor3ub(color.red(), color.green(), color.blue());
				glVertex2f(x - 5, y);
				glVertex2f(x + 5, y);
				glVertex2f(x, y - 5);
				glVertex2f(x, y + 5);
				glEnd();
			}
		}
	}
}

void RigidBody::draw3D(int frame)
{
	if (visible && poseComputed[frame] && isReferencesSet())
	{
		glPushMatrix();
		double m[16];
		//inversere Rotation = transposed rotation
		//and opengl requires transposed, so we set R

		cv::Mat rotationMat;
		cv::Rodrigues(rotationvectors[frame], rotationMat);
		for (unsigned int y = 0; y < 3; y++)
		{
			m[y * 4] = rotationMat.at<double>(y, 0);
			m[y * 4 + 1] = rotationMat.at<double>(y, 1);
			m[y * 4 + 2] = rotationMat.at<double>(y, 2);
			m[y * 4 + 3] = 0.0;
		}
		//inverse translation = translation rotated with inverse rotation/transposed rotation
		//R-1 * -t = R^tr * -t
		m[12] = m[0] * -translationvectors[frame][0] + m[4] * -translationvectors[frame][1] + m[8] * -translationvectors[frame][2];
		m[13] = m[1] * -translationvectors[frame][0] + m[5] * -translationvectors[frame][1] + m[9] * -translationvectors[frame][2];
		m[14] = m[2] * -translationvectors[frame][0] + m[6] * -translationvectors[frame][1] + m[10] * -translationvectors[frame][2];
		m[15] = 1.0;
		
		glMultMatrixd(m);

		for (int i = 0; i < points3D.size(); i++)
		{
			glColor3ub(color.red(), color.green(), color.blue());
			glBegin(GL_LINES);
			glVertex3d(center.x, center.y, center.z);
			glVertex3d(points3D[i].x, points3D[i].y, points3D[i].z);
			glEnd();
		}

		if (dummypoints.size() > 0){
			glLineStipple(10, 0xAAAA);
			glEnable(GL_LINE_STIPPLE);
			for (int i = 1; i < dummypoints.size(); i++){
				glBegin(GL_LINES);
				glColor3ub(color.red(), color.green(), color.blue());
				glVertex3d(center.x, center.y, center.z);
				glVertex3d(dummypoints[i].x, dummypoints[i].y, dummypoints[i].z);
				glEnd();
			}
			glDisable(GL_LINE_STIPPLE);
		}

		glPopMatrix();
	}
}

//void RigidBody::setAllPoints(DigitizingObject * object, int Frame){
//	if (poseComputed[Frame]){
//		for (int c = 0; c < object->mainwindow->cams.size(); c++){
//			std::vector <cv::Point2d> image_points = projectToCameraSpace(object->mainwindow->cams[c], Frame, false);
//
//			for (int i = 0; i < image_points.size(); i++){
//				if (c == 0){
//					object->markers[pointsIdx[i]].pointsCam1[Frame].x = image_points[i].x;
//					object->markers[pointsIdx[i]].pointsCam1[Frame].y = image_points[i].y;
//					object->markers[pointsIdx[i]].statusCam1[Frame] = PREDICTED_RIGIDBODY;
//				}
//				else{
//					object->markers[pointsIdx[i]].pointsCam2[Frame].x = image_points[i].x;
//					object->markers[pointsIdx[i]].pointsCam2[Frame].y = image_points[i].y;
//					object->markers[pointsIdx[i]].statusCam2[Frame] = PREDICTED_RIGIDBODY;
//				}
//			}
//		}
//		object->mainwindow->recompute3DCoordinates();
//	}
//}
//
//void RigidBody::set3DPoint(DigitizingObject * object, int Frame, int idx){
//	if (poseComputed[Frame]){
//		for (int c = 0; c < object->mainwindow->cams.size(); c++){
//			std::vector <cv::Point2d> image_points = projectToCameraSpace(object->mainwindow->cams[c], Frame, false);
//
//			for (int i = 0; i < image_points.size(); i++){
//				if (idx == pointsIdx[i]){
//					if (c == 0){
//						object->markers[pointsIdx[i]].pointsCam1[Frame].x = image_points[i].x;
//						object->markers[pointsIdx[i]].pointsCam1[Frame].y = image_points[i].y;
//						object->markers[pointsIdx[i]].statusCam1[Frame] = PREDICTED_RIGIDBODY;
//					}
//					else{
//						object->markers[pointsIdx[i]].pointsCam2[Frame].x = image_points[i].x;
//						object->markers[pointsIdx[i]].pointsCam2[Frame].y = image_points[i].y;
//						object->markers[pointsIdx[i]].statusCam2[Frame] = PREDICTED_RIGIDBODY;
//					}
//				}
//			}
//		}
//		object->mainwindow->recompute3DCoordinates(false);
//	}
//}
//
//void RigidBody::setMissing2DPoints(Camera * cam, DigitizingObject * object, int Frame){
//
//}
//
//markerStatus RigidBody::getStatus(DigitizingObject * object, int Frame){
//	markerStatus status;
//	for (int i = 0; i < pointsIdx.size(); i++){
//		status = MIN_marker(status, object->markers[pointsIdx[i]].statusWorld[Frame]);
//	}
//	return status;
//}
//
//void RigidBody::computeErrors(DigitaizingObject * object, int Frame){
//	errorMean2D[Frame] = 0;
//	int count = 0;
//	for (int i = 0; i < pointsIdx.size(); i++){
//		if (object->markers[pointsIdx[i]].statusWorld[Frame] > UNDEFINED){
//			errorMean2D[Frame] += object->markers[pointsIdx[i]].error2D[Frame];
//			count++;
//		}
//	}
//
//	if (count != 0) errorMean2D[Frame] /= count;
//	errorSd2D[Frame] = 0;
//	for (int i = 0; i < pointsIdx.size(); i++){
//		if (object->markers[pointsIdx[i]].statusWorld[Frame] > UNDEFINED){
//			errorSd2D[Frame] += fabs(object->markers[pointsIdx[i]].error2D[Frame] - errorMean2D[Frame]);
//		}
//	}
//	if (count != 0) errorSd2D[Frame] /= count;
//
//	computePoseBasedOnWorldcoordinates(object, Frame);
//
//	if (poseComputed[Frame]){
//		std::vector <cv::Point3d> points3D_frame;
//		errorMean3D[Frame] = 0;
//		for (int i = 0; i < pointsIdx.size(); i++){
//			if (object->markers[pointsIdx[i]].statusWorld[Frame] > UNDEFINED){
//				cv::Mat xmat = cv::Mat(points3D[i], true);
//				cv::Mat tmp_mat = rotationmatrices[Frame].t() * ((xmat)-translationvectors[Frame]);
//
//				cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
//					tmp_mat.at<double>(0, 1),
//					tmp_mat.at<double>(0, 2));
//
//				cv::Point3d diff = object->markers[pointsIdx[i]].pointsWorld[Frame] - pt3d;
//				object->markers[pointsIdx[i]].error3D[Frame] = cv::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
//				errorMean3D[Frame] += object->markers[pointsIdx[i]].error3D[Frame];
//			}
//		}
//		if (count != 0) errorMean3D[Frame] /= count;
//
//		errorSd3D[Frame] = 0;
//		for (int i = 0; i < pointsIdx.size(); i++){
//			if (object->markers[pointsIdx[i]].statusWorld[Frame] > UNDEFINED){
//				errorSd3D[Frame] += fabs(object->markers[pointsIdx[i]].error3D[Frame] - errorMean3D[Frame]);
//			}
//		}
//		if (count != 0) errorSd3D[Frame] /= count;
//
//		points3D_frame.clear();
//	}
//}