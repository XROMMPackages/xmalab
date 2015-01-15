#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/RigidBody.h"

using namespace xma;

RigidBody::RigidBody(int size){
	expanded = false;
	initialised = false;

	init(size);
}

RigidBody::~RigidBody(){
	pointsIdx.clear();

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

void RigidBody::clearPointIdx()
{
	pointsIdx.clear();
}

void RigidBody::addPointIdx(int idx)
{
	pointsIdx.push_back(idx);
}

void RigidBody::removePointIdx(int idx)
{
	pointsIdx.erase(std::remove(pointsIdx.begin(), pointsIdx.end(), idx), pointsIdx.end());
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
	for(int i = 0 ; i < rotationvectors.size(); i ++){
		rotationvectors[i].release();
		translationvectors[i].release();
	}
	
	rotationvectors.clear();
	translationvectors.clear();
	poseComputed.clear();

	errorMean2D.clear();
	errorSd2D.clear();
	errorMean3D.clear();
	errorSd3D.clear();
}

void RigidBody::init(int size){
	clear();

	for(int i = 0 ; i < size; i ++){
		cv::Mat rotationvector;
		rotationvector.create(3,1,CV_64F);
		rotationvectors.push_back(rotationvector);

		cv::Mat translationvector;
		translationvector.create(3,1,CV_64F);
		translationvectors.push_back(translationvector);

		poseComputed.push_back(0);

		errorMean2D.push_back(0);
		errorSd2D.push_back(0);
		errorMean3D.push_back(0);
		errorSd3D.push_back(0);
	}
}

