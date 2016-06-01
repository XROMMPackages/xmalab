//  ----------------------------------
//  XMA Lab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMA Lab software is provided under the terms of the GNU General Public License version 3 
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
///\file MarkerDetection.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/MarkerDetection.h" 

#include "ui/MainWindow.h"

#include "core/Project.h"
#include "core/Image.h"
#include "core/Trial.h"
#include "core/Marker.h"

#include <QtCore>
#include <opencv/highgui.h>
#include <core/Settings.h>

//#define WRITEIMAGES 1

using namespace xma;

int MarkerDetection::nbInstances = 0;

#ifndef sign
#define sign(a) ((a>=0)?1:(-1))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

MarkerDetection::MarkerDetection(int camera, int trial, int frame, int marker, double searcharea, bool refinementAfterTracking) : QObject()
{
	nbInstances++;
	m_camera = camera;
	m_trial = trial;
	m_frame = frame;
	m_marker = marker;
	m_method = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getMethod();
	m_refinementAfterTracking = refinementAfterTracking;
	m_x = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame].x;
	m_y = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame].y;
	m_searchArea = (int)(searcharea + 0.5);

	if (m_method == 1)
	{
		if (m_searchArea < 50) m_searchArea = 50;
	}
	else if (m_searchArea < 10) m_searchArea = 10;

	m_input_size = (Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSizeOverride() > 0) ? Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSizeOverride() :
		               (Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSize() > 0) ? Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSize() : 5;

	m_thresholdOffset = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getThresholdOffset();
#ifdef WRITEIMAGES
	//fprintf(stderr, "Start Marker Detection : Camera %d Pos %lf %lf Size %lf\n", m_camera, cerx, y, m_input_size);
#endif
}

MarkerDetection::~MarkerDetection()
{
}

void MarkerDetection::detectMarker()
{
	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(detectMarker_threadFinished()));

	QFuture<void> future = QtConcurrent::run(this, &MarkerDetection::detectMarker_thread);
	m_FutureWatcher->setFuture(future);
}

cv::Point2d MarkerDetection::detectionPoint(Image* image, int method, cv::Point2d center, int searchArea, int masksize, double threshold, double* size)
{
	cv::Point2d point_out(center.x, center.y);
	double tmp_size;

	int off_x = (int)(center.x - searchArea + 0.5);
	int off_y = (int)(center.y - searchArea + 0.5);

	//preprocess image
	cv::Mat subimage;
	image->getSubImage(subimage, searchArea, off_x, off_y);

#ifdef WRITEIMAGES
	cv::Mat orig2;
	cv::cvtColor(subimage, orig2, CV_GRAY2RGB);
	cv::imwrite("1_Det_original.png", orig2);
#endif
	if (method == 0 || method == 2 || method == 4 || method == 5)
	{
		if (method == 2 || method == 5) subimage = cv::Scalar::all(255) - subimage;

		//Convert To float
		cv::Mat img_float;
		subimage.convertTo(img_float, CV_32FC1);
#ifdef WRITEIMAGES
		cv::Mat orig;
		cv::cvtColor(subimage, orig, CV_GRAY2RGB);
#endif
		//Create Blurred image
		int radius = (int)(1.5 * masksize + 0.5);
		double sigma = radius * sqrt(2 * log(255)) - 1;
		cv::Mat blurred;
		cv::GaussianBlur(img_float, blurred, cv::Size(2 * radius + 1, 2 * radius + 1), sigma);

#ifdef WRITEIMAGES
		cv::imwrite("2_Det_blur.png", blurred);
#endif

		//Substract Background
		cv::Mat diff = img_float - blurred;
		cv::normalize(diff, diff, 0, 255, cv::NORM_MINMAX, -1, cv::Mat());
		diff.convertTo(subimage, CV_8UC1);

#ifdef WRITEIMAGES
		cv::imwrite("3_Det_diff.png", diff);
#endif

		//Median
		cv::medianBlur(subimage, subimage, 3);

#ifdef WRITEIMAGES
		cv::imwrite("4_Det_med.png", subimage);
#endif

		//Thresholding
		double minVal;
		double maxVal;
		minMaxLoc(subimage, &minVal, &maxVal);
		double thres = 0.5 * minVal + 0.5 * subimage.at<uchar>(searchArea, searchArea) + threshold * 0.01 * 255;
		cv::threshold(subimage, subimage, thres, 255, cv::THRESH_BINARY_INV);
		//cv::adaptiveThreshold(image, image, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 15, 10);
#ifdef WRITEIMAGES
		//fprintf(stderr, "Thres %lf Selected %d\n", thres, image.at<uchar>(searchArea, searchArea));
		cv::imwrite("5_Det_thresh.png", subimage);
#endif
		cv::GaussianBlur(subimage, subimage, cv::Size(3, 3), 1.3);
#ifdef WRITEIMAGES
		//fprintf(stderr, "Thres %lf Selected %d\n", thres, image.at<uchar>(searchArea, searchArea));
		cv::imwrite("6_Det_threshBlur.png", subimage);
#endif

		//Find contours
		cv::vector<cv::vector<cv::Point>> contours;
		cv::vector<cv::Vec4i> hierarchy;
		cv::findContours(subimage, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(off_x, off_y));
		double dist = 1000;
		int bestIdx = -1;

		//Find closest contour
		for (unsigned int i = 0; i < contours.size(); i++)
		{
			cv::Point2f circle_center;
			float circle_radius;
			cv::minEnclosingCircle(contours[i], circle_center, circle_radius);

			double distTmp = sqrt((center.x - circle_center.x) * (center.x - circle_center.x) + (center.y - circle_center.y) * (center.y - circle_center.y));
			if (distTmp < dist)
			{
				bestIdx = i;
				dist = distTmp;
			}
		}



		//set contour
		if (bestIdx >= 0)
		{
			cv::Point2f circle_center;
			float circle_radius;
			cv::minEnclosingCircle(contours[bestIdx], circle_center, circle_radius);
			point_out.x = circle_center.x;
			point_out.y = circle_center.y;
			tmp_size = circle_radius;

#ifdef WRITEIMAGES
			cv::Point2d cent = circle_center - cv::Point2f(off_x, off_y);
			cv::circle(orig, cent, circle_radius, cv::Scalar(0, 0, 255, 50));
			std::cerr << circle_radius << std::endl;
			std::cerr << cent.x << " " << cent.y << std::endl;

			cv::imwrite("7_Detected.png", orig);
#endif
		}
#ifdef WRITEIMAGES 
		else
		{
			fprintf(stderr, "Not found\n");
		}

		//fprintf(stderr, "Stop Marker Detection : Camera %d Pos %lf %lf Size %lf\n", m_camera, x, y, size);
#endif
		//clean
		img_float.release();
		blurred.release();
		diff.release();
		hierarchy.clear();
		contours.clear();
	}
	else if (method == 3)
	{
		IplImage* imgGrey = new IplImage(subimage);
		int w = imgGrey->width;
		int h = imgGrey->height;
		IplImage* eig_image = cvCreateImage(cvSize(w, h), IPL_DEPTH_32F, 1);
		IplImage* temp_image = cvCreateImage(cvSize(w, h), IPL_DEPTH_32F, 1);

		CvPoint2D32f corners[50] = {0};
		int corner_count = 50;
		double quality_level = 0.0001;
		double min_distance = 3;
		int eig_block_size = 7;
		int use_harris = true;
		cvGoodFeaturesToTrack(imgGrey, eig_image, temp_image, corners, &corner_count, quality_level, min_distance,NULL, eig_block_size, use_harris);

		int half_win_size = 7;
		int iteration = 100;
		double epislon = 0.001;
		cvFindCornerSubPix(imgGrey, corners, corner_count, cvSize(half_win_size, half_win_size), cvSize(-1, -1), cvTermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, iteration, epislon));

		double dist_min = searchArea * searchArea;
		double dist;
		for (int i = 0; i < corner_count; i ++)
		{
			dist = cv::sqrt((corners[i].x - (searchArea + 1)) * (corners[i].x - (searchArea + 1)) + (corners[i].y - (searchArea + 1)) * (corners[i].y - (searchArea + 1)));
			if (dist < dist_min)
			{
				point_out.x = off_x + corners[i].x;
				point_out.y = off_y + corners[i].y;
				tmp_size = half_win_size * 2 + 1;
				dist_min = dist;
			}
		}
		cvReleaseImage(&eig_image);
		cvReleaseImage(&temp_image);
	}
	else if (method == 1 || method == 6)
	{
		cv::SimpleBlobDetector::Params paramsBlob;

		paramsBlob.thresholdStep = Settings::getInstance()->getFloatSetting("BlobDetectorThresholdStep");
		paramsBlob.minThreshold = Settings::getInstance()->getFloatSetting("BlobDetectorMinThreshold");
		paramsBlob.maxThreshold = Settings::getInstance()->getFloatSetting("BlobDetectorMaxThreshold");
		paramsBlob.minRepeatability = Settings::getInstance()->getIntSetting("BlobDetectorMinRepeatability");
		paramsBlob.minDistBetweenBlobs = Settings::getInstance()->getFloatSetting("BlobDetectorMinDistBetweenBlobs");

		paramsBlob.filterByColor = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByColor");
		if (method == 1)
		{
			paramsBlob.blobColor = 255 - Settings::getInstance()->getIntSetting("BlobDetectorBlobColor");
		}
		else if (method == 6)
		{
			paramsBlob.blobColor = Settings::getInstance()->getIntSetting("BlobDetectorBlobColor");
		}
		paramsBlob.filterByArea = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByArea");
		paramsBlob.minArea = Settings::getInstance()->getFloatSetting("BlobDetectorMinArea");
		paramsBlob.maxArea = Settings::getInstance()->getFloatSetting("BlobDetectorMaxArea");

		paramsBlob.filterByCircularity = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByCircularity");
		paramsBlob.minCircularity = Settings::getInstance()->getFloatSetting("BlobDetectorMinCircularity");
		paramsBlob.maxCircularity = Settings::getInstance()->getFloatSetting("BlobDetectorMaxCircularity");

		paramsBlob.filterByInertia = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByInertia");
		paramsBlob.minInertiaRatio = Settings::getInstance()->getFloatSetting("BlobDetectorMinInertiaRatio");
		paramsBlob.maxInertiaRatio = Settings::getInstance()->getFloatSetting("BlobDetectorMaxInertiaRatio");

		paramsBlob.filterByConvexity = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByConvexity");
		paramsBlob.minConvexity = Settings::getInstance()->getFloatSetting("BlobDetectorMinConvexity");
		paramsBlob.maxConvexity = Settings::getInstance()->getFloatSetting("BlobDetectorMaxConvexity");

		cv::FeatureDetector* detector = new cv::SimpleBlobDetector(paramsBlob);
		cv::vector<cv::KeyPoint> keypoints;

		detector->detect(subimage, keypoints);

		double dist_min = searchArea * searchArea;
		double dist;
		for (unsigned int i = 0; i < keypoints.size(); i++)
		{
			dist = cv::sqrt((keypoints[i].pt.x - (searchArea + 1)) * (keypoints[i].pt.x - (searchArea + 1)) + (keypoints[i].pt.y - (searchArea + 1)) * (keypoints[i].pt.y - (searchArea + 1)));
			if (dist < dist_min)
			{
				point_out.x = off_x + keypoints[i].pt.x;
				point_out.y = off_y + keypoints[i].pt.y;
				tmp_size = keypoints[i].size;
				dist_min = dist;
			}
		}
		keypoints.clear();
	}

	subimage.release();

	if (size != NULL)
	{
		*size = tmp_size;
	}

	return point_out;
}

void MarkerDetection::detectMarker_thread()
{
	if (m_method == 6) return;

	cv::Point2d pt = detectionPoint(Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[m_camera]->getImage(), m_method, cv::Point2d(m_x, m_y), m_searchArea, m_input_size, m_thresholdOffset, &m_size);

	if (m_method == 4 || m_method == 5) refinePointPolynomialFit(pt, m_size, (m_method == 4), m_camera, m_trial);

	m_x = pt.x;
	m_y = pt.y;
}

void MarkerDetection::detectMarker_threadFinished()
{
	Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->setSize(m_camera, m_frame, m_size);
	Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->setPoint(m_camera, m_frame, m_x, m_y, m_refinementAfterTracking ? TRACKED : SET);

	delete m_FutureWatcher;
	nbInstances--;
	if (nbInstances == 0)
	{
		emit detectMarker_finished();
	}
	delete this;
}

void MarkerDetection::refinePointPolynomialFit(cv::Point2d& pt, double& radius_out, bool darkMarker, int camera, int trial)
{
	double limmult = 1.6; //multiplies size of box around particle for fitting -- not a sensitive parameter - should be slightly over one... say 1.6
	double maskmult = 1; //multiplies fall - off of weighting exponential in fine fit  -- should be about 1.0
	double improverthresh = 0.5; //repeat centre refinement cycle if x or y correction is greater than improverthresh
	bool subpixpeak = true;

	double skewness;
	double J;
	double eccentricity;
	double rotation;

	double radius = radius_out;
	double x = pt.x;
	double y = pt.y;
#ifdef WRITEIMAGES
	std::cerr << "radius " << radius << std::endl;
	std::cerr << "Pt " << x << " " << y << std::endl;
#endif
	//repeat subpixel correction until satisfactory
	int doextracycles = 3;
	bool stillgood = true;
	int refinementcount = 0;
	double maxrefinements = limmult * radius / improverthresh; //if it takes more than maxrefinements to find centre correction, then move on

	while (stillgood && (doextracycles > 0))
	{
		refinementcount = refinementcount + 1;
		double w = (int)(limmult * radius + 0.5);

		//preprocess image
		int off_x = (int)(x - w + 0.5);
		int off_y = (int)(y - w + 0.5);

		//preprocess image
		cv::Mat subimage;
		Project::getInstance()->getTrials()[trial]->getVideoStreams()[camera]->getImage()->getSubImage(subimage, w, off_x, off_y);
		
#ifdef WRITEIMAGES
		cv::imwrite("1_Refine_original.png", subimage);
#endif

		if (subimage.cols * subimage.rows < 15)
		{
			//std::cerr << "Too small" << std::endl;
			return;
		}

		cv::Mat A;
		A.create(subimage.cols * subimage.rows, 15, CV_64F);
		cv::Mat B;
		B.create(subimage.cols * subimage.rows, 1, CV_64F);
		cv::Mat p;
		p.create(15, 1, CV_64F);

		int count = 0;
		double tmpx, tmpy, tmpw;
		for (int j = 0; j < subimage.cols; j++)
		{
			for (int i = 0; i < subimage.rows; i++)
			{
				tmpx = off_x - x + j;
				tmpy = off_y - y + i;
				tmpw = exp(-(tmpx * tmpx + tmpy * tmpy) / (radius * radius * maskmult));
				if (darkMarker)
				{
					B.at<double>(count, 0) = tmpw * (255 - subimage.at<uchar>(i, j));
				}
				else
				{
					B.at<double>(count, 0) = tmpw * (subimage.at<uchar>(i, j));
				}
				A.at<double>(count, 0) = tmpw;
				int ocol = 1;
				for (int order = 1; order <= 4; order++)
				{
					for (int ocol2 = ocol; ocol2 < ocol + order; ocol2++)
					{
						//std::cerr  << ocol2 << ": Add X to " << ocol2 - order << std::endl;
						A.at<double>(count, ocol2) = tmpx * A.at<double>(count, ocol2 - order);
					}
					ocol = ocol + order;
					//std::cerr << ocol << ": Add Y to " << ocol - order - 1 << std::endl;
					A.at<double>(count, ocol) = tmpy * A.at<double>(count, ocol - order - 1);
					ocol++;
				}
				count++;
			}
		}

		cv::solve(A, B, p, cv::DECOMP_QR);

		cv::Mat quadric;
		quadric.create(subimage.size(), CV_64F);

		double val[6];
		double val2;
		for (int j = 0; j < subimage.cols; j++)
		{
			for (int i = 0; i < subimage.rows; i++)
			{
				tmpx = off_x - x + j;
				tmpy = off_y - y + i;

				val[0] = 1;
				int ocol = 1;
				for (int order = 1; order <= 2; order++)
				{
					for (int ocol2 = ocol; ocol2 < ocol + order; ocol2++)
					{
						val[ocol2] = tmpx * val[ocol2 - order];
					}
					ocol = ocol + order;
					val[ocol] = tmpy * val[ocol - order - 1];
					ocol++;
				}

				val2 = 0;
				for (int o = 0; o < 6; o++)
				{
					val2 += val[o] * p.at<double>(o, 0);
				}
				quadric.at<double>(i, j) = val2;
			}
		}

		double a = p.at<double>(3, 0);
		double b = p.at<double>(4, 0) / 2.0;
		double c = p.at<double>(5, 0);
		double d = p.at<double>(1, 0) / 2.0;
		double f = p.at<double>(2, 0) / 2.0;
		double g = p.at<double>(0, 0);

		J = a * c - b * b;
		double xc = (b * f - c * d) / J;
		double yc = (b * d - a * f) / J;

		x = x + sign(xc) * min(fabs(xc), improverthresh);
		y = y + sign(yc) * min(fabs(yc), improverthresh);

		rotation = 0.5 * (M_PI / 2.0 - atan((c - a) / 2 / b) + (a - c < 0)) * M_PI / 2.0;
		double ct = cos(rotation);
		double st = sin(rotation);
		double P1 = p.at<double>(10, 0) * pow(ct, 4) - p.at<double>(11, 0) * pow(ct, 3) * st + p.at<double>(12, 0) * ct * ct * st * st - p.at<double>(13, 0) * ct * pow(st, 3) + p.at<double>(14, 0) * pow(st, 4);
		double P2 = p.at<double>(10, 0) * pow(st, 4) + p.at<double>(11, 0) * pow(st, 3) * ct + p.at<double>(12, 0) * st * st * ct * ct + p.at<double>(13, 0) * st * pow(ct, 3) + p.at<double>(14, 0) * pow(ct, 4);
		double Q1 = p.at<double>(3, 0) * ct * ct - p.at<double>(4, 0) * ct * st + p.at<double>(5, 0) * st * st;
		double Q2 = p.at<double>(3, 0) * st * st + p.at<double>(4, 0) * st * ct + p.at<double>(5, 0) * ct * ct;
		radius = fabs(sqrt(sqrt(Q1 * Q2 / P1 / P2 / 36))); //geometric mean

		stillgood = (refinementcount <= maxrefinements) && (J > 0); //if not still good, stop at once...
		bool improverswitch = (fabs(xc) > improverthresh) || (fabs(yc) > improverthresh); //check if xc, yc above thresh or extra cycles required
		doextracycles -= (!improverswitch) ? 1 : 0; //if still good and improverswitch turns off, do extra cycles

		if (!stillgood || doextracycles == 0)
		{
			double semiaxes1 = sqrt(2 * (a * f * f + c * d * d + g * b * b - 2 * b * d * f - a * c * g) / -J / ((a - c) * sqrt(1 + 4 * b * b / ((a - c) * (a - c))) - c - a));
			double semiaxes2 = sqrt(2 * (a * f * f + c * d * d + g * b * b - 2 * b * d * f - a * c * g) / -J / ((c - a) * sqrt(1 + 4 * b * b / ((a - c) * (a - c))) - c - a));
			if (semiaxes1 > semiaxes2)
			{
				double tmpaxis = semiaxes1;
				semiaxes1 = semiaxes2;
				semiaxes2 = tmpaxis;
			}
			eccentricity = sqrt(1 - (semiaxes1 * semiaxes1) / (semiaxes2 * semiaxes2));

			skewness = (fabs(p.at<double>(6, 0)) + fabs(p.at<double>(7, 0)) + fabs(p.at<double>(8, 0)) + fabs(p.at<double>(9, 0))) * radius / J;

			if (!subpixpeak)
			{
				//calculate sub - pixel corrections based on centroid of image within particle
				double xedge = 2 * radius / sqrt(pow(cos(rotation), 2) + pow(sin(rotation), 2) / (1 - eccentricity * eccentricity)) / (1 + sqrt(1 - eccentricity * eccentricity));
				cv::Mat inparticle;
				inparticle.create(quadric.size(), CV_8UC1);
				for (int j = 0; j < quadric.cols; j++)
				{
					for (int i = 0; i < quadric.rows; i++)
					{
						inparticle.at<char>(i, j) = (quadric.at<double>(i, j) > a * xedge * xedge + 2 * d * xedge + g) ? 1 : 0;
					}
				}
				cv::Mat weight;
				weight.create(quadric.size(), CV_64F);
				double sumweight = 0;
				double sumx = 0;
				double sumy = 0;
				double tmpWeight;
				for (int j = 0; j < quadric.cols; j++)
				{
					for (int i = 0; i < quadric.rows; i++)
					{
						tmpx = off_x - x + j;
						tmpy = off_y - y + i;
						tmpw = exp(-(tmpx * tmpx + tmpy * tmpy) / (radius * radius * maskmult));
						tmpWeight = tmpw * inparticle.at<char>(i, j) * subimage.at<char>(i, j);
						weight.at<double>(i, j) = tmpWeight;
						sumweight += tmpWeight;
						sumx += tmpx * tmpWeight;
						sumy += tmpy * tmpWeight;
					}
				}
				if (sumweight != 0.0)
				{
					xc = sumx / sumweight;
					yc = sumx / sumweight;
				}

				x = x + xc;
				y = y + yc;

				weight.release();
				inparticle.release();
			}
		}

		p.release();
		B.release();
		subimage.release();
		A.release();
	}

	if (!isnan(radius) && !isnan(x) && !isnan(y))
	{
		pt.y = y;
		pt.x = x;
		radius_out = radius;
	}

#ifdef WRITEIMAGES
	std::cerr << "radius " << radius_out << std::endl;
	std::cerr << "Pt " << pt.x << " " << pt.y << std::endl;
#endif

	//std::cerr << "J " << J << std::endl;
	//std::cerr << "skewness " << skewness << std::endl;
	//std::cerr << "eccentricity " << eccentricity << std::endl;
	//std::cerr << "rotation " << rotation << std::endl;
	//std::cerr << "radius " << radius << std::endl;
	//std::cerr << "Pt " << x << " " << y << std::endl;
}

