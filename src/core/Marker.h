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
///\file Marker.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef MARKER_H_
#define MARKER_H_

#include <QString>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/video/tracking.hpp>

#include <QColor>

#define MIN_marker(a,b) (((a)<(b))?(a):(b))

namespace xma
{
//	enum markerStatus
//	{
//		DELETED = -2,
//		LOST = -1,
//		UNDEFINED = 0,
//		PREDICTED_PREVIOUS = 1,
//		PREDICTED_2FRAME = 2,
//		PREDICTED_RIGIDBODY = 3,
//		TRACKED = 4,
//		SET = 5,
//		MANUAL_REFINED = 6
//	};

		enum markerStatus
		{
			UNTRACKABLE = -30,
			DELETED = -20,
			LOST = -10,
			UNDEFINED = 0,
			PREDICTED = 10,
			INTERPOLATED = 20,
			TRACKED = 40,
			TRACKED_AND_OPTIMIZED = 45,
			SET = 50,
			SET_AND_OPTIMIZED = 55,
			MANUAL = 60,
			MANUAL_AND_OPTIMIZED = 65
		};

		enum interpolationMethod
		{
			NONE = 0,
			REPEAT = 10,
			LINEAR = 20,
			CUBIC = 30
		};

	class Trial;

	class Marker
	{
	public:

		Marker(int nbCameras, int size, Trial* _trial);
		virtual ~Marker();

		void setDescription(QString _description);
		QString getDescription();

		const std::vector<std::vector<markerStatus> >& getStatus2D();
		std::vector<std::vector<cv::Point2d> >& getPoints2D();
		std::vector<std::vector<cv::Point2d> >& getPoints2D_projected();
		std::vector<std::vector<double> >& getError2D();

		const std::vector<markerStatus>& getStatus3D();
		std::vector<cv::Point3d>& getPoints3D();

		void setReference3DPoint(double x, double y, double z);
		void loadReference3DPoint(QString filename);
		void saveReference3DPoint(QString filename);

		cv::Point3d getReference3DPoint();
		bool Reference3DPointSet();

		void setPoint(int camera, int activeFrame, double x, double y, markerStatus status);
		std::vector<cv::Point2d> getEpipolarLine(int cameraOrigin, int CameraDestination, int frame);
		void reconstruct3DPoint(int frame, bool updateAll = false);
		void reconstruct3DPointZisserman(int frame);
		void reconstruct3DPointZissermanIncremental(int frame);
		void reconstruct3DPointZissermanMatlab(int frame);
		void reconstruct3DPointZissermanIncrementalMatlab(int frame);
		void reconstruct3DPointRayIntersection(int frame);
		int getMarkerPrediction(int camera, int frame, double& x, double& y, bool forward);

		double getSize();
		double getSizeRange();
		void setSize(int camera, int frame, double size_value);
		void save(QString points_filename, QString status_filename = "", QString markersize_filename = "", QString pointsWorld_filename = "");
		void load(QString points_filename, QString status_filename, QString markersize_filename);
		void save3DPoints(QString points_filename, QString status_filename);
		void load3DPoints(QString points_filename, QString status_filename);
		void resetMultipleFrames(int camera, int frameStart, int frameEnd, bool toggleUntrackable = false);

		void interpolate();
		void setInterpolation(int frame, interpolationMethod method);
		interpolationMethod getInterpolation(int frame);
		bool getHasInterpolation();
		void updateHasInterpolation(); 
		void saveInterpolation(QString filename);
		void loadInterpolation(QString filename);

		void update(bool updateAll = false);

		bool isValid(int camera, int frame);
		void reset(int camera, int frame);

		int getSizeOverride();
		void setSizeOverride(int value);

		int getThresholdOffset();
		void setThresholdOffset(int value);

		int getMaxPenalty();
		void setMaxPenalty(int value);

		int getMethod();
		void setMethod(int value);

		Trial* getTrial();

		bool getRequiresRecomputation();
		void setRequiresRecomputation(bool value);
		bool filterMarker(double cutoffFrequency, const std::vector <cv::Point3d> &marker_in, const std::vector <markerStatus>& status_in, std::vector <cv::Point3d> &marker_out, std::vector <markerStatus>& status_out);

		void updateToProject12();
		void updateToProject13();

		QColor getStatusColor(int camera, int frame);
		void reprojectPoint(int frame);

		int getFirstTrackedFrame(int cam = -1);
		int getLastTrackedFrame(int cam = -1);
		int getFramesTracked(int cam = -1);
		double getReprojectionError(double * sd = nullptr, int start = -1, int end = -1);

	private:
		void init(int nbCameras, int size);
		void addFrame();
		void clear();
		void updateError(int frame);
		void filterData(std::vector<int> idx, double cutoffFrequency, const  std::vector<cv::Point3d>& marker_in, const  std::vector<markerStatus>& status_in, std::vector<cv::Point3d>& marker_out, std::vector<markerStatus>& status_out);
		markerStatus updateStatus12(int statusOld);

		Trial* trial;

		QString description;
		int trialIdx;
		std::vector<std::vector<cv::Point2d> > points2D;
		std::vector<std::vector<cv::Point2d> > points2D_projected;
		std::vector<std::vector<markerStatus> > status2D;
		std::vector<std::vector<double> > error2D;
		std::vector<interpolationMethod> interpolation;
		bool hasInterpolation;
		void updateMeanSize();
		std::vector<std::vector<double> > markerSize;
		double meanSize;
		double sizeRange;

		int thresholdOffset;
		int sizeOverride;
		int maxPenalty;
		int method;
		//int interpolation;

		bool point3D_ref_set;
		cv::Point3d point3D_ref;
		std::vector<cv::Point3d> points3D;
		std::vector<markerStatus> status3D;
		std::vector<double> error3D;

		bool requiresRecomputation;

	};
}

#endif 

