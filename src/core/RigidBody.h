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
///\file RigidBody.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef RIGIDBODY_H_
#define RIGIDBODY_H_

#include <QString>
#include <QColor>

#include <vector>
#include <opencv/cv.h>


namespace xma
{
	class Trial;
	class Camera;
	class Marker;
	class RigidBodyObj;


	class RigidBody
	{
	public:

		RigidBody(int size, Trial* trial);
		void copyData(RigidBody* rb);
		virtual ~RigidBody();

		void setDescription(QString _description);
		QString getDescription();

		const std::vector<int>& getPointsIdx();
		const std::vector<int>& getPoseComputed();
		const std::vector<int>& getPoseFiltered();
		const std::vector<QString>& getReferenceNames();
		const std::vector<cv::Vec3d>& getRotationVector(bool filtered);
		const std::vector<cv::Vec3d>& getTranslationVector(bool filtered);

		const std::vector<double>& getErrorMean2D();
		const std::vector<double>& getErrorSd2D();
		const std::vector<double>& getErrorMean3D();
		const std::vector<double>& getErrorSd3D();

		const std::vector<double>& getErrorMean2D_filtered();
		const std::vector<double>& getErrorSd2D_filtered();
		const std::vector<double>& getErrorMean3D_filtered();
		const std::vector<double>& getErrorSd3D_filtered();

		double getRotationEulerAngle(bool filtered, int frame, int part);
		Marker* getMarker(int idx);

		void clearPointIdx();
		void addPointIdx(int idx);
		void removePointIdx(int idx);
		void setPointIdx(int idx, int markerIdx);

		void setExpanded(bool _expanded);
		bool isExpanded();

		void computeCoordinateSystemAverage();
		void computeCoordinateSystem(int Frame);
		void computePose(int Frame);

		std::vector<cv::Point2d> projectToImage(Camera* cam, int Frame, bool with_center, bool dummy = false, bool dummy_frame = false, bool filtered = false);
		void setMissingPoints(int Frame);

		int setReferenceFromFile(QString filename);
		double fitAndComputeError(std::vector<cv::Point3d> src, std::vector<cv::Point3d> dst);

		void save(QString filename_referenceNames, QString filename_points3D);
		void load(QString filename_referenceNames, QString filename_points3D);
		void saveTransformations(QString filename, bool inverse, bool filtered);
		bool getTransformationMatrix(int frame, bool filtered, double* trans);

		int isReferencesSet();
		void setReferencesSet(int value);
		void resetReferences();
		void setReferenceMarkerReferences();
		void updateCenter();

		bool getOverrideCutoffFrequency();
		void setOverrideCutoffFrequency(bool value);
		double getCutoffFrequency();
		void setCutoffFrequency(double value);

		bool getVisible();
		void setVisible(bool value);

		QColor getColor();
		void setColor(QColor value);

		void draw2D(Camera* cam, int frame);
		void draw3D(int frame);
		void recomputeTransformations();
		void makeRotationsContinous();
		void filterTransformations();

		Trial* getTrial();

		void addDummyPoint(QString name, QString filenamePointRef, QString filenamePointRef2, int markerID, QString filenamePointCoords = "");
		void saveDummy(int count, QString filenamePointRef, QString filenamePointRef2, QString filenamePointCoords);
		void clearAllDummyPoints();
		const std::vector<QString>& getDummyNames();
		bool allReferenceMarkerReferencesSet();

		bool transformPoint(cv::Point3d in, cv::Point3d& out, int frame, bool filtered = false);

		bool addMeshModel(QString filename);
		bool hasMeshModel();
		QString getMeshModelname();
		bool getDrawMeshModel();
		void setDrawMeshModel(bool value);
		void drawMesh(int frame);
		double getMeshScale();
		void setMeshScale(double value);

	private:
		void init(int size);
		void clear();
		void addFrame();

		void filterData(std::vector<int> idx);

		void updateError(int Frame, bool filtered = false);

		bool visible;
		QColor color;

		QString description;
		bool expanded;
		bool initialised;
		int referencesSet;

		bool overrideCutoffFrequency;
		double cutoffFrequency;

		Trial* trial;

		std::vector<int> pointsIdx;
		std::vector<cv::Point3d> points3D;

		cv::Point3d center;
		std::vector<QString> referenceNames;

		std::vector<QString> dummyNames;
		std::vector<cv::Point3d> dummypoints;
		std::vector<cv::Point3d> dummypoints2;
		std::vector<int> dummyRBIndex;
		std::vector<std::vector<cv::Point3d> > dummypointsCoords;
		std::vector<std::vector<bool> > dummypointsCoordsSet;

		//for each frame
		std::vector<int> poseComputed;
		std::vector<int> poseFiltered;
		std::vector<cv::Vec3d> rotationvectors;
		std::vector<cv::Vec3d> translationvectors;
		std::vector<cv::Vec3d> rotationvectors_filtered;
		std::vector<cv::Vec3d> translationvectors_filtered;

		std::vector<double> errorMean2D;
		std::vector<double> errorSd2D;
		std::vector<double> errorMean3D;
		std::vector<double> errorSd3D;

		std::vector<double> errorMean2D_filtered;
		std::vector<double> errorSd2D_filtered;
		std::vector<double> errorMean3D_filtered;
		std::vector<double> errorSd3D_filtered;

		RigidBodyObj * meshmodel;
		bool m_drawMeshModel;
		double meshScale;
	};
}

#endif 

