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
///\file RigidBody.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/RigidBody.h"
#include "core/Trial.h"
#include "core/Camera.h"
#include "core/Marker.h"
#include "core/Project.h"
#include "core/Settings.h"
#include "core/UndistortionObject.h"
#include "core/CalibrationImage.h"
#include "core/HelperFunctions.h"
#include "core/RigidBodyObj.h"
#include "processing/ButterworthLowPassFilter.h" //should move this dependency
#include "processing/RigidBodyPoseOptimization.h"
#include "processing/RigidBodyPoseFrom2D.h"

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

RigidBody::RigidBody(int size, Trial* _trial)
{
	expanded = false;
	initialised = false;
	trial = _trial;
	referencesSet = false;
	color.setRgb(255, 0, 0);
	visible = false;
	meshmodel = NULL;
	m_drawMeshModel = false;
	cutoffFrequency = 0;
	overrideCutoffFrequency = false;
	hasOptimizedCoordinates = false;
	meshScale = 1.0;
	init(size);
}

void RigidBody::copyData(RigidBody* rb)
{
	setDescription(rb->getDescription());
	clearPointIdx();

	for (unsigned int j = 0; j < rb->getPointsIdx().size(); j++)
	{
		addPointIdx(rb->getPointsIdx()[j], false);
	}

	if (rb->isReferencesSet())
	{
		setReferencesSet(rb->isReferencesSet());

		for (unsigned int i = 0; i < rb->referenceNames.size(); i++)
		{
			referenceNames[i] = rb->referenceNames[i];
		}
		for (unsigned int i = 0; i < rb->points3D.size(); i++)
		{
			points3D[i] = rb->points3D[i];
			points3D_original[i] = rb->points3D_original[i];
		}

		updateCenter();
	}

	bool modelLoaded = addMeshModel(rb->getMeshModelname());
	if (modelLoaded) {
		m_drawMeshModel = rb->getDrawMeshModel();
		meshScale = rb->getMeshScale();
	}

	dummyNames.clear();
	dummyRBIndex.clear();
	dummypointsCoords.clear();
	dummypointsCoordsSet.clear();
	dummypoints.clear();
	dummypoints2.clear();

	for (unsigned int i = 0; i < rb->dummyRBIndex.size(); i++)
	{
		if (dummyRBIndex[i] >= 0)
		{
			dummyRBIndex.push_back(rb->dummyRBIndex[i]);
			dummyNames.push_back(rb->dummyNames[i]);
			dummypoints.push_back(rb->dummypoints[i]);
			dummypoints2.push_back(rb->dummypoints2[i]);
		}
	}

	hasOptimizedCoordinates = rb->hasOptimizedCoordinates;
	overrideCutoffFrequency = rb->getOverrideCutoffFrequency();
	cutoffFrequency = rb->getOverrideCutoffFrequency();
	color = rb->getColor();
	meshScale = rb->getMeshScale();
	expanded= rb->isExpanded();
	color = rb->getColor();
	visible = rb->getVisible();
	initialised = rb->initialised;
}

RigidBody::~RigidBody()
{
	pointsIdx.clear();
	points3D.clear();
	referenceNames.clear();

	if (meshmodel != NULL)
	{
		delete meshmodel;
	}

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

std::vector<int>& RigidBody::getPoseComputed()
{
	return poseComputed;
}

const std::vector<QString>& RigidBody::getReferenceNames()
{
	return referenceNames;
}

const std::vector<cv::Point3d>& RigidBody::getReferencePoints()
{
	return points3D;
}

const std::vector<cv::Point3d>& RigidBody::getReference3DPointOriginal()
{
	return points3D_original;
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

void RigidBody::addPointIdx(int idx, bool recompute)
{
	pointsIdx.push_back(idx);
	points3D.push_back(cv::Point3d(0, 0, 0));
	points3D_original.push_back(cv::Point3d(0, 0, 0));
	referenceNames.push_back("");
	resetReferences();
	if (recompute)
	{
		recomputeTransformations();
		filterTransformations();
	}
}

void RigidBody::removePointIdx(int idx)
{
	int pos = std::find(pointsIdx.begin(), pointsIdx.end(), idx) - pointsIdx.begin();
	if (pos < pointsIdx.size()){
		points3D.erase(std::remove(points3D.begin(), points3D.end(), points3D[pos]), points3D.end());
		points3D_original.erase(std::remove(points3D_original.begin(), points3D_original.end(), points3D_original[pos]), points3D_original.end());
		referenceNames.erase(std::remove(referenceNames.begin(), referenceNames.end(), referenceNames[pos]), referenceNames.end());
		pointsIdx.erase(std::remove(pointsIdx.begin(), pointsIdx.end(), idx), pointsIdx.end());
		//resetReferences();
		recomputeTransformations();
		filterTransformations();
	}
}

void RigidBody::updatePointIdx(int idx)
{
	bool requiresUpdate = false;
	for (std::vector<int>::iterator it = pointsIdx.begin(); it < pointsIdx.end(); ++it)
	{
		if (*it > idx)
		{
			*it = *it - 1;
			requiresUpdate = true;
		}
	}
	if (requiresUpdate){
		recomputeTransformations();
		filterTransformations();
	}
}

void RigidBody::resetReferences()
{
	for (unsigned int i = 0; i < referenceNames.size(); i++)
	{
		referenceNames[i] = "";
		points3D[i].x = 0;
		points3D[i].y = 0;
		points3D[i].z = 0;
		points3D_original[i].x = 0;
		points3D_original[i].y = 0;
		points3D_original[i].z = 0;
	}
	setReferencesSet(0);
	initialised = false;
	hasOptimizedCoordinates = false;
	setReferenceMarkerReferences();
	recomputeTransformations();
	filterTransformations();
}

bool RigidBody::allReferenceMarkerReferencesSet()
{
	bool allset = true;
	for (unsigned int i = 0; i < pointsIdx.size(); i++)
	{
		if (!trial->getMarkers()[pointsIdx[i]]->Reference3DPointSet())
		{
			allset = false;
		}
	}
	return allset;
}

bool RigidBody::transformPoint(cv::Point3d in, cv::Point3d& out, int frame, bool filtered)
{
	if (!poseComputed[frame])
	{
		computePose(frame); 
	}

	if (poseComputed[frame] || (filtered && poseFiltered[frame]))
	{
		cv::Mat rotationMatrix;
		cv::Rodrigues(getRotationVector(filtered)[frame], rotationMatrix);

		cv::Mat xmat = cv::Mat(in, true);
		cv::Mat tmp_mat;

		tmp_mat = rotationMatrix.t() * ((xmat) - cv::Mat(getTranslationVector(filtered)[frame]));

		out.x = tmp_mat.at<double>(0, 0) ,
			out.y = tmp_mat.at<double>(1, 0) ,
			out.z = tmp_mat.at<double>(2, 0);
		return true;
	}
	return false;
}

bool RigidBody::addMeshModel(QString filename)
{
	if (meshmodel != NULL)
	{
		delete meshmodel;
	}

	RigidBodyObj* tmp = new RigidBodyObj(filename, this);

	if (tmp->vboSet())
	{
		meshmodel = tmp;
		return true;
	}
	else
	{
		delete tmp;
		return false;
	}
}

bool RigidBody::hasMeshModel()
{
	return meshmodel != NULL;
}

QString RigidBody::getMeshModelname()
{
	if (meshmodel == NULL) 
		return "";

	return meshmodel->getFilename();
}

bool RigidBody::getDrawMeshModel()
{
	return m_drawMeshModel;
}

void RigidBody::setDrawMeshModel(bool value)
{
	m_drawMeshModel = value;
}

void RigidBody::drawMesh(int frame)
{
	meshmodel->render(frame);
}

double RigidBody::getMeshScale()
{
	return meshScale;
}

void RigidBody::setMeshScale(double value)
{
	meshScale = value;
}

int RigidBody::getFirstTrackedFrame()
{
	for (int i = 0; i < poseComputed.size(); i++)
	{
		if (poseComputed[i] >= 0) return i + 1;
	}
	return -1;
}

int RigidBody::getLastTrackedFrame()
{
	for (int i = poseComputed.size() - 1; i >= 0 ; i--)
	{
		if (poseComputed[i] >= 0) return i + 1;
	}
	return -1;
}

int RigidBody::getFramesTracked()
{
	int count = 0;
	for (int i = 0; i < poseComputed.size(); i++)
	{
		if (poseComputed[i] >= 0) count++;
	}
	return count;
}

void RigidBody::getMarkerToMarkerSD(double & sd_all, int & count_all, int start, int end)
{
	if (start == -1) start = 0;
	
	sd_all = 0;
	count_all = 0;
	for (unsigned i = 0; i < pointsIdx.size(); i++)
	{
		if (end == -1) end = trial->getMarkers()[pointsIdx[i]]->getStatus3D().size();

		for (unsigned j = i + 1; j < pointsIdx.size(); j++)
		{
			double mean = 0;
			int count = 0;
			for (int f = start; f < end; f++)
			{
				if (trial->getMarkers()[pointsIdx[i]]->getStatus3D()[f] > UNDEFINED && trial->getMarkers()[pointsIdx[j]]->getStatus3D()[f] > UNDEFINED)
				{
					cv::Point3d diff = trial->getMarkers()[pointsIdx[i]]->getPoints3D()[f] - trial->getMarkers()[pointsIdx[j]]->getPoints3D()[f];
					mean += cv::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
					count++;
				}
			}
			if (count > 0) mean = mean / count;

			for (int f = start; f < end; f++)
			{
				if (trial->getMarkers()[pointsIdx[i]]->getStatus3D()[f] > UNDEFINED && trial->getMarkers()[pointsIdx[j]]->getStatus3D()[f] > UNDEFINED)
				{
					cv::Point3d diff = trial->getMarkers()[pointsIdx[i]]->getPoints3D()[f] - trial->getMarkers()[pointsIdx[j]]->getPoints3D()[f];
					double err = cv::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
					sd_all += pow(err - mean, 2);
				}
			}		
			count_all += count;
		}
	}

	if (count_all > 1)
		sd_all = sqrt(sd_all / (count_all -1));
}

double RigidBody::getError3D(bool filtered, int start, int end)
{
	if (start == -1) start = 0;
	if (end == -1) end = poseComputed.size();

	double error3D = 0.0;
	int count = 0;

	for (int i = start; i < end; i++)
	{
		if (filtered)
		{
			if (poseFiltered[i])
			{
				count++;
				error3D += errorMean3D_filtered[i];
			}
		}
		else{
			if (poseComputed[i])
			{
				count++;
				error3D += errorMean3D[i];
			}
		}
	}

	if (count != 0) error3D /= count;

	return error3D;
}

int RigidBody::addDummyPointsForOptimization(std::vector<cv::Point2d>& Pts2D, std::vector<cv::Point3d>& Pts3D, std::vector<int>& cameraIdx, int Frame)
{
	int count = 0;
	for (unsigned int i = 0; i < dummypoints.size(); i++)
	{
		cv::Point3d src;
		cv::Point3d dst;
		if (dummyRBIndex[i] >= 0)
		{
			cv::Point3d dummy_tmp;
			if (trial->getRigidBodies()[dummyRBIndex[i]]->transformPoint(dummypoints2[i], dummy_tmp, Frame))
			{
				src = cv::Point3f(dummy_tmp.x
					, dummy_tmp.y
					, dummy_tmp.z);
				dst = cv::Point3f(dummypoints[i].x
					, dummypoints[i].y
					, dummypoints[i].z);
			}
		}
		else if (dummypointsCoordsSet[i][Frame])
		{
			src  = cv::Point3f(dummypointsCoords[i][Frame].x
				, dummypointsCoords[i][Frame].y
				, dummypointsCoords[i][Frame].z);
			dst = cv::Point3f(dummypoints[i].x
				, dummypoints[i].y
				, dummypoints[i].z);
		}

		for (int c = 0; c < Project::getInstance()->getCameras().size(); c++)
		{
			Camera* cam = Project::getInstance()->getCameras()[c];
			if (cam->isCalibrated())
			{
				cv::Mat projMatrs = cam->getProjectionMatrix(trial->getReferenceCalibrationImage());
				cv::Mat pt;
				pt.create(4, 1, CV_64F);
				pt.at<double>(0, 0) = src.x;
				pt.at<double>(1, 0) = src.y;
				pt.at<double>(2, 0) = src.z;
				pt.at<double>(3, 0) = 1;
				cv::Mat pt_out = projMatrs * pt;
				double z = pt_out.at<double>(2, 0);
				
				if (z != 0.0)
				{
					cv::Point2d pt_trans;
					pt_trans.x = pt_out.at<double>(0, 0) / z;
					pt_trans.y = pt_out.at<double>(1, 0) / z;

					Pts2D.push_back(pt_trans);
					Pts3D.push_back(dst);
					cameraIdx.push_back(c);
					count++;
				}			
			}
		}
	}
	return count;
}

void RigidBody::setReferenceMarkerReferences()
{
	if (allReferenceMarkerReferencesSet())
	{
		for (unsigned int i = 0; i < pointsIdx.size(); i++)
		{
			points3D[i].x = trial->getMarkers()[pointsIdx[i]]->getReference3DPoint().x;
			points3D[i].y = trial->getMarkers()[pointsIdx[i]]->getReference3DPoint().y;
			points3D[i].z = trial->getMarkers()[pointsIdx[i]]->getReference3DPoint().z;
			points3D_original[i].x = trial->getMarkers()[pointsIdx[i]]->getReference3DPoint().x;
			points3D_original[i].y = trial->getMarkers()[pointsIdx[i]]->getReference3DPoint().y;
			points3D_original[i].z = trial->getMarkers()[pointsIdx[i]]->getReference3DPoint().z;
			referenceNames[i] = trial->getMarkers()[pointsIdx[i]]->getDescription() + " - FromMarker";
		}
		initialised = true;
		setReferencesSet(1);
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

void RigidBody::clear()
{
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
	dummypoints2.clear();
	dummyRBIndex.clear();
	dummypointsCoords.clear();
	dummypointsCoordsSet.clear();
}

const std::vector<QString>& RigidBody::getDummyNames()
{
	return dummyNames;
}

cv::Point3f RigidBody::getDummyCoordinates(int id, int frame)
{
	if (dummyRBIndex[id] >= 0)
	{
		cv::Point3d dummy_tmp;
		if (trial->getRigidBodies()[dummyRBIndex[id]]->transformPoint(dummypoints2[id], dummy_tmp, frame))
		{
			return dummy_tmp;
		}
	}
	else if (dummypointsCoordsSet[id][frame])
	{
		return  cv::Point3f(dummypointsCoords[id][frame].x
			, dummypointsCoords[id][frame].y
			, dummypointsCoords[id][frame].z);
	}

	return cv::Point3f(0, 0, 0);
}

void RigidBody::init(int size)
{
	clear();

	rotationvectors.resize(size,cv::Vec3d());
	translationvectors.resize(size, cv::Vec3d());
	rotationvectors_filtered.resize(size, cv::Vec3d());
	translationvectors_filtered.resize(size, cv::Vec3d());

	poseComputed.resize(size, 0);
	poseFiltered.resize(size, 0);

	errorMean2D.resize(size, 0);
	errorSd2D.resize(size, 0);
	errorMean3D.resize(size, 0);
	errorSd3D.resize(size, 0);

	errorMean2D_filtered.resize(size, 0);
	errorSd2D_filtered.resize(size, 0);
	errorMean3D_filtered.resize(size, 0);
	errorSd3D_filtered.resize(size, 0);
}

void RigidBody::computeCoordinateSystemAverage()
{
	if (!isReferencesSet())
	{
		std::vector<cv::Point3d> points3D_mean;

		int count = 0;
		for (unsigned int f = 0; f < poseComputed.size(); f++)
		{
			computeCoordinateSystem(f);
			computePose(f);
			if (poseComputed[f])
			{
				bool useFrame = true;
				for (unsigned int i = 0; i < pointsIdx.size(); i++)
				{
					if (trial->getMarkers()[pointsIdx[i]]->getStatus3D()[f] <= UNDEFINED)
					{
						useFrame = false;
					}
				}
				if (useFrame)
				{
					count++;
					for (unsigned int i = 0; i < pointsIdx.size(); i++)
					{
						cv::Point3d pt = trial->getMarkers()[pointsIdx[i]]->getPoints3D()[f];

						cv::Mat xmat = cv::Mat(pt, true);

						cv::Mat rotMattmp;
						cv::Rodrigues(rotationvectors[f], rotMattmp);
						cv::Mat tmp_mat = rotMattmp * (xmat) + cv::Mat(translationvectors[f]);

						pt.x = tmp_mat.at<double>(0, 0);
						pt.y = tmp_mat.at<double>(1, 0);
						pt.z = tmp_mat.at<double>(2, 0);

						if (count == 1)
						{
							points3D_mean.push_back(pt);
						}
						else
						{
							points3D_mean[i] += pt;
						}
						rotMattmp.release();
					}
				}
			}
		}

		if (count == 0) return;

		for (unsigned int i = 0; i < points3D_mean.size(); i++)
		{
			points3D_mean[i].x /= count;
			points3D_mean[i].y /= count;
			points3D_mean[i].z /= count;
		}

		cv::Point3d center = cv::Point3d(0, 0, 0);
		for (unsigned int i = 0; i < points3D_mean.size(); i++)
		{
			center += points3D_mean[i];
		}

		if (points3D_mean.size() > 0)
		{
			center.x /= pointsIdx.size();
			center.y /= pointsIdx.size();
			center.z /= pointsIdx.size();
		}

		if (points3D_mean.size() >= 2)
		{
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
			
			for (unsigned int i = 0; i < points3D_mean.size(); i++)
			{
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
		else
		{
			points3D.clear();
			for (unsigned int i = 0; i < points3D_mean.size(); i++)
			{
				cv::Point3d pt = points3D_mean[i] - center;
				points3D.push_back(pt);
			}
		}
		points3D_mean.clear();

		for (unsigned int f = 0; f < poseComputed.size(); f++)
		{
			computePose(f);
		}
	}
}

void RigidBody::setOptimized(bool optimized)
{
	hasOptimizedCoordinates = false;
	points3D = points3D_original;

	if (!isReferencesSet())
	{
		return;
	}

	if (optimized){
		std::vector <int> goodFrames;
		for (unsigned int f = 0; f < poseComputed.size(); f++)
		{
			if (poseComputed[f])
			{
				bool allfound = true;
				for (unsigned int i = 0; i < pointsIdx.size(); i++)
				{
					if (trial->getMarkers()[pointsIdx[i]]->getStatus3D()[f] <= INTERPOLATED) allfound = false;
				}
				if (allfound){
					goodFrames.push_back(f);
					computePose(f);
				}
			}
		}

		if (goodFrames.size() > 0)
		{
			std::vector < std::vector <cv::Point3d> > pts_aligned;
			for (std::vector<int>::iterator f = goodFrames.begin(); f < goodFrames.end(); ++f){
				std::vector<cv::Point3d> pts_aligned_frame;
				cv::Mat rotationMatrix;
				cv::Rodrigues(rotationvectors[*f], rotationMatrix);

				for (unsigned int i = 0; i < pointsIdx.size(); i++)
				{
					cv::Mat xmat = cv::Mat(trial->getMarkers()[pointsIdx[i]]->getPoints3D()[*f], true);
					cv::Mat tmp_mat;

					tmp_mat = rotationMatrix * (xmat)+cv::Mat(translationvectors[*f]);

					cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
						tmp_mat.at<double>(1, 0),
						tmp_mat.at<double>(2, 0));
					pts_aligned_frame.push_back(pt3d);
				}
				pts_aligned.push_back(pts_aligned_frame);
			}
			points3D.clear();
			for (int pt_idx = 0; pt_idx < pts_aligned[0].size(); pt_idx++)
			{
				points3D.push_back(cv::Point3d(0, 0, 0));

				for (int f = 0; f < pts_aligned.size(); f++)
				{
					points3D[pt_idx] += pts_aligned[f][pt_idx];
				}
				points3D[pt_idx].x /= pts_aligned.size();
				points3D[pt_idx].y /= pts_aligned.size();
				points3D[pt_idx].z /= pts_aligned.size();
			}
			hasOptimizedCoordinates = true;
		}
	}
}

void RigidBody::computeCoordinateSystem(int Frame)
{
	if (!isReferencesSet())
	{
		for (unsigned int i = 0; i < pointsIdx.size(); i++)
		{
			if (trial->getMarkers()[pointsIdx[i]]->getStatus3D()[Frame] <= UNDEFINED) return;
		}
		points3D.clear();
		cv::Point3d center = cv::Point3d(0, 0, 0);
		for (unsigned int i = 0; i < pointsIdx.size(); i++)
		{
			center += trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame];
		}

		if (pointsIdx.size() > 0)
		{
			center.x /= pointsIdx.size();
			center.y /= pointsIdx.size();
			center.z /= pointsIdx.size();
		}

		if (pointsIdx.size() >= 2)
		{
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

			for (unsigned int i = 0; i < pointsIdx.size(); i++)
			{
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
		else
		{
			for (unsigned int i = 0; i < pointsIdx.size(); i++)
			{
				cv::Point3d pt = trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame] - center;
				points3D.push_back(pt);
			}
		}
	}
}


void RigidBody::computePose(int Frame)
{
	while (Frame >= (int) poseComputed.size()) addFrame();

	poseComputed[Frame] = 0;
	bool success = false;
	if (Project::getInstance()->getCalibration() == NO_CALIBRATION)
		return;
	
	if (initialised)
	{
		std::vector<cv::Point3d> src;
		std::vector<cv::Point3d> dst;
		cv::Mat out;
		cv::Mat inliers;

		for (unsigned int i = 0; i < pointsIdx.size(); i++)
		{
			if (trial->getMarkers()[pointsIdx[i]]->getStatus3D()[Frame] > UNDEFINED)
			{
				src.push_back(cv::Point3f(trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame].x
				                         ,trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame].y
				                         ,trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame].z));
				dst.push_back(cv::Point3f(points3D[i].x
				                         ,points3D[i].y
				                         ,points3D[i].z));
			}
		}

		for (unsigned int i = 0; i < dummypoints.size(); i++)
		{
			if (dummyRBIndex[i] >= 0)
			{
				cv::Point3d dummy_tmp;
				if (trial->getRigidBodies()[dummyRBIndex[i]]->transformPoint(dummypoints2[i], dummy_tmp, Frame))
				{
					src.push_back(cv::Point3f(dummy_tmp.x
					                         ,dummy_tmp.y
					                         ,dummy_tmp.z));
					dst.push_back(cv::Point3f(dummypoints[i].x
					                         ,dummypoints[i].y
					                         ,dummypoints[i].z));
				}
			}
			else if (dummypointsCoordsSet[i][Frame])
			{
				src.push_back(cv::Point3f(dummypointsCoords[i][Frame].x
				                         ,dummypointsCoords[i][Frame].y
				                         ,dummypointsCoords[i][Frame].z));
				dst.push_back(cv::Point3f(dummypoints[i].x
				                         ,dummypoints[i].y
				                         ,dummypoints[i].z));
			}
		}

		if (!Settings::getInstance()->getBoolSetting("DisableRBComputeAdvanced") && dst.size()<3 && dst.size() >= 1)
		{
			//try to find more points for initialisation
			RigidBodyPoseFrom2D * poseFrom2d = new RigidBodyPoseFrom2D(this, Frame);
			poseFrom2d->findFrom2Dwith3D(src, dst);
			delete poseFrom2d;
		}

		if (dst.size() >= 3)
		{
			std::vector<std::vector<double> > y, x;
			std::vector<std::vector<double> > Y, X;
			std::vector<double> vnl_tmp(3), yg(3, 0), xg(3, 0);
			std::vector<double> tmp;
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
			cv::Rodrigues(rotMatTmp, rotationvectors[Frame]);

			cv::Mat xgmat = cv::Mat(xg, true);
			cv::Mat ygmat = cv::Mat(yg, true);
			cv::Mat t = ygmat - rotMatTmp * xgmat;
			translationvectors[Frame] = cv::Vec3d(t);

			success = 1;
		}
		else if (!Settings::getInstance()->getBoolSetting("DisableRBComputeAdvanced"))
		{
			int goodCamera = -1;
			int bestCameracount = 4;
			for (int c = 0; c < Project::getInstance()->getCameras().size(); c++)
			{
				int count = 0;
				for (int p = 0; p < getPointsIdx().size(); p++){
					if (getTrial()->getMarkers()[getPointsIdx()[p]]->getStatus2D()[c][Frame] > UNDEFINED)
					{
						count++;
					}
				}
				if (count >= bestCameracount)
				{
					goodCamera = c;
				}
			}
			if (goodCamera >= 0)
			{
				cv::Vec3d rotationVec_tmp;
				cv::Vec3d translationVec_tmp;
				std::vector<cv::Point3f> object_points;
				std::vector<cv::Point2f> image_points;
				cv::Mat distortion_coeffs = cv::Mat::zeros(8, 1, CV_64F);
				for (int p = 0; p < getPointsIdx().size(); p++){
					if (getTrial()->getMarkers()[getPointsIdx()[p]]->getStatus2D()[goodCamera][Frame] > UNDEFINED)
					{
						object_points.push_back(points3D[p]);
						image_points.push_back(Project::getInstance()->getCameras()[goodCamera]->undistortPoint(getTrial()->getMarkers()[getPointsIdx()[p]]->getPoints2D()[goodCamera][Frame], true));
					}
				}

				
				cv::solvePnP(object_points, image_points, Project::getInstance()->getCameras()[goodCamera]->getCameraMatrix(), distortion_coeffs, rotationVec_tmp, translationVec_tmp, false, cv::SOLVEPNP_EPNP);
				

				cv::Mat trans1;
				trans1.create(4, 4, CV_64FC1);
				cv::Mat rot1;
				cv::Rodrigues(rotationVec_tmp, rot1);

				for (unsigned int i = 0; i < 3; ++i)
				{
					for (unsigned int j = 0; j < 3; ++j)
					{
						trans1.at<double>(i, j) = rot1.at<double>(i, j);
					}
				}
				//set t
				for (unsigned int j = 0; j < 3; ++j)
				{
					trans1.at<double>(j, 3) = translationVec_tmp[j];
				}

				for (unsigned int j = 0; j < 3; ++j)
				{
					trans1.at<double>(3, j) = 0;
				}
				trans1.at<double>(3, 3) = 1;

				cv::Mat out = Project::getInstance()->getCameras()[goodCamera]->getCalibrationImages()[trial->getReferenceCalibrationImage()]->getTransformationMatrix() * trans1;

				out = out.inv();

				for (unsigned int i = 0; i < 3; ++i)
				{
					for (unsigned int j = 0; j < 3; ++j)
					{
						rot1.at<double>(i, j) = out.at<double>(i, j);
					}
				}
				cv::Rodrigues(rot1, rotationvectors[Frame]);

				for (unsigned int j = 0; j < 3; ++j)
				{
					translationvectors[Frame][j] = out.at<double>(j, 3);
				}
				success = 1;
			}
		}

		if (Settings::getInstance()->getBoolSetting("OptimizeRigidBody") && success)
		{
			RigidBodyPoseOptimization * opt = new RigidBodyPoseOptimization(this, Frame);
			opt->optimizeRigidBodySetup();
			delete opt;
		}
	}
	poseComputed[Frame] = success;
	updateError(Frame);
}

void RigidBody::updateError(int Frame, bool filtered)
{
	double eMean2D = 0;
	double eMean3D = 0;
	double eSD2D = 0;
	double eSD3D = 0;

	int count;

	if ((points3D.size() > 0) && ((poseComputed[Frame] && !filtered) || (filtered && poseFiltered[Frame])))
	{
		count = 0;

		std::vector<double> dist;
		for (unsigned int c = 0; c < Project::getInstance()->getCameras().size(); c++)
		{
			std::vector<cv::Point2d> image_points = projectToImage(Project::getInstance()->getCameras()[c], Frame, false, false, false, filtered);

			for (unsigned int i = 0; i < pointsIdx.size(); i++)
			{
				if (trial->getMarkers()[pointsIdx[i]]->getStatus2D()[c][Frame] > UNDEFINED)
				{
					cv::Point2d diff = trial->getMarkers()[pointsIdx[i]]->getPoints2D()[c][Frame] - image_points[i];
					dist.push_back(cv::sqrt(diff.x * diff.x + diff.y * diff.y));
					eMean2D += dist[count];
					count++;
				}
			}

			image_points.clear();
		}

		if (count != 0) eMean2D /= count;

		for (unsigned int i = 0; i < dist.size(); i++)
		{
			eSD2D += pow(dist[i] - eMean2D, 2);
		}
		if (count != 0) eSD2D = sqrt(eSD2D / (count - 1));;

		count = 0;
		cv::Mat rotationMatrix;
		if (filtered)
		{
			cv::Rodrigues(rotationvectors_filtered[Frame], rotationMatrix);
		}
		else
		{
			cv::Rodrigues(rotationvectors[Frame], rotationMatrix);
		}
		dist.clear();

		for (unsigned int i = 0; i < points3D.size(); i++)
		{
			cv::Mat xmat = cv::Mat(points3D[i], true);
			cv::Mat tmp_mat;

			if (filtered)
			{
				tmp_mat = rotationMatrix.t() * ((xmat) - cv::Mat(translationvectors_filtered[Frame]));
			}
			else
			{
				tmp_mat = rotationMatrix.t() * ((xmat) - cv::Mat(translationvectors[Frame]));
			}

			if (trial->getMarkers()[pointsIdx[i]]->getStatus3D()[Frame] > 0)
			{
				cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
				                 tmp_mat.at<double>(1, 0),
				                 tmp_mat.at<double>(2, 0));

				cv::Point3d diff = trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame] - pt3d;
				dist.push_back(cv::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z));
				eMean3D += dist[count];
				count++;
			}
		}
		if (count != 0) eMean3D /= count;

		for (unsigned int i = 0; i < dist.size(); i++)
		{
			eSD3D += pow(dist[i] - eMean3D, 2);
		}
		if (count != 0) eSD3D = sqrt(eSD3D / (count - 1));;

		dist.clear();
		rotationMatrix.release();
	}

	if (!filtered)
	{
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

double RigidBody::fitAndComputeError(std::vector<cv::Point3d> src, std::vector<cv::Point3d> dst)
{
	double errorMean = 0;

	//for (unsigned int i = 0; i < src.size(); i++)
	//{
	//	fprintf(stderr, "%d %lf %lf %lf - %lf %lf %lf\n", i, dst[i].x, dst[i].y, dst[i].z, src[i].x, src[i].y, src[i].z);
	//}

	if (src.size() >= 3)
	{
		std::vector<std::vector<double> > y, x;
		std::vector<std::vector<double> > Y, X;
		std::vector<double> vnl_tmp(3), yg(3, 0), xg(3, 0);
		std::vector<double> tmp;
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

		cv::Mat rotMat;
		rotMat = U * Sd * VT;

		cv::Mat xgmat = cv::Mat(xg, true);
		cv::Mat ygmat = cv::Mat(yg, true);

		cv::Mat transMat;
		transMat = ygmat - rotMat * xgmat;

		std::vector<cv::Point3d> points3D_frame;

		for (unsigned int i = 0; i < dst.size(); i++)
		{
			cv::Mat xmat = cv::Mat(dst[i], true);
			cv::Mat tmp_mat = rotMat.t() * ((xmat) - transMat);

			cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
			                 tmp_mat.at<double>(1, 0),
			                 tmp_mat.at<double>(2, 0));

			cv::Point3d diff = src[i] - pt3d;
			errorMean += cv::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
		}

		errorMean /= dst.size();
	}
	initialised = true;

	return errorMean;
}

void RigidBody::save(QString filename_referenceNames, QString filename_points3D)
{
	std::ofstream outfile_Names(filename_referenceNames.toStdString());
	outfile_Names.precision(12);
	for (unsigned int i = 0; i < referenceNames.size(); i++)
	{
		outfile_Names << referenceNames[i].toStdString() << std::endl;
	}
	outfile_Names.close();
	std::ofstream outfile_Points(filename_points3D.toStdString());
	outfile_Points.precision(12);
	for (unsigned int j = 0; j < points3D.size(); j++)
	{
		if (hasOptimizedCoordinates){
			outfile_Points << points3D_original[j].x << "," << points3D_original[j].y << "," << points3D_original[j].z << std::endl;
			//std::cerr << points3D_original[j].x << "," << points3D_original[j].y << "," << points3D_original[j].z << std::endl;
		} 
		else
		{
			outfile_Points << points3D[j].x << "," << points3D[j].y << "," << points3D[j].z << std::endl;
		}
	}
	outfile_Points.close();
}

void RigidBody::saveOptimized(QString filename_points3DOptimized, bool withHeader)
{
	std::ofstream outfile_Points(filename_points3DOptimized.toStdString());
	outfile_Points.precision(12);
	if (withHeader)
	{
		for (unsigned int j = 0; j < points3D.size(); j++)
		{
			outfile_Points << referenceNames[j].toStdString() << "_x," << referenceNames[j].toStdString() << "_y," << referenceNames[j].toStdString() << "_z";
			if (j == points3D.size() - 1)
			{
				outfile_Points << std::endl;
			} 
			else
			{
				outfile_Points << ",";
			}
		}

		for (unsigned int j = 0; j < points3D.size(); j++)
		{
			outfile_Points << points3D[j].x << "," << points3D[j].y << "," << points3D[j].z;
			if (j == points3D.size() - 1)
			{
				outfile_Points << std::endl;
			}
			else
			{
				outfile_Points << ",";
			}
		}
	}
	else{
		for (unsigned int j = 0; j < points3D.size(); j++)
		{
			outfile_Points << points3D[j].x << "," << points3D[j].y << "," << points3D[j].z << std::endl;
		}
	}
	outfile_Points.close();
}

void RigidBody::load(QString filename_referenceNames, QString filename_points3D)
{
	std::ifstream fin;
	std::istringstream in;
	std::string line;

	hasOptimizedCoordinates = false;

	fin.open(filename_points3D.toStdString());
	points3D.clear();
	points3D_original.clear();
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		QString tmp_coords = QString::fromStdString(line);
		QStringList coords_list = tmp_coords.split(",");
		if (coords_list.size() == 3)
		{
			points3D.push_back(cv::Point3d(coords_list.at(0).toDouble(), coords_list.at(1).toDouble(), coords_list.at(2).toDouble()));
			points3D_original.push_back(cv::Point3d(coords_list.at(0).toDouble(), coords_list.at(1).toDouble(), coords_list.at(2).toDouble()));
		}
	}
	fin.close();

	fin.open(filename_referenceNames.toStdString());
	referenceNames.clear();
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		if (!line.empty())
		{
			QString tmp_Name = QString::fromStdString(line);
			referenceNames.push_back(tmp_Name);
		}
	}
	fin.close();

	setReferencesSet(2);
	initialised = true;
}

void RigidBody::loadOptimized(QString filename_points3DOptimized)
{
	std::ifstream fin;
	std::istringstream in;
	std::string line;

	hasOptimizedCoordinates = true;

	for (int i = 0; i < points3D.size(); i++)
	{
		points3D_original[i] = points3D[i];
	}

	fin.open(filename_points3DOptimized.toStdString());
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
	if (idx.size() <= 12)
	{
		for (unsigned int i = 0; i < idx.size(); i++)
		{
			translationvectors_filtered[idx[i]][0] = translationvectors[idx[i]][0];
			translationvectors_filtered[idx[i]][1] = translationvectors[idx[i]][1];
			translationvectors_filtered[idx[i]][2] = translationvectors[idx[i]][2];

			rotationvectors_filtered[idx[i]][0] = rotationvectors[idx[i]][0];
			rotationvectors_filtered[idx[i]][1] = rotationvectors[idx[i]][1];
			rotationvectors_filtered[idx[i]][2] = rotationvectors[idx[i]][2];
			poseFiltered[idx[i]] = 1;
		}
	}
	else
	{
		std::vector<double> t1;
		std::vector<double> t2;
		std::vector<double> t3;
		std::vector<double> r1;
		std::vector<double> r2;
		std::vector<double> r3;
		std::vector<double> r4;

		for (unsigned int i = 0; i < idx.size(); i++)
		{
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

		ButterworthLowPassFilter* filter = new ButterworthLowPassFilter(4, cutoff, trial->getRecordingSpeed());

		filter->filter(t1, t1_out);
		filter->filter(t2, t2_out);
		filter->filter(t3, t3_out);
		filter->filter(r1, r1_out);
		filter->filter(r2, r2_out);
		filter->filter(r3, r3_out);

		for (unsigned int i = 0; i < idx.size(); i++)
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
	if (Settings::getInstance()->getBoolSetting("Filter3DPoints")) {
		double cutoff = (getOverrideCutoffFrequency()) ? getCutoffFrequency() : trial->getCutoffFrequency();
		
		//Filter 3D points
		std::vector <std::vector <markerStatus> > status3d_filtered; //= trial->getMarkers()[pointsIdx[i]]->getStatus3D()[Frame]
		std::vector <std::vector <cv::Point3d> > point3D_filtered; //= trial->getMarkers()[pointsIdx[i]]->getPoints3D()[Frame]

		for (unsigned int i = 0; i < pointsIdx.size(); i++)
		{
			std::vector<cv::Point3d> marker;
			std::vector<markerStatus> status;
			trial->getMarkers()[pointsIdx[i]]->filterMarker(cutoff, trial->getMarkers()[pointsIdx[i]]->getPoints3D(), trial->getMarkers()[pointsIdx[i]]->getStatus3D()
				, marker, status);

			status3d_filtered.push_back(status);
			point3D_filtered.push_back(marker);
		}

		std::vector <std::vector <markerStatus> >dummyPoint_status_filtered ;
		std::vector <std::vector <cv::Point3d> > dummyPoint_filtered;

		for (unsigned int i = 0; i < dummypoints.size(); i++)
		{
			std::vector<cv::Point3d> marker_in;
			std::vector<markerStatus> status_in;

			for (int Frame = 0; Frame < trial->getNbImages(); Frame++)
			{
				if (dummyRBIndex[i] >= 0)
				{
					cv::Point3d dummy_tmp;
					if (trial->getRigidBodies()[dummyRBIndex[i]]->transformPoint(dummypoints2[i], dummy_tmp, Frame))
					{
						marker_in.push_back(cv::Point3f(dummy_tmp.x
							, dummy_tmp.y
							, dummy_tmp.z));

						status_in.push_back(SET);
					}
					else {
						marker_in.push_back(cv::Point3f(-2, -2, -2));
						status_in.push_back(UNDEFINED);
					}
				}
				else if (dummypointsCoordsSet[i][Frame])
				{
					marker_in.push_back(cv::Point3f(dummypointsCoords[i][Frame].x
						, dummypointsCoords[i][Frame].y
						, dummypointsCoords[i][Frame].z));

					status_in.push_back(SET);
				}
				else {
					marker_in.push_back(cv::Point3f(-2, -2, -2));
					status_in.push_back(UNDEFINED);
				}
			}
			std::vector<cv::Point3d> marker;
			std::vector<markerStatus> status;
			trial->getMarkers()[pointsIdx[i]]->filterMarker(cutoff, marker_in, status_in, marker, status);

			dummyPoint_status_filtered.push_back(status);
			dummyPoint_filtered.push_back(marker);
		}

		//Compute Transformation
		for (int Frame = 0; Frame < trial->getNbImages(); Frame++)
		{
			std::vector<cv::Point3d> src;
			std::vector<cv::Point3d> dst;
			cv::Mat out;
			cv::Mat inliers;

			for (unsigned int i = 0; i < status3d_filtered.size(); i++)
			{
				if (status3d_filtered[i][Frame] > UNDEFINED)
				{
					src.push_back(cv::Point3f(point3D_filtered[i][Frame].x
						, point3D_filtered[i][Frame].y
						, point3D_filtered[i][Frame].z));
					dst.push_back(cv::Point3f(points3D[i].x
						, points3D[i].y
						, points3D[i].z));
				}
			}

			for (unsigned int i = 0; i < dummypoints.size(); i++)
			{
				if (dummyPoint_status_filtered[i][Frame])
				{
					src.push_back(cv::Point3f(dummyPoint_filtered[i][Frame].x
						, dummyPoint_filtered[i][Frame].y
						, dummyPoint_filtered[i][Frame].z));
					dst.push_back(cv::Point3f(dummypoints[i].x
						, dummypoints[i].y
						, dummypoints[i].z));
				}
			}

			if (dst.size() >= 3)
			{
				std::vector<std::vector<double> > y, x;
				std::vector<std::vector<double> > Y, X;
				std::vector<double> vnl_tmp(3), yg(3, 0), xg(3, 0);
				std::vector<double> tmp;
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
				cv::Rodrigues(rotMatTmp, rotationvectors_filtered[Frame]);

				cv::Mat xgmat = cv::Mat(xg, true);
				cv::Mat ygmat = cv::Mat(yg, true);
				cv::Mat t = ygmat - rotMatTmp * xgmat;
				translationvectors_filtered[Frame] = cv::Vec3d(t);
				poseFiltered[Frame] = true;
			}
			else {
				poseFiltered[Frame] = false;
			}
		}
	
		//set rotations numerically steady
		for (int i = 1; i < trial->getNbImages(); i++)
		{
			if (poseFiltered[i - 1] && poseFiltered[i])
			{
				double d = rotationvectors_filtered[i - 1].dot(rotationvectors_filtered[i]);
				double diffangle = d / cv::norm(rotationvectors_filtered[i - 1]) / cv::norm(rotationvectors_filtered[i]);

				if (180 / M_PI * acos(diffangle) > 150)
				{
					double angle = cv::norm(rotationvectors_filtered[i]);
					rotationvectors_filtered[i][0] = -rotationvectors_filtered[i][0] / angle * (2 * M_PI - angle);
					rotationvectors_filtered[i][1] = -rotationvectors_filtered[i][1] / angle * (2 * M_PI - angle);
					rotationvectors_filtered[i][2] = -rotationvectors_filtered[i][2] / angle * (2 * M_PI - angle);
				}
			}
		}
	}
	else {

		for (int i = 0; i < trial->getNbImages(); i++)
		{
			poseFiltered[i] = 0;
		}

		double cutoff = (getOverrideCutoffFrequency()) ? getCutoffFrequency() : trial->getCutoffFrequency();

		if (cutoff > 0 && trial->getRecordingSpeed() > 0 &&
			0 < (cutoff / (trial->getRecordingSpeed() * 0.5)) &&
			(cutoff / (trial->getRecordingSpeed() * 0.5)) < 1)
		{
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

void RigidBody::addDummyPoint(QString name, QString filenamePointRef, QString filenamePointRef2, int markerID, QString filenamePointCoords)
{
	dummyNames.push_back(name);

	std::ifstream fin;
	std::string line;

	fin.open(filenamePointRef.toStdString());
	littleHelper::safeGetline(fin, line);
	littleHelper::safeGetline(fin, line);
	fin.close();
	QString tmp_coords = QString::fromStdString(line);
	QStringList coords_list = tmp_coords.split(",");
	dummypoints.push_back(cv::Point3d(coords_list.at(0).toDouble(), coords_list.at(1).toDouble(), coords_list.at(2).toDouble()));

	if (coords_list.size() > 3)
	{
		markerID = coords_list.at(3).toInt();
	}

	dummyRBIndex.push_back(markerID);

	if (markerID == -1)
	{
		fin.open(filenamePointCoords.toStdString());
		littleHelper::safeGetline(fin, line);
		std::vector<cv::Point3d> tmpCoords;
		std::vector<bool> tmpDef;

		while (!littleHelper::safeGetline(fin, line).eof())
		{
			tmp_coords = QString::fromStdString(line);
			coords_list = tmp_coords.split(",");
			if (coords_list.size() == 3)
			{
				if (coords_list.at(0) == "NaN" || coords_list.at(1) == "NaN" || coords_list.at(2) == "NaN")
				{
					tmpDef.push_back(false);
					tmpCoords.push_back(cv::Point3d(0, 0, 0));
				}
				else
				{
					tmpDef.push_back(true);
					tmpCoords.push_back(cv::Point3d(coords_list.at(0).toDouble(), coords_list.at(1).toDouble(), coords_list.at(2).toDouble()));
				}
			}
		}

		while (tmpCoords.size() < poseComputed.size())
		{
			tmpDef.push_back(false);
			tmpCoords.push_back(cv::Point3d(0, 0, 0));
		}

		dummypointsCoords.push_back(tmpCoords);
		dummypointsCoordsSet.push_back(tmpDef);
		dummypoints2.push_back(cv::Point3d(0, 0, coords_list.at(2).toDouble()));
		fin.close();
	}
	else
	{
		fin.open(filenamePointRef2.toStdString());
		littleHelper::safeGetline(fin, line);
		littleHelper::safeGetline(fin, line);
		fin.close();
		tmp_coords = QString::fromStdString(line);
		coords_list = tmp_coords.split(",");
		dummypoints2.push_back(cv::Point3d(coords_list.at(0).toDouble(), coords_list.at(1).toDouble(), coords_list.at(2).toDouble()));
		
		//Have to initialise all the values
		std::vector<cv::Point3d> tmpCoords(poseComputed.size(), cv::Point3d(0, 0, 0));
		std::vector<bool> tmpDef(poseComputed.size(),false);
		dummypointsCoords.push_back(tmpCoords);
		dummypointsCoordsSet.push_back(tmpDef);
	}
}

void RigidBody::saveDummy(int count, QString filenamePointRef, QString filenamePointRef2, QString filenamePointCoords)
{
	std::ofstream outfileRef(filenamePointRef.toStdString());
	outfileRef.precision(12);
	outfileRef << "x,y,z" << std::endl;
	outfileRef << dummypoints[count].x << "," << dummypoints[count].y << "," << dummypoints[count].z << "," << dummyRBIndex[count] << std::endl;
	outfileRef.close();
	
	if(dummyRBIndex[count] < 0){
		std::ofstream outfileCoords(filenamePointCoords.toStdString());
		outfileCoords.precision(12);
		outfileCoords << "x,y,z" << std::endl;
		for (unsigned int i = 0; i < dummypointsCoords[count].size(); i++)
		{
			if (dummypointsCoordsSet[count][i])
			{
				outfileCoords << dummypointsCoords[count][i].x << "," << dummypointsCoords[count][i].y << "," << dummypointsCoords[count][i].z << std::endl;
			}
			else
			{
				outfileCoords << "NaN,NaN,NaN" << std::endl;
			}
		}
	}
	else
	{
		std::ofstream outfileRef2(filenamePointRef2.toStdString());
		outfileRef2.precision(12);
		outfileRef2 << "x,y,z" << std::endl;
		outfileRef2 << dummypoints2[count].x << "," << dummypoints2[count].y << "," << dummypoints2[count].z << std::endl;
		outfileRef2.close();
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



void RigidBody::setTransformation(int frame, cv::Vec3d rotVec, cv::Vec3d transVec)
{
	rotationvectors[frame][0] = rotVec[0];
	rotationvectors[frame][1] = rotVec[1];
	rotationvectors[frame][2] = rotVec[2];

	translationvectors[frame][0] = transVec[0];
	translationvectors[frame][1] = transVec[1];
	translationvectors[frame][2] = transVec[2];

	//updateError(frame);
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
		return conv * atan2(-rotMat.at<double>(2, 0), sqrt(rotMat.at<double>(2, 1) * rotMat.at<double>(2, 1) + rotMat.at<double>(2, 2) * rotMat.at<double>(2, 2)));
	}
	else if (part == 2)
	{
		return conv * atan2(rotMat.at<double>(1, 0), rotMat.at<double>(0, 0));
	}
	//This should never happen
	return 0.0;
}

void RigidBody::saveTransformations(QString filename, bool inverse, bool filtered)
{
	if (!filename.isEmpty())
	{
		std::ofstream outfile(filename.toStdString());
		outfile.precision(12);
		for (int i = 0; i < trial->getNbImages(); i++)
		{
			if ((poseComputed[i] && !filtered) || (poseFiltered[i] && filtered))
			{
				double m[16];
				//inversere Rotation = transposed rotation
				//and opengl requires transposed, so we set R
				if (inverse)
				{
					cv::Mat rotationMat;
					if (!filtered)
					{
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
					if (!filtered)
					{
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
					if (!filtered)
					{
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
					if (!filtered)
					{
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

				outfile << m[0] << "," << m[1] << "," << m[2] << "," << m[3] << ",";
				outfile << m[4] << "," << m[5] << "," << m[6] << "," << m[7] << ",";
				outfile << m[8] << "," << m[9] << "," << m[10] << "," << m[11] << ",";
				outfile << m[12] << "," << m[13] << "," << m[14] << "," << m[15];
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
	if ((poseComputed[frame] && !filtered) || (poseFiltered[frame] && filtered))
	{
		cv::Mat rotationMat;
		if (!filtered)
		{
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
		if (!filtered)
		{
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

std::vector<cv::Point2d> RigidBody::projectToImage(Camera* cam, int Frame, bool with_center, bool dummy, bool dummy_frame, bool filtered)
{
	std::vector<cv::Point3d> points3D_frame;
	cv::Mat rotMatTmp;
	if (filtered)
	{
		cv::Rodrigues(rotationvectors_filtered[Frame], rotMatTmp);
	}
	else
	{
		cv::Rodrigues(rotationvectors[Frame], rotMatTmp);
	}
	if (with_center)
	{
		cv::Mat xmat = cv::Mat(center, true);

		cv::Mat tmp_mat;
		if (filtered)
		{
			tmp_mat = rotMatTmp.t() * ((xmat) - cv::Mat(translationvectors_filtered[Frame]));
		}
		else
		{
			tmp_mat = rotMatTmp.t() * ((xmat) - cv::Mat(translationvectors[Frame]));
		}
		cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
		                 tmp_mat.at<double>(1, 0),
		                 tmp_mat.at<double>(2, 0));

		points3D_frame.push_back(pt3d);
	}

	if (!dummy)
	{
		for (unsigned int i = 0; i < points3D.size(); i++)
		{
			cv::Mat xmat = cv::Mat(points3D[i], true);

			cv::Mat tmp_mat;
			if (filtered)
			{
				tmp_mat = rotMatTmp.t() * ((xmat) - cv::Mat(translationvectors_filtered[Frame]));
			}
			else
			{
				tmp_mat = rotMatTmp.t() * ((xmat) - cv::Mat(translationvectors[Frame]));
			}
			cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
			                 tmp_mat.at<double>(1, 0),
			                 tmp_mat.at<double>(2, 0));

			points3D_frame.push_back(pt3d);
		}
	}
	else
	{
		if (!dummy_frame)
		{
			for (unsigned int i = 0; i < dummypoints.size(); i++)
			{
				cv::Mat xmat = cv::Mat(dummypoints[i], true);

				cv::Mat tmp_mat;
				if (filtered)
				{
					tmp_mat = rotMatTmp.t() * ((xmat) - cv::Mat(translationvectors_filtered[Frame]));
				}
				else
				{
					tmp_mat = rotMatTmp.t() * ((xmat) - cv::Mat(translationvectors[Frame]));
				}
				cv::Point3d pt3d(tmp_mat.at<double>(0, 0),
				                 tmp_mat.at<double>(1, 0),
				                 tmp_mat.at<double>(2, 0));
				points3D_frame.push_back(pt3d);
			}
		}
		else
		{
			for (unsigned int i = 0; i < dummypoints.size(); i++)
			{
				if (dummyRBIndex[i] >= 0)
				{
					cv::Point3d dummy_tmp;
					if (trial->getRigidBodies()[dummyRBIndex[i]]->transformPoint(dummypoints2[i], dummy_tmp, Frame))
					{
						points3D_frame.push_back(dummy_tmp);
					}
				}
				else if (dummypointsCoordsSet[i][Frame])
				{
					points3D_frame.push_back(dummypointsCoords[i][Frame]);
				}
			}
		}
	}
	rotMatTmp.release();
	std::vector<cv::Point3f>object_points;
	std::vector<cv::Point2f>image_points;

	// Transfer the points into the correct size matrices
	for (unsigned int i = 0; i < points3D_frame.size(); ++i)
	{
		object_points.push_back(cv::Point3f(
			points3D_frame[i].x,
			points3D_frame[i].y,
			points3D_frame[i].z));
		image_points.push_back(cv::Point2f(0, 0));
	}

	if (points3D_frame.size() != 0){
		std::vector<float> distCoeff;
		cv::projectPoints(
			object_points,
			cam->getCalibrationImages()[trial->getReferenceCalibrationImage()]->getRotationVector(),
			cam->getCalibrationImages()[trial->getReferenceCalibrationImage()]->getTranslationVector(),
			cam->getCameraMatrix(),
			distCoeff,
			image_points);
	}
	else
	{
		std::cerr << description.toStdString() << " " << poseComputed[Frame] << " " << Frame << std::endl;
 	}

	std::vector<cv::Point2d> points2D_frame;
	for (unsigned int i = 0; i < points3D_frame.size(); i++)
	{
		cv::Point2d pt = image_points[i];
		points2D_frame.push_back(cam->undistortPoint(pt, false));
	}
	points3D_frame.clear();

	return points2D_frame;
}

void RigidBody::setMissingPoints(int Frame)
{
	if (poseComputed[Frame])
	{
		for (unsigned int c = 0; c < Project::getInstance()->getCameras().size(); c++)
		{
			std::vector<cv::Point2d> image_points = projectToImage(Project::getInstance()->getCameras()[c], Frame, false);
			for (unsigned int i = 0; i < image_points.size(); i++)
			{
				//if (trial->getMarkers()[pointsIdx[i]]->getStatus2D()[Frame][c] <= PREDICTED_RIGIDBODY)
				{
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
	fin.open(filename.toStdString());
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
		coords_list.size() != pointsIdx.size() * 3)
		return -1;


	std::vector<cv::Point3d> points3D_tmp;
	std::vector<QString> referenceNames_tmp;

	for (unsigned int i = 0; i < pointsIdx.size(); i++)
	{
		referenceNames_tmp.push_back(names_list.at(3 * i));
		referenceNames_tmp[i].replace("_x", "");
		points3D_tmp.push_back(cv::Point3d(coords_list.at(3 * i).toDouble(), coords_list.at(3 * i + 1).toDouble(), coords_list.at(3 * i + 2).toDouble()));
	}

	std::vector<int> permutations;
	for (unsigned int i = 0; i < points3D_tmp.size(); i++)
		permutations.push_back(i);

	double error = 1000000.0;
	std::vector<int> best_permutation;
	do
	{
		std::vector<cv::Point3d> perm_points;
		for (unsigned int i = 0; i < permutations.size(); i++)
			perm_points.push_back(points3D_tmp[permutations[i]]);

		double error_tmp = fitAndComputeError(perm_points, points3D);

		if (error_tmp < error)
		{
			error = error_tmp;
			//fprintf(stderr, "Error %lf Perm ", error);
			best_permutation.clear();
			for (unsigned int i = 0; i < permutations.size(); i++)
			{
				best_permutation.push_back(permutations[i]);
				//fprintf(stderr, " %d ", best_permutation[i]);
			}
			//fprintf(stderr, "\n");
		}
	}
	while (std::next_permutation(permutations.begin(), permutations.end()));

	if (error > 100)
	{
		return -2;
	}

	points3D.clear();
	points3D_original.clear();
	hasOptimizedCoordinates = false;
	referenceNames.clear();

	for (unsigned int i = 0; i < best_permutation.size(); i++)
	{
		points3D.push_back(points3D_tmp[best_permutation[i]]);
		points3D_original.push_back(points3D_tmp[best_permutation[i]]);
		referenceNames.push_back(referenceNames_tmp[best_permutation[i]]);
	}

	setReferencesSet(2);

	recomputeTransformations();
	filterTransformations();
	return 1;
}

bool RigidBody::setReferenceFromFrame(int frame)
{
	bool canSet = true;
	for (unsigned int i = 0; i < pointsIdx.size(); i++)
	{
		if (trial->getMarkers()[pointsIdx[i]]->getStatus3D()[frame] <= UNDEFINED) canSet = false;
	}

	if (!canSet) return false;

	points3D.clear();
	points3D_original.clear();
	hasOptimizedCoordinates = false;
	referenceNames.clear();

	for (unsigned int i = 0; i < pointsIdx.size(); i++)
	{
		points3D.push_back(trial->getMarkers()[pointsIdx[i]]->getPoints3D()[frame]);
		points3D_original.push_back(trial->getMarkers()[pointsIdx[i]]->getPoints3D()[frame]);
		referenceNames.push_back(trial->getMarkers()[pointsIdx[i]]->getDescription() + "_Frame" + QString::number(frame + 1));
	}
	initialised = true;
	setReferencesSet(2);
	recomputeTransformations();
	filterTransformations();
	return true;
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

	if (points3D.size() > 0)
	{
		for (unsigned int i = 0; i < points3D.size(); i++)
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

bool RigidBody::getHasOptimizedCoordinates()
{
	return hasOptimizedCoordinates;
}

QColor RigidBody::getColor()
{
	return color;
}

void RigidBody::setColor(QColor value)
{
	color.setRgb(value.red(), value.green(), value.blue());
}

void RigidBody::draw2D(Camera* cam, int frame)
{
	if (visible && (int) poseComputed.size() > frame && (poseComputed[frame] || (Settings::getInstance()->getBoolSetting("TrialDrawFiltered") && poseFiltered[frame])) && isReferencesSet())
	{
		std::vector<cv::Point2d> points2D_projected = projectToImage(cam, frame, true, false, false, Settings::getInstance()->getBoolSetting("TrialDrawFiltered"));

		for (unsigned int i = 1; i < points2D_projected.size(); i++)
		{
			glBegin(GL_LINES);
			glColor3ub(color.red(), color.green(), color.blue());
			glVertex2f(points2D_projected[0].x, points2D_projected[0].y);
			glVertex2f(points2D_projected[i].x, points2D_projected[i].y);
			glEnd();
		}

		if (dummyNames.size() > 0)
		{
			int count = 0;
			for (unsigned int i = 0; i < dummypoints.size(); i++)
			{
				if (dummyRBIndex[i] >= 0)
				{
					cv::Point3d dummy_tmp;
					if (trial->getRigidBodies()[dummyRBIndex[i]]->transformPoint(dummypoints2[i], dummy_tmp, frame, Settings::getInstance()->getBoolSetting("TrialDrawFiltered")))
					{
						count++;
					}
				}
				else if (dummypointsCoordsSet[i][frame])
				{
					count++;
				}
			}

			if (count == 0) return;

			points2D_projected.clear();
			points2D_projected = projectToImage(cam, frame, true, true);

			glLineStipple(10, 0xAAAA);
			glEnable(GL_LINE_STIPPLE);
			for (unsigned int i = 1; i < points2D_projected.size(); i++)
			{
				glBegin(GL_LINES);
				glColor3ub(color.red(), color.green(), color.blue());
				glVertex2f(points2D_projected[0].x, points2D_projected[0].y);
				glVertex2f(points2D_projected[i].x, points2D_projected[i].y);
				glEnd();
			}
			glDisable(GL_LINE_STIPPLE);

			points2D_projected.clear();
			points2D_projected = projectToImage(cam, frame, false, true, true);

			for (unsigned int i = 0; i < points2D_projected.size(); i++)
			{
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
	if (visible && (getPoseComputed()[frame] || (Settings::getInstance()->getBoolSetting("TrialDrawFiltered") && getPoseFiltered()[frame])) && isReferencesSet())
	{
		bool filtered_trans = Settings::getInstance()->getBoolSetting("TrialDrawFiltered");
		glPushMatrix();
		double m[16];
		//inversere Rotation = transposed rotation
		//and opengl requires transposed, so we set R

		cv::Mat rotationMat;
		cv::Rodrigues(getRotationVector(filtered_trans)[frame], rotationMat);
		for (unsigned int y = 0; y < 3; y++)
		{
			m[y * 4] = rotationMat.at<double>(y, 0);
			m[y * 4 + 1] = rotationMat.at<double>(y, 1);
			m[y * 4 + 2] = rotationMat.at<double>(y, 2);
			m[y * 4 + 3] = 0.0;
		}
		//inverse translation = translation rotated with inverse rotation/transposed rotation
		//R-1 * -t = R^tr * -t
		m[12] = m[0] * -getTranslationVector(filtered_trans)[frame][0] + m[4] * -getTranslationVector(filtered_trans)[frame][1] + m[8] * -getTranslationVector(filtered_trans)[frame][2];
		m[13] = m[1] * -getTranslationVector(filtered_trans)[frame][0] + m[5] * -getTranslationVector(filtered_trans)[frame][1] + m[9] * -getTranslationVector(filtered_trans)[frame][2];
		m[14] = m[2] * -getTranslationVector(filtered_trans)[frame][0] + m[6] * -getTranslationVector(filtered_trans)[frame][1] + m[10] * -getTranslationVector(filtered_trans)[frame][2];
		m[15] = 1.0;

		glMultMatrixd(m);

		for (unsigned int i = 0; i < points3D.size(); i++)
		{
			glColor3ub(color.red(), color.green(), color.blue());
			glBegin(GL_LINES);
			glVertex3d(center.x, center.y, center.z);
			glVertex3d(points3D[i].x, points3D[i].y, points3D[i].z);
			glEnd();
		}

		if (dummypoints.size() > 0)
		{
			glLineStipple(10, 0xAAAA);
			glEnable(GL_LINE_STIPPLE);
			for (unsigned int i = 1; i < dummypoints.size(); i++)
			{
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



