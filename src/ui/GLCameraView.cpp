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
//  PROVIDED -AS IS-, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file GLCameraView.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef XMA_USE_PAINTER
#include <GL/glew.h>
#endif
#include "gl/MultisampleFrameBuffer.h"
#include "gl/DistortionShader.h"
#include "gl/BlendShader.h"
#include "ui/GLCameraView.h"
#include "ui/State.h"
#include "ui/ErrorDialog.h"
#include "ui/WizardDockWidget.h"
#include "ui/ConsoleDockWidget.h"
#include "ui/GLSharedWidget.h"
#include "ui/DetailViewDockWidget.h"

#include "core/Camera.h"
#include "core/UndistortionObject.h"
#include "core/CalibrationImage.h"
#include "core/CalibrationObject.h"
#include "core/Image.h"
#include "core/Trial.h"
#include "core/Project.h"
#include "core/Marker.h"
#include "core/RigidBody.h"
#include "core/Settings.h"
#include "core/CalibrationSequence.h"
#include "processing/MarkerDetection.h"


#include <QMouseEvent>
#include <QPainter>

#include <iostream> 
#include <math.h>
#include "MainWindow.h"

#ifndef XMA_USE_PAINTER
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
#endif


using namespace xma;

GLCameraView::GLCameraView(QWidget* parent)
#ifdef XMA_USE_PAINTER
	: QWidget(parent)
#else
	: QOpenGLWidget(parent)
#endif
{
    camera = NULL;
    setMinimumSize(50, 50);
    setAutoFillBackground(false);
    this->setCursor(QCursor(Qt::CrossCursor));
#ifndef XMA_USE_PAINTER
    window_width = 50;
    window_height = 50;
    x_offset = 0;
    y_offset = 0;
    setZoomRatio(1.0, true);
    detailedView = false;
    bias = 0.0;
    scale = 1.0;
    transparency = 0.5;
    renderTransparentModels = true;
    showStatusColors = false;
    distortionShader = 0;
    blendShader = NULL; 
    rigidbodyBufferUndistorted = NULL;
#else
	window_width = 50;
	window_height = 50;
	x_offset = 0;
	y_offset = 0;
	setZoomRatio(1.0, true);
	detailedView = false;
	bias = 0.0;
	scale = 1.0;
	transparency = 0.5;
	renderTransparentModels = true;
	showStatusColors = false;
#endif

#ifndef XMA_USE_PAINTER
	LightAmbient[0] = LightAmbient[1] = LightAmbient[2] = 0.1f;
	LightAmbient[3] = 1.0f;

	LightDiffuse[0] = LightDiffuse[1] = LightDiffuse[2] = 0.7f;
	LightDiffuse[3] = 1.0f;

	LightPosition_front[0] = LightPosition_front[1] = LightPosition_front[3] = 0.0f;
	LightPosition_front[2] = 1.0f;

	LightPosition_back[0] = LightPosition_back[1] = LightPosition_back[3] = 0.0f;
	LightPosition_back[2] = -1.0f;
#endif
}

GLCameraView::~GLCameraView()
{
#ifndef XMA_USE_PAINTER
    if (blendShader)
        delete blendShader;

    if (rigidbodyBufferUndistorted)
        delete rigidbodyBufferUndistorted;

    if (distortionShader)
        delete distortionShader;
#endif
}

void GLCameraView::setCamera(Camera* _camera)
{
    camera = _camera;
#ifndef XMA_USE_PAINTER
    camera_width = camera->getWidth();
    camera_height = camera->getHeight();
    if (!detailedView){
        if (rigidbodyBufferUndistorted)
            delete rigidbodyBufferUndistorted;
        rigidbodyBufferUndistorted = new FrameBuffer(camera_width, camera_height);
        if (distortionShader)
            delete distortionShader;
        distortionShader = new DistortionShader(camera);
        if (blendShader)
            delete blendShader;
        blendShader = new BlendShader(); 
    }
#else
	camera_width = camera->getWidth();
	camera_height = camera->getHeight();
#endif
}

#ifdef XMA_USE_PAINTER
// Painter-based implementation
static inline QImage matToQImageCopy(const cv::Mat& mat)
{
	if (mat.empty()) return QImage();
	if (mat.type() == CV_8UC1) {
		QImage img(mat.cols, mat.rows, QImage::Format_Grayscale8);
		for (int y = 0; y < mat.rows; ++y) {
			memcpy(img.scanLine(y), mat.ptr(y), mat.cols);
		}
		return img;
	} else if (mat.type() == CV_8UC3) {
		QImage img(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_RGB888);
		return img.rgbSwapped().copy();
	} else if (mat.type() == CV_8UC4) {
		QImage img(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_ARGB32);
		return img.copy();
	}
	// Fallback copy via conversion
	cv::Mat tmp;
	if (mat.channels() == 3) {
		cv::cvtColor(mat, tmp, cv::COLOR_BGR2RGB);
		QImage img(tmp.data, tmp.cols, tmp.rows, static_cast<int>(tmp.step), QImage::Format_RGB888);
		return img.copy();
	}
	return QImage();
}

void GLCameraView::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.fillRect(rect(), QColor(0,0,0));
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::TextAntialiasing, true);
	// keep pixels crisp for images
	p.setRenderHint(QPainter::SmoothPixmapTransform, false);

	if (!camera) {
		p.setPen(Qt::white);
		p.drawText(rect(), Qt::AlignCenter, QStringLiteral("No camera"));
		return;
	}

	// Select source image
	xma::Image* src = nullptr;
	bool color = false;
	if (State::getInstance()->getWorkspace() == UNDISTORTION) {
		if (camera->hasUndistortion()) {
			int t = State::getInstance()->getUndistortionVisImage();
			src = camera->getUndistortionObject()->getDisplayImage(t);
			color = (t != 0);
		}
	} else if (State::getInstance()->getWorkspace() == CALIBRATION && Project::getInstance()->getCalibration() == INTERNAL) {
		bool dist = (State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE);
		src = camera->getCalibrationSequence()->getImage(State::getInstance()->getActiveFrameCalibration(), dist);
	} else if (State::getInstance()->getWorkspace() == DIGITIZATION) {
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0) {
			if (!Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getIsDefault()) {
				src = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getVideoStreams()[camera->getID()]->getImage();
			}
		}
	}

	QImage img;
	if (src) {
		cv::Mat m;
		src->getImage(m, color);
		img = matToQImageCopy(m);
	}

	// Auto-fit if needed
	if (autozoom) {
		setZoomToFit();
	}

	// Draw image with zoom/pan using a single painter transform that overlays can reuse
	if (!img.isNull()) {
		const qreal dpr = this->devicePixelRatio();
		// Translate by half the widget size (logical coords), plus offset scaled to logical via 1/zoom
		const double sx = window_width * 0.5 + x_offset / zoomRatio;
		const double sy = window_height * 0.5 + y_offset / zoomRatio;

		// Build transform from image space (pixels) -> widget logical coords
		QTransform T;
		T.translate(sx, sy);
		T.scale(1.0 / (zoomRatio * dpr), 1.0 / (zoomRatio * dpr));
		p.setTransform(T, false);

		// With the transform set, draw the image at its image-space origin
		p.drawImage(QPointF(0.0, 0.0), img);

		// Overlays (draw in image coordinates; they inherit the same transform)
		if (State::getInstance()->getWorkspace() == UNDISTORTION) {
			drawUndistortionOverlays(p);
		} else if (State::getInstance()->getWorkspace() == CALIBRATION) {
			drawCalibrationOverlays(p);
			// Draw calibration wizard reference crosses (pre-calibration clicks)
			WizardDockWidget::getInstance()->draw(&p);
		} else if (State::getInstance()->getWorkspace() == DIGITIZATION) {
			drawDigitizationOverlays(p);
		}
	} else {
		p.setPen(Qt::white);
		p.drawText(rect(), Qt::AlignCenter, QStringLiteral("No image"));
	}
}

void GLCameraView::drawUndistortionOverlays(QPainter& p)
{
	if (!camera || !camera->hasUndistortion()) return;
	p.setRenderHint(QPainter::Antialiasing, true);

	// Draw in image space (transform already set by paintEvent)
	int visPts = State::getInstance()->getUndistortionVisPoints();
	auto uo = camera->getUndistortionObject();

	if (visPts == 1) {
		// detected points (red)
		std::vector<cv::Point2d> pts;
		uo->getDetectedPoints(pts);
		QPen pen(QColor(255,0,0));
		p.setPen(pen);
		for (auto &pt : pts) {
			p.drawLine(QPointF(pt.x - 2, pt.y), QPointF(pt.x + 2, pt.y));
			p.drawLine(QPointF(pt.x, pt.y - 2), QPointF(pt.x, pt.y + 2));
		}
	} else if (visPts == 2 || visPts == 3 || visPts == 4) {
		std::vector<cv::Point2d> d, r, u;
		std::vector<bool> inlier;
		uo->getGridPoints(d, r, inlier);
		if (visPts == 3) {
			// Build undistorted points from distorted grid if not directly exposed
			u.reserve(d.size());
			for (const auto& pd : d) u.push_back(uo->transformPoint(pd, true));
		}
		const std::vector<cv::Point2d> &pts = (visPts == 2 ? d : (visPts == 3 ? u : r));
		for (size_t i = 0; i < pts.size(); ++i) {
			QColor c = (i < inlier.size() && inlier[i]) ? QColor(0, 204, 0) : QColor(204, 0, 0);
			p.setPen(QPen(c));
			const auto &pt = pts[i];
			p.drawLine(QPointF(pt.x - 2, pt.y), QPointF(pt.x + 2, pt.y));
			p.drawLine(QPointF(pt.x, pt.y - 2), QPointF(pt.x, pt.y + 2));
		}
	}

	if (camera->getUndistortionObject()->isCenterSet()) {
		auto c = camera->getUndistortionObject()->getCenter();
		p.setPen(QPen(QColor(0,0,255)));
		p.drawLine(QPointF(c.x - 5, c.y - 5), QPointF(c.x + 5, c.y + 5));
		p.drawLine(QPointF(c.x + 5, c.y - 5), QPointF(c.x - 5, c.y + 5));
	}
}

void GLCameraView::drawCalibrationOverlays(QPainter& p)
{
	if (!camera) return;
	if (!(State::getInstance()->getWorkspace() == CALIBRATION && Project::getInstance()->getCalibration() == INTERNAL)) return;

	int frame = State::getInstance()->getActiveFrameCalibration();
	int textMode = State::getInstance()->getCalibrationVisText();
	bool distorted = (State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE);
	// First: draw the point overlays (crosses), mirroring CalibrationImage::draw(int)
	int ptsMode = State::getInstance()->getCalibrationVisPoints();
	auto calibImg = camera->getCalibrationImages()[frame];
	p.setRenderHint(QPainter::Antialiasing, true);

	auto drawCross = [&](double x, double y, const QColor& c, int halfSize) {
		p.setPen(QPen(c));
		p.drawLine(QPointF(x - halfSize, y), QPointF(x + halfSize, y));
		p.drawLine(QPointF(x, y - halfSize), QPointF(x, y + halfSize));
	};

	switch (ptsMode) {
		case 1: { // detectedPoints_ALL (small red crosses)
			const auto& all = calibImg->getDetectedPointsAll();
			for (const auto& pt : all) {
				drawCross(pt.x, pt.y, QColor(255, 0, 0), 2);
			}
			break;
		}
		case 2: { // detectedPoints (color by inlier)
			const auto& pts = calibImg->getDetectedPoints();
			const auto& in = calibImg->getInliers();
			for (size_t i = 0; i < pts.size() && i < in.size(); ++i) {
				QColor c;
				if (in[i] == 1) c = QColor(0, 204, 0);
				else if (in[i] == 0) c = QColor(204, 0, 0);
				else c = QColor(0, 0, 255); // -1 or others
				drawCross(pts[i].x, pts[i].y, c, 5);
			}
			break;
		}
		case 3: // projectedPoints (distorted)
		case 5: { // projectedPointsUndistorted
			std::vector<double> xs, ys; std::vector<QString> dummy; std::vector<bool> inlierBool;
			bool wantDistorted = (ptsMode == 3);
			calibImg->getDrawTextData(/*type*/1, wantDistorted, xs, ys, dummy, inlierBool);
			const auto& in = calibImg->getInliers();
			for (size_t i = 0; i < xs.size() && i < ys.size() && i < in.size(); ++i) {
				QColor c;
				if (in[i] == 1) c = QColor(0, 204, 0);
				else if (in[i] == 0) c = QColor(204, 0, 0);
				else c = QColor(0, 0, 255);
				drawCross(xs[i], ys[i], c, 5);
			}
			break;
		}
		case 4: { // detectedPointsUndistorted
			const auto& pts = calibImg->getDetectedPointsUndistorted();
			const auto& in = calibImg->getInliers();
			for (size_t i = 0; i < pts.size() && i < in.size(); ++i) {
				QColor c;
				if (in[i] == 1) c = QColor(0, 204, 0);
				else if (in[i] == 0) c = QColor(204, 0, 0);
				else c = QColor(0, 0, 255);
				drawCross(pts[i].x, pts[i].y, c, 5);
			}
			break;
		}
		default:
			break;
	}

	// Then: draw any requested text overlays
	if (textMode > 0) {
		std::vector<double> xs, ys; std::vector<QString> texts; std::vector<bool> inlier;
		calibImg->getDrawTextData(textMode, distorted, xs, ys, texts, inlier);
		QFont f = this->font(); f.setPointSize(12); p.setFont(f);
		for (size_t i = 0; i < texts.size() && i < xs.size() && i < ys.size(); ++i) {
			QColor color = (i < inlier.size() && inlier[i]) ? QColor(0, 255, 0) : QColor(255, 0, 0);
			p.setPen(color);
			p.drawText(QPointF(xs[i] + 3, ys[i]), texts[i]);
		}
	}
}

void GLCameraView::drawDigitizationOverlays(QPainter& p)
{
	if (!camera) return;
	if (Settings::getInstance()->getBoolSetting("TrialDrawHideAll")) return;

	const auto& trials = Project::getInstance()->getTrials();
	const int trialIndex = State::getInstance()->getActiveTrial();
	if (trialIndex < 0 || trialIndex >= static_cast<int>(trials.size())) return;
	Trial* trial = trials[trialIndex];
	if (!trial || trial->getIsDefault()) return;

	const int cameraId = camera->getID();
	const int frame = State::getInstance()->getActiveFrameTrial();
	const auto& markers = trial->getMarkers();
	const int activeMarkerIdx = trial->getActiveMarkerIdx();

	const auto has2D = [&](Marker* marker) -> bool {
		if (!marker) return false;
		const auto& statusPerCamera = marker->getStatus2D();
		if (cameraId >= static_cast<int>(statusPerCamera.size())) return false;
		if (frame >= static_cast<int>(statusPerCamera[cameraId].size())) return false;
		return statusPerCamera[cameraId][frame] > 0;
	};

	if (Settings::getInstance()->getBoolSetting("TrialDrawFiltered") &&
	    (trial->getCutoffFrequency() <= 0.0 || trial->getRecordingSpeed() <= 0.0))
	{
		p.save();
		QFont font = this->font();
		font.setPointSize(12);
		p.setFont(font);
		p.setPen(QColor(255, 0, 0));
		const QString message = QStringLiteral("Rendering of filtered data is enabled, but framerate and cutoff are not set correctly");
		const QFontMetrics fm(font);
		const qreal textWidth = fm.horizontalAdvance(message);
		const qreal x = 0.5 * camera_width - 0.5 * textWidth;
		const qreal y = fm.ascent() + 6.0;
		p.drawText(QPointF(x, y), message);
		p.restore();
	}

	const bool drawMarkers = Settings::getInstance()->getBoolSetting("TrialDrawMarkers");
	const bool coloredCross = Settings::getInstance()->getBoolSetting("ShowColoredMarkerCross");
	const bool advancedCrosshair = Settings::getInstance()->getBoolSetting("AdvancedCrosshairDetailView");
	const bool drawProjectedAll = Settings::getInstance()->getBoolSetting("DrawProjected2DpositionsForAllPoints");
	const bool show3dPointDetail = Settings::getInstance()->getBoolSetting("Show3dPointDetailView");

	if (drawMarkers)
	{
		p.save();
		if (!detailedView)
		{
			for (int idx = 0; idx < static_cast<int>(markers.size()); ++idx)
			{
				Marker* marker = markers[idx];
				if (!has2D(marker)) continue;
				const cv::Point2d& pt = marker->getPoints2D()[cameraId][frame];
				QColor color = (idx == activeMarkerIdx) ? QColor(255, 0, 0) : QColor(0, 255, 0);
				if (coloredCross)
				{
					color = marker->getStatusColor(cameraId, frame);
				}
				p.setPen(QPen(color, 0));
				p.drawLine(QPointF(pt.x - 5.0, pt.y), QPointF(pt.x + 5.0, pt.y));
				p.drawLine(QPointF(pt.x, pt.y - 5.0), QPointF(pt.x, pt.y + 5.0));
			}
		}
		else if (activeMarkerIdx >= 0 && activeMarkerIdx < static_cast<int>(markers.size()))
		{
			Marker* marker = markers[activeMarkerIdx];
			if (has2D(marker))
			{
				const cv::Point2d& pt = marker->getPoints2D()[cameraId][frame];
				p.setPen(QPen(QColor(255, 0, 0), 0));
				p.drawLine(QPointF(pt.x - 12.0, pt.y), QPointF(pt.x + 12.0, pt.y));
				p.drawLine(QPointF(pt.x, pt.y - 12.0), QPointF(pt.x, pt.y + 12.0));

				if (advancedCrosshair)
				{
					for (int i = 0; i < 6; ++i)
					{
						p.drawLine(QPointF(pt.x - 1.0, pt.y + i * 2.0), QPointF(pt.x + 1.0, pt.y + i * 2.0));
						p.drawLine(QPointF(pt.x - 1.0, pt.y - i * 2.0), QPointF(pt.x + 1.0, pt.y - i * 2.0));
						p.drawLine(QPointF(pt.x + i * 2.0, pt.y - 1.0), QPointF(pt.x + i * 2.0, pt.y + 1.0));
						p.drawLine(QPointF(pt.x - i * 2.0, pt.y - 1.0), QPointF(pt.x - i * 2.0, pt.y + 1.0));
					}
					double size = marker->getSize();
					if (size > 0.0)
					{
						p.save();
						p.setPen(QPen(QColor(255, 0, 0, 76)));
						p.setBrush(Qt::NoBrush);
						p.drawRect(QRectF(pt.x - size, pt.y - size, size * 2.0, size * 2.0));
						p.restore();
					}
				}
			}
		}
		p.restore();
	}

	const auto drawProjectedCross = [&](const cv::Point2d& pt, qreal size) {
		p.drawLine(QPointF(pt.x - size, pt.y - size), QPointF(pt.x + size, pt.y + size));
		p.drawLine(QPointF(pt.x + size, pt.y - size), QPointF(pt.x - size, pt.y + size));
	};

	if (activeMarkerIdx >= 0 && activeMarkerIdx < static_cast<int>(markers.size()))
	{
		Marker* marker = markers[activeMarkerIdx];
		if (marker && frame < static_cast<int>(marker->getStatus3D().size()) && marker->getStatus3D()[frame] > 0)
		{
			const auto& projections = marker->getPoints2D_projected();
			if (cameraId < static_cast<int>(projections.size()) &&
			    frame < static_cast<int>(projections[cameraId].size()) &&
			    (!detailedView || show3dPointDetail))
			{
				p.save();
				p.setPen(QPen(QColor(0, 255, 255), 0));
				drawProjectedCross(projections[cameraId][frame], 5.0);
				p.restore();
			}
		}
	}

	if (!detailedView && drawProjectedAll)
	{
		p.save();
		p.setPen(QPen(QColor(0, 255, 255), 0));
		for (Marker* marker : markers)
		{
			if (!marker) continue;
			if (frame >= static_cast<int>(marker->getStatus3D().size()) || marker->getStatus3D()[frame] <= 0) continue;
			const auto& projections = marker->getPoints2D_projected();
			if (cameraId >= static_cast<int>(projections.size()) || frame >= static_cast<int>(projections[cameraId].size())) continue;
			drawProjectedCross(projections[cameraId][frame], 5.0);
		}
		p.restore();
	}

	if (!detailedView && Settings::getInstance()->getBoolSetting("TrialDrawRigidBodyConstellation"))
	{
		p.save();
		const bool wantFiltered = Settings::getInstance()->getBoolSetting("TrialDrawFiltered");
		const auto& rigidBodies = trial->getRigidBodies();
		for (RigidBody* rigidBody : rigidBodies)
		{
			if (!rigidBody) continue;
			if (!rigidBody->getVisible()) continue;
			const auto& poseComputed = rigidBody->getPoseComputed();
			if (frame < 0 || frame >= static_cast<int>(poseComputed.size())) continue;
			const auto& poseFiltered = rigidBody->getPoseFiltered();
			const bool hasPoseComputed = poseComputed[frame] != 0;
			const bool useFiltered = wantFiltered && frame < static_cast<int>(poseFiltered.size()) && poseFiltered[frame] != 0;
			if (!hasPoseComputed && !useFiltered) continue;
			if (rigidBody->isReferencesSet() <= 0) continue;

			const QColor rbColor = rigidBody->getColor();
			QPen solidPen(rbColor);
			solidPen.setWidthF(0.0);
			p.setPen(solidPen);

			std::vector<cv::Point2d> projected = rigidBody->projectToImage(camera, frame, true, false, false, useFiltered);
			if (projected.size() >= 2)
			{
				const QPointF centerPt(projected[0].x, projected[0].y);
				for (size_t idx = 1; idx < projected.size(); ++idx)
				{
					const QPointF endPt(projected[idx].x, projected[idx].y);
					p.drawLine(centerPt, endPt);
				}
			}

			if (!rigidBody->getDummyNames().empty())
			{
				std::vector<cv::Point2d> dashed = rigidBody->projectToImage(camera, frame, true, true, false, useFiltered);
				if (dashed.size() >= 2)
				{
					QPen dashPen(rbColor);
					dashPen.setStyle(Qt::DashLine);
					dashPen.setWidthF(0.0);
					p.setPen(dashPen);
					const QPointF centerPt(dashed[0].x, dashed[0].y);
					for (size_t idx = 1; idx < dashed.size(); ++idx)
					{
						const QPointF endPt(dashed[idx].x, dashed[idx].y);
						p.drawLine(centerPt, endPt);
					}
				}

				std::vector<cv::Point2d> dummies = rigidBody->projectToImage(camera, frame, false, true, true, useFiltered);
				if (!dummies.empty())
				{
					QPen crossPen(rbColor);
					crossPen.setWidthF(0.0);
					p.setPen(crossPen);
					for (const auto& pt : dummies)
					{
						p.drawLine(QPointF(pt.x - 5.0, pt.y), QPointF(pt.x + 5.0, pt.y));
						p.drawLine(QPointF(pt.x, pt.y - 5.0), QPointF(pt.x, pt.y + 5.0));
					}
				}
			}
		}
		p.restore();
	}

	if (Settings::getInstance()->getBoolSetting("TrialDrawEpipolar") &&
	    (!detailedView || Settings::getInstance()->getBoolSetting("ShowEpiLineDetailView")) &&
	    activeMarkerIdx >= 0 && activeMarkerIdx < static_cast<int>(markers.size()))
	{
		Marker* marker = markers[activeMarkerIdx];
		if (marker)
		{
			p.save();
			p.setPen(QPen(QColor(0, 0, 255), 0));
			for (unsigned int otherCam = 0; otherCam < Project::getInstance()->getCameras().size(); ++otherCam)
			{
				if (otherCam == static_cast<unsigned int>(cameraId)) continue;
				const auto& statusPerCamera = marker->getStatus2D();
				if (otherCam >= statusPerCamera.size()) continue;
				if (frame >= static_cast<int>(statusPerCamera[otherCam].size())) continue;
				if (statusPerCamera[otherCam][frame] <= 0) continue;
				const std::vector<cv::Point2d> epiline = marker->getEpipolarLine(otherCam, cameraId, frame);
				for (size_t i = 1; i < epiline.size(); ++i)
				{
					p.drawLine(QPointF(epiline[i - 1].x, epiline[i - 1].y),
					          QPointF(epiline[i].x, epiline[i].y));
				}
			}
			p.restore();
		}
	}

	const bool shouldShowIds = Settings::getInstance()->getBoolSetting("TrialDrawMarkerIds") &&
		(!detailedView || Settings::getInstance()->getBoolSetting("ShowIDsInDetail"));
	if (shouldShowIds)
	{
		std::vector<double> xs;
		std::vector<double> ys;
		std::vector<QString> labels;
		trial->getDrawTextData(cameraId, frame, xs, ys, labels);
		p.save();
		QFont font = this->font();
		font.setPointSize(15);
		p.setFont(font);
		for (int i = 0; i < static_cast<int>(labels.size()) && i < static_cast<int>(markers.size()); ++i)
		{
			Marker* marker = markers[i];
			if (!has2D(marker)) continue;
			QColor color;
			if (Settings::getInstance()->getBoolSetting("ShowColoredMarkerIDs"))
			{
				color = marker->getStatusColor(cameraId, frame);
			}
			else
			{
				color = (i == activeMarkerIdx) ? QColor(255, 0, 0) : QColor(0, 255, 0);
			}
			p.setPen(color);
			p.drawText(QPointF(xs[i] + 3.0, ys[i]), labels[i]);
		}
		p.restore();
	}

	if (State::getInstance()->getActiveCamera() == cameraId)
	{
		WizardDockWidget::getInstance()->draw(&p);
	}
}

void GLCameraView::resizeEvent(QResizeEvent*)
{
	window_width = width();
	window_height = height();
	if (autozoom) setZoomToFit();
}

void GLCameraView::setMinimumWidthGL(bool set)
{
	if (set && camera) {
		double ratio = static_cast<double>(camera->getWidth()) / camera->getHeight();
		qreal dpr = this->devicePixelRatio();
		this->setMinimumWidth(static_cast<int>(ratio * this->size().height() / dpr));
	} else {
		this->setMinimumWidth(0);
	}
}

void GLCameraView::setAutoZoom(bool on)
{
	if (on) {
		setZoomRatio(zoomRatio, true);
		setZoomToFit();
	} else {
		setZoomRatio(zoomRatio, false);
	}
}

void GLCameraView::setZoom(int value)
{
	setZoomRatio(100.0 / value, autozoom);
	update();
}

void GLCameraView::setDetailedView()
{
	detailedView = true;
	setZoomRatio(0.2, false);
}

void GLCameraView::setScale(double value) { scale = value; update(); }
void GLCameraView::setBias(double value) { bias = value; update(); }
void GLCameraView::setTransparency(double value)
{
	transparency = value; if (transparency < 0.0) transparency = 0.0; if (transparency > 1.0) transparency = 1.0; update();
}
void GLCameraView::setRenderTransparentModels(bool value) { renderTransparentModels = value; update(); }

void GLCameraView::setZoomToFit()
{
	if (!camera) return;
	qreal dpr = this->devicePixelRatio();
	double effectiveWidth = window_width * dpr;
	double effectiveHeight = window_height * dpr;
	setZoomRatio(((double)camera_width) / effectiveWidth > ((double)camera_height) / effectiveHeight ? ((double)camera_width) / effectiveWidth : ((double)camera_height) / effectiveHeight, autozoom);
	x_offset = -0.5 * (zoomRatio * effectiveWidth);
	y_offset = -0.5 * (zoomRatio * effectiveHeight);
	update();
}

void GLCameraView::setZoomTo100() { setZoomRatio(1.0, autozoom); update(); }

void GLCameraView::clampXY()
{
	qreal dpr = this->devicePixelRatio();
	double effectiveWidth = window_width * dpr;
	double effectiveHeight = window_height * dpr;

	if (camera_width < effectiveWidth * zoomRatio) {
		if (x_offset < -0.5 * (zoomRatio * effectiveWidth)) x_offset = -0.5 * (zoomRatio * effectiveWidth);
		if (x_offset > -camera_width + 0.5 * (zoomRatio * effectiveWidth)) x_offset = -camera_width + 0.5 * (zoomRatio * effectiveWidth);
	} else {
		if (x_offset > -0.5 * (zoomRatio * effectiveWidth)) x_offset = -0.5 * (zoomRatio * effectiveWidth);
		if (x_offset < -camera_width + 0.5 * (zoomRatio * effectiveWidth)) x_offset = -camera_width + 0.5 * (zoomRatio * effectiveWidth);
	}

	if (camera_height < effectiveHeight * zoomRatio) {
		if (y_offset < -0.5 * (zoomRatio * effectiveHeight)) y_offset = -0.5 * (zoomRatio * effectiveHeight);
		if (y_offset > -camera_height + 0.5 * (zoomRatio * effectiveHeight)) y_offset = -camera_height + 0.5 * (zoomRatio * effectiveHeight);
	} else {
		if (y_offset > -0.5 * (zoomRatio * effectiveHeight)) y_offset = -0.5 * (zoomRatio * effectiveHeight);
		if (y_offset < -camera_height + 0.5 * (zoomRatio * effectiveHeight)) y_offset = -camera_height + 0.5 * (zoomRatio * effectiveHeight);
	}
}

void GLCameraView::mouseMoveEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::RightButton) {
		qreal dpr = this->devicePixelRatio();
		x_offset -= (prev_x - zoomRatio * e->pos().x() * dpr);
		y_offset -= (prev_y - zoomRatio * e->pos().y() * dpr);
		prev_y = zoomRatio * e->pos().y() * dpr;
		prev_x = zoomRatio * e->pos().x() * dpr;
		clampXY();
		update();
	}
}

void GLCameraView::mouseDoubleClickEvent(QMouseEvent* e)
{
	State::getInstance()->changeActiveCamera(this->camera->getID());
	if (e->buttons() & Qt::LeftButton) {
		if (State::getInstance()->getWorkspace() == CALIBRATION
			&& camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() == 1)
		{
			qreal dpr = this->devicePixelRatio();
			double x = zoomRatio * (e->pos().x() * dpr) - ((window_width * dpr) * zoomRatio * 0.5 + x_offset);
			double y = zoomRatio * (e->pos().y() * dpr) - ((window_height * dpr) * zoomRatio * 0.5 + y_offset);
			camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->toggleInlier(x, y, State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE);
			update();
		}
	}
}

void GLCameraView::mousePressEvent(QMouseEvent* e)
{
	if (!camera) {
		return;
	}
	const bool wasActive = (State::getInstance()->getActiveCamera() == camera->getID());
	State::getInstance()->changeActiveCamera(this->camera->getID());
	if (!wasActive && e->button() == Qt::LeftButton)
	{
		MainWindow::getInstance()->redrawGL();
		return;
	}
	qreal dpr = this->devicePixelRatio();
	if (e->buttons() & Qt::RightButton) {
		prev_y = zoomRatio * e->pos().y() * dpr;
		prev_x = zoomRatio * e->pos().x() * dpr;
	} else if (e->buttons() & Qt::LeftButton) {
		double x = zoomRatio * (e->pos().x() * dpr) - ((window_width * dpr) * zoomRatio * 0.5 + x_offset) - 0.5;
		double y = zoomRatio * (e->pos().y() * dpr) - ((window_height * dpr) * zoomRatio * 0.5 + y_offset) - 0.5;
		if (State::getInstance()->getWorkspace() == UNDISTORTION) {
			if (camera->hasUndistortion()) {
				int clickmode = State::getInstance()->getUndistortionMouseMode();
				if (clickmode == 1) {
					int vismode = State::getInstance()->getUndistortionVisPoints();
					if (vismode == 2 || vismode == 3 || vismode == 4) {
						camera->getUndistortionObject()->toggleOutlier(vismode, x, y);
					} else {
						ErrorDialog::getInstance()->showErrorDialog("You either have to display the grid points (distorted or undistorted) or the reference points to toggle outlier");
					}
				} else if (clickmode == 2) {
					int vismode = State::getInstance()->getUndistortionVisImage();
					if (vismode == 0) {
						camera->getUndistortionObject()->setCenter(x, y);
					} else {
						ErrorDialog::getInstance()->showErrorDialog("You have to display the distorted image to set the center");
					}
				}
			}
		} else if (State::getInstance()->getWorkspace() == CALIBRATION) {
			if (e->modifiers().testFlag(Qt::AltModifier)) {
				int method = Settings::getInstance()->getIntSetting("DetectionMethodForCalibration");
				if (method > 0) {
					if (method == 5) { method = 6; }
					else { method = method - 1; }
					cv::Point out = MarkerDetection::detectionPoint(camera->getCalibrationSequence()->getImage(State::getInstance()->getActiveFrameCalibration(), true), method, cv::Point2d(x, y), 40, 5);
					x = out.x; y = out.y;
				}
			}

			if (WizardDockWidget::getInstance()->manualCalibrationRunning()) {
				WizardDockWidget::getInstance()->addCalibrationReference(x, y);
			} else {
				if (camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() != 1) {
					WizardDockWidget::getInstance()->addCalibrationReference(x, y);
				} else {
					if (e->modifiers().testFlag(Qt::ControlModifier)) {
						camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setPointManual(x, y, State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE);
					}
				}
			}
		} else if (State::getInstance()->getWorkspace() == DIGITIZATION) {
			if (e->modifiers().testFlag(Qt::ControlModifier) && !detailedView) {
				WizardDockWidget::getInstance()->addDigitizationPoint(camera->getID(), x, y);
			} else if (e->modifiers().testFlag(Qt::ShiftModifier)) {
				WizardDockWidget::getInstance()->selectDigitizationPoint(camera->getID(), x, y);
			} else {
				WizardDockWidget::getInstance()->moveDigitizationPoint(camera->getID(), x, y, detailedView);
			}
			if (!detailedView) DetailViewDockWidget::getInstance()->centerViews();
			WizardDockWidget::getInstance()->updateDialog();
		}
		MainWindow::getInstance()->redrawGL();
	}
}

void GLCameraView::wheelEvent(QWheelEvent* e)
{
	if (!detailedView || !Settings::getInstance()->getBoolSetting("CenterDetailView")) {
		if (e->modifiers().testFlag(Qt::ControlModifier)) {
			if (State::getInstance()->getWorkspace() == DIGITIZATION) {
				setTransparency(transparency + 1.0 / 2400.0 * e->angleDelta().y());
				emit transparencyChanged(transparency);
			}
		} else {
			State::getInstance()->changeActiveCamera(this->camera->getID());
			double zoom_prev = zoomRatio;
			setZoomRatio(zoomRatio * 1 - e->angleDelta().y() / 1000.0, false);
			QPoint coordinatesGlobal = e->globalPosition().toPoint();
			QPoint coordinates = this->mapFromGlobal(coordinatesGlobal);
			qreal dpr = this->devicePixelRatio();
			y_offset += (zoom_prev - zoomRatio) * (0.5 * window_height * dpr - coordinates.y() * dpr);
			x_offset += (zoom_prev - zoomRatio) * (0.5 * window_width * dpr - coordinates.x() * dpr);
			update();
		}
	}
}

void GLCameraView::centerViewToPoint(bool resetZoom)
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION) {
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0) {
			Marker* activeMarker = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker();
			if (activeMarker != NULL) {
				double x, y;
				if (activeMarker->getStatus2D()[camera->getID()][State::getInstance()->getActiveFrameTrial()] > 0) {
					x = activeMarker->getPoints2D()[camera->getID()][State::getInstance()->getActiveFrameTrial()].x;
					y = activeMarker->getPoints2D()[camera->getID()][State::getInstance()->getActiveFrameTrial()].y;
					x_offset = -x;
					y_offset = -y;
				} else if (activeMarker->getMarkerPrediction(camera->getID(), State::getInstance()->getActiveFrameTrial(), x, y, true)) {
					x_offset = -x; y_offset = -y;
				} else if (activeMarker->getMarkerPrediction(camera->getID(), State::getInstance()->getActiveFrameTrial(), x, y, false)) {
					x_offset = -x; y_offset = -y;
				}
			}
		}
		if (resetZoom) setZoomRatio(0.2, false);
	}
}

void GLCameraView::UseStatusColors(bool value) { showStatusColors = value; }

void GLCameraView::setZoomRatio(double newZoomRation, bool newAutozoom)
{
	newZoomRation = (newZoomRation > 100.0 / 999.0) ? newZoomRation : 100.0 / 999.0;
	if (zoomRatio != newZoomRation) {
		zoomRatio = newZoomRation;
		int newZoom = floor(100.0 / zoomRatio + 0.5);
		emit zoomChanged(newZoom);
	}
	if (autozoom != newAutozoom) {
		autozoom = newAutozoom;
		emit autozoomChanged(autozoom);
	}
}

#else // !XMA_USE_PAINTER

// Non-macOS implementation only
void GLCameraView::clampXY()
{
	qreal devicePixelRatio = this->devicePixelRatio();
	double effectiveWidth = window_width * devicePixelRatio;
	double effectiveHeight = window_height * devicePixelRatio;
    
	if (camera_width < effectiveWidth * zoomRatio)
	{
		if (x_offset < -0.5 * (zoomRatio * effectiveWidth)) x_offset = -0.5 * (zoomRatio * effectiveWidth);
		if (x_offset > -camera_width + 0.5 * (zoomRatio * effectiveWidth)) x_offset = -camera_width + 0.5 * (zoomRatio * effectiveWidth);
	}
	else
	{
		if (x_offset > -0.5 * (zoomRatio * effectiveWidth)) x_offset = -0.5 * (zoomRatio * effectiveWidth);
		if (x_offset < -camera_width + 0.5 * (zoomRatio * effectiveWidth)) x_offset = -camera_width + 0.5 * (zoomRatio * effectiveWidth);
	}

	if (camera_height < effectiveHeight * zoomRatio)
	{
		if (y_offset < -0.5 * (zoomRatio * effectiveHeight)) y_offset = -0.5 * (zoomRatio * effectiveHeight);
		if (y_offset > -camera_height + 0.5 * (zoomRatio * effectiveHeight)) y_offset = -camera_height + 0.5 * (zoomRatio * effectiveHeight);
	}
	else
	{
		if (y_offset > -0.5 * (zoomRatio * effectiveHeight)) y_offset = -0.5 * (zoomRatio * effectiveHeight);
		if (y_offset < -camera_height + 0.5 * (zoomRatio * effectiveHeight)) y_offset = -camera_height + 0.5 * (zoomRatio * effectiveHeight);
	}
}

void GLCameraView::mouseMoveEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::RightButton)
	{
		qreal devicePixelRatio = this->devicePixelRatio();
		x_offset -= (prev_x - zoomRatio * e->pos().x() * devicePixelRatio);
		y_offset -= (prev_y - zoomRatio * e->pos().y() * devicePixelRatio);

		prev_y = zoomRatio * e->pos().y() * devicePixelRatio;
		prev_x = zoomRatio * e->pos().x() * devicePixelRatio;

		clampXY();
		update();
	}
}

void GLCameraView::mouseDoubleClickEvent(QMouseEvent* e)
{
	State::getInstance()->changeActiveCamera(this->camera->getID());
	if (e->buttons() & Qt::LeftButton)
	{
		if (State::getInstance()->getWorkspace() == CALIBRATION
			&& camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() == 1)
		{
			qreal devicePixelRatio = this->devicePixelRatio();
			double x = zoomRatio * (e->pos().x() * devicePixelRatio) - ((window_width * devicePixelRatio) * zoomRatio * 0.5 + x_offset);
			double y = zoomRatio * (e->pos().y() * devicePixelRatio) - ((window_height * devicePixelRatio) * zoomRatio * 0.5 + y_offset);
			camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->toggleInlier(x, y, State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE);
			update();
		}
	}
}

void GLCameraView::mousePressEvent(QMouseEvent* e)
{
	if (!camera) {
		return;
	}
	const bool wasActive = (State::getInstance()->getActiveCamera() == camera->getID());
	State::getInstance()->changeActiveCamera(this->camera->getID());
	if (!wasActive && e->button() == Qt::LeftButton)
	{
		MainWindow::getInstance()->redrawGL();
		return;
	}
	qreal devicePixelRatio = this->devicePixelRatio();
	
	if (e->buttons() & Qt::RightButton)
	{
		prev_y = zoomRatio * e->pos().y() * devicePixelRatio;
		prev_x = zoomRatio * e->pos().x() * devicePixelRatio;
	}
	else if (e->buttons() & Qt::LeftButton)
	{
		double x = zoomRatio * (e->pos().x() * devicePixelRatio) - ((window_width * devicePixelRatio) * zoomRatio * 0.5 + x_offset) - 0.5;
		double y = zoomRatio * (e->pos().y() * devicePixelRatio) - ((window_height * devicePixelRatio) * zoomRatio * 0.5 + y_offset) - 0.5;
		if (State::getInstance()->getWorkspace() == UNDISTORTION)
		{
			if (camera->hasUndistortion())
			{
				int clickmode = State::getInstance()->State::getInstance()->getUndistortionMouseMode();
				if (clickmode == 1)
				{
					int vismode = State::getInstance()->getUndistortionVisPoints();
					if (vismode == 2 || vismode == 3 || vismode == 4)
					{
						camera->getUndistortionObject()->toggleOutlier(vismode, x, y);
					}
					else
					{
						ErrorDialog::getInstance()->showErrorDialog("You either have to display the grid points (distorted or undistorted) or the reference points to toggle outlier");
					}
				}
				else if (clickmode == 2)
				{
					int vismode = State::getInstance()->getUndistortionVisImage();
					if (vismode == 0)
					{
						camera->getUndistortionObject()->setCenter(x, y);
					}
					else
					{
						//Fix also allow undistorted selection
						ErrorDialog::getInstance()->showErrorDialog("You have to display the distorted image to set the center");
					}
				}
			}
		}
		else if (State::getInstance()->getWorkspace() == CALIBRATION)
		{
			if (e->modifiers().testFlag(Qt::AltModifier))
			{
				int method = Settings::getInstance()->getIntSetting("DetectionMethodForCalibration");
				if (method > 0)
				{
					if (method == 5)
					{
						method = 6;// white blobs
					}
					else
					{
						method = method - 1; // different ordering than normal detection
					}
					cv::Point out = MarkerDetection::detectionPoint(camera->getCalibrationSequence()->getImage(State::getInstance()->getActiveFrameCalibration(), true), method, cv::Point2d(x, y), 40, 5);
					x = out.x;
					y = out.y;
				}
			}

			if (WizardDockWidget::getInstance()->manualCalibrationRunning())
			{
				WizardDockWidget::getInstance()->addCalibrationReference(x, y);
				//updateGL();
			}
			else
			{
				if (camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() != 1)
				{
					WizardDockWidget::getInstance()->addCalibrationReference(x, y);
				}
				else
				{
					if (e->modifiers().testFlag(Qt::ControlModifier))
					{
						camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setPointManual(x, y, State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE);
						//updateGL();
					}
				}
			}
		}
		else if (State::getInstance()->getWorkspace() == DIGITIZATION)
		{
			if (e->modifiers().testFlag(Qt::ControlModifier) && !detailedView)
			{
				WizardDockWidget::getInstance()->addDigitizationPoint(camera->getID(), x, y);
			}
			else if (e->modifiers().testFlag(Qt::ShiftModifier))
			{
				WizardDockWidget::getInstance()->selectDigitizationPoint(camera->getID(), x, y);
			}
			else
			{
				WizardDockWidget::getInstance()->moveDigitizationPoint(camera->getID(), x, y, detailedView);
			}

			if (!detailedView) DetailViewDockWidget::getInstance()->centerViews();

			WizardDockWidget::getInstance()->updateDialog();
		}
		MainWindow::getInstance()->redrawGL();
	}
}

void GLCameraView::wheelEvent(QWheelEvent* e)
{
	if (!detailedView || !Settings::getInstance()->getBoolSetting("CenterDetailView"))
	{
		if (e->modifiers().testFlag(Qt::ControlModifier))
		{
			if (State::getInstance()->getWorkspace() == DIGITIZATION){
				setTransparency(transparency + 1.0 / 2400.0 * e->angleDelta().y());
				emit transparencyChanged(transparency);
			}
		}
		else{
			State::getInstance()->changeActiveCamera(this->camera->getID());
			double zoom_prev = zoomRatio;

			setZoomRatio(zoomRatio * 1 - e->angleDelta().y() / 1000.0, false);

			QPoint coordinatesGlobal = e->globalPosition().toPoint();
			QPoint coordinates = this->mapFromGlobal(coordinatesGlobal);
			qreal devicePixelRatio = this->devicePixelRatio();

			y_offset += (zoom_prev - zoomRatio) * (0.5 * window_height * devicePixelRatio - coordinates.y() * devicePixelRatio);
			x_offset += (zoom_prev - zoomRatio) * (0.5 * window_width * devicePixelRatio - coordinates.x() * devicePixelRatio);

			update();
		}
	}
}

void GLCameraView::initializeGL()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Black Background
	glClearDepth(1.0f); // Depth Buffer Setup
	glDisable(GL_DEPTH_TEST);
}

void GLCameraView::setAutoZoom(bool on)
{
	if (on)
	{
		setZoomRatio(zoomRatio, true);
		setZoomToFit();
	}
	else
	{
		setZoomRatio(zoomRatio, false);
	}
}

void GLCameraView::setZoom(int value)
{
	setZoomRatio(100.0 / value, autozoom);
	update();
}

void GLCameraView::setDetailedView()
{
	detailedView = true;
	setZoomRatio(0.2, false);
}

void GLCameraView::setScale(double value)
{
	scale = value;
	update();
}

void GLCameraView::setBias(double value)
{
	bias = value;
	update();
}

void GLCameraView::setTransparency(double value)
{
	
	transparency = value;

	if (transparency < 0.0)  transparency = 0.0;

	if (transparency > 1.0)  transparency = 1.0;

	update();
	update();
}

void GLCameraView::setRenderTransparentModels(bool value)
{
	renderTransparentModels = value;
	update();
}

void GLCameraView::setZoomToFit()
{
	qreal devicePixelRatio = this->devicePixelRatio();
	double effectiveWidth = window_width * devicePixelRatio;
	double effectiveHeight = window_height * devicePixelRatio;
	
	setZoomRatio((((double) camera_width) / effectiveWidth > ((double) camera_height) / effectiveHeight) ? ((double) camera_width) / effectiveWidth : ((double) camera_height) / effectiveHeight, autozoom);

	x_offset = -0.5 * (zoomRatio * effectiveWidth);
	y_offset = -0.5 * (zoomRatio * effectiveHeight);

	update();
	update();
}

void GLCameraView::setZoomTo100()
{
	setZoomRatio(1.0, autozoom);
	update();
}

void GLCameraView::setMinimumWidthGL(bool set)
{
	if (set)
	{
		double ratio = camera->getWidth() / camera->getHeight();
		qreal devicePixelRatio = this->devicePixelRatio();
		this->setMinimumWidth(ratio * this->size().height() / devicePixelRatio);
	}
	else
	{
		this->setMinimumWidth(0);
	}
}

void GLCameraView::resizeGL(int _w, int _h)
{
	window_width = _w;
	window_height = _h;

	// Handle high DPI displays by using device pixel ratio
	qreal devicePixelRatio = this->devicePixelRatio();
	glViewport(0, 0, window_width * devicePixelRatio, window_height * devicePixelRatio);
	if (autozoom)setZoomToFit();
	if (autozoom)setZoomToFit();
}

void GLCameraView::transformTextPos(GLdouble out[4], const GLdouble m[16], const GLdouble in[4])
{
#define M(row,col)  m[col*4+row]
	out[0] =
		M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * in[3];
	out[1] =
		M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * in[3];
	out[2] =
		M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * in[3];
	out[3] =
		M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * in[3];
#undef M
}

bool GLCameraView::projectTextPos(GLdouble objx, GLdouble objy, GLdouble objz,
	const GLdouble model[16], const GLdouble proj[16],
	const GLint viewport[4],
	GLdouble * winx, GLdouble * winy, GLdouble * winz)
{
	GLdouble in[4], out[4];

	in[0] = objx;
	in[1] = objy;
	in[2] = objz;
	in[3] = 1.0;
	transformTextPos(out, model, in);
	transformTextPos(in, proj, out);

	if (in[3] == 0.0)
		return GL_FALSE;

	in[0] /= in[3];
	in[1] /= in[3];
	in[2] /= in[3];

	*winx = viewport[0] + (1 + in[0]) * viewport[2] / 2;
	*winy = viewport[1] + (1 + in[1]) * viewport[3] / 2;

	*winz = (1 + in[2]) / 2;
	return GL_TRUE;
}

void GLCameraView::renderText(double x, double y, double z, const QString &str, QColor fontColor , const QFont & font) {
	int width = this->width();
	int height = this->height();

	GLdouble model[4][4], proj[4][4];
	GLint view[4];
	glGetDoublev(GL_MODELVIEW_MATRIX, &model[0][0]);
	glGetDoublev(GL_PROJECTION_MATRIX, &proj[0][0]);
	glGetIntegerv(GL_VIEWPORT, &view[0]);
	GLdouble textPosX = 0, textPosY = 0, textPosZ = 0;

	if (!projectTextPos(x, y, z,
		&model[0][0], &proj[0][0], &view[0],
		&textPosX, &textPosY, &textPosZ))
		return;

	// Handle high DPI displays: convert from device coordinates to widget coordinates
	qreal devicePixelRatio = this->devicePixelRatio();
	textPosX /= devicePixelRatio;
	textPosY /= devicePixelRatio;
	
	textPosY = height - textPosY; // y is inverted

	QPainter painter(this);
	painter.setPen(fontColor);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
	painter.drawText(textPosX, textPosY, str); // z = pointT4.z + distOverOp / 4
	painter.end();
	painter.end();
}

void GLCameraView::renderTextCentered(QString string)
{
	setFont(QFont(this->font().family(), 24));
	QFontMetrics fm(this->font());
	renderText(-zoomRatio * fm.horizontalAdvance(string) * 0.5 - x_offset,
               -zoomRatio * fm.height() * 0.5 - y_offset,
               0.0, string, QColor(255, 0, 0));
}

void GLCameraView::renderPointText(bool calibration)
{
	std::vector<double> x;
	std::vector<double> y;
	std::vector<QString> text;
	if (calibration)
	{
		std::vector<bool> inlier;
		camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getDrawTextData(
			((!camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() == 1)
				&& WizardDockWidget::getInstance()->manualCalibrationRunning()) ? IDCALIBTEXT : State::getInstance()->getCalibrationVisText(), State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE, x, y, text, inlier);
		setFont(QFont(this->font().family(), 15.0));
		QFontMetrics fm(this->font());

		if (State::getInstance()->getCalibrationVisText() == 4) {
			for (unsigned int i = 0; i < x.size(); i++)
			{
				text[i] = CalibrationObject::getInstance()->getMarkerNames()[i];
			}
		}

		if (State::getInstance()->getCalibrationVisText() != 3)
		{
			for (unsigned int i = 0; i < x.size(); i++)
			{
				QColor color;
				if (inlier[i])
				{
					color =  QColor(0, 255, 0);
				}
				else
				{
					color = QColor(255, 0, 0);
				}
				renderText(x[i] + 3, y[i], 0.0, text[i], color);
			}
		}
		else
		{
			double max = 0;
			for (unsigned int i = 0; i < text.size(); i++)
			{
				if (text[i].toDouble() > max) max = text[i].toDouble();
			}
			for (unsigned int i = 0; i < x.size(); i++)
			{
				double val = text[i].toDouble();
				renderText(x[i] + 3, y[i], 0.0, text[i], QColor(255.0 * val / max, 255.0 * (1.0 - val / max), 0));
			}
		}
		inlier.clear();
	}
	else
	{
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getDrawTextData(this->camera->getID(), State::getInstance()->getActiveFrameTrial(), x, y, text);
		setFont(QFont(this->font().family(), 15.0));
		QFontMetrics fm(this->font());
		int active = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
		for (unsigned int i = 0; i < x.size(); i++)
		{
			QColor color;
			if (Settings::getInstance()->getBoolSetting("ShowColoredMarkerIDs"))
			{
				color = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->getStatusColor(this->camera->getID(), State::getInstance()->getActiveFrameTrial());
			}
			else{
				if (active == i)
				{
					color = QColor(255, 0, 0);
				}
				else
				{
					color = QColor(0, 255, 0);
				}
			}
			renderText(x[i] + 3, y[i], 0.0, text[i], color);
		}
	}

	x.clear();
	y.clear();
	text.clear();
	text.clear();
}

void GLCameraView::drawTexture()
{
	bool drawImage = true;
	glEnable(GL_TEXTURE_2D);
	if (State::getInstance()->getWorkspace() == UNDISTORTION)
	{
		if (camera->hasUndistortion())
		{
			camera->getUndistortionObject()->bindTexture(State::getInstance()->getUndistortionVisImage());
		}
		else
		{
			drawImage = false;
			renderTextCentered("No undistortion grid loaded");
		}
	}
	else if ((State::getInstance()->getWorkspace() == CALIBRATION) && (Project::getInstance()->getCalibration() == INTERNAL))
	{
		camera->getCalibrationSequence()->bindTexture(State::getInstance()->getActiveFrameCalibration(),State::getInstance()->getCalibrationVisImage());
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			if (!Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getIsDefault())
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getVideoStreams()[camera->getID()]->bindTexture();
		}
	}
	if (drawImage)
	{
		glDisable(GL_LIGHTING);
		if (State::getInstance()->getWorkspace() != DIGITIZATION ||( State::getInstance()->getActiveTrial() != -1 && !Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getIsDefault()))
			drawQuad();
	}
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GLCameraView::drawQuad()
{
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(-0.5, -0.5);
	glTexCoord2f(0, 1);
	glVertex2f(-0.5, camera_height - 0.5);
	glTexCoord2f(1, 1);
	glVertex2f(camera_width - 0.5, camera_height - 0.5);
	glTexCoord2f(1, 0);
	glVertex2f(camera_width - 0.5, -0.5);
	glEnd();
	glEnd();
}

void GLCameraView::centerViewToPoint(bool resetZoom)
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			Marker* activeMarker = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker();
			if (activeMarker != NULL)
			{
				double x, y;
				if (activeMarker->getStatus2D()[camera->getID()][State::getInstance()->getActiveFrameTrial()] > 0)
				{
					x = activeMarker->getPoints2D()[camera->getID()][State::getInstance()->getActiveFrameTrial()].x;
					y = activeMarker->getPoints2D()[camera->getID()][State::getInstance()->getActiveFrameTrial()].y;
					x_offset = -x;
					y_offset = -y;
				}
				else if (activeMarker->getMarkerPrediction(camera->getID(), State::getInstance()->getActiveFrameTrial(), x, y, true))
				{
					x_offset = -x;
					y_offset = -y;
				}
				else if (activeMarker->getMarkerPrediction(camera->getID(), State::getInstance()->getActiveFrameTrial(), x, y, false))
				{
					x_offset = -x;
					y_offset = -y;
				}
			}
		}
		if (resetZoom) setZoomRatio(0.2, false);
	}
}

void GLCameraView::UseStatusColors(bool value)
{
	showStatusColors = value;
}


void GLCameraView::paintGL()
{
	doDistortion = false;
	renderMeshes = false;

	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			if (!detailedView && Settings::getInstance()->getBoolSetting("TrialDrawRigidBodyMeshmodels") && !Settings::getInstance()->getBoolSetting("TrialDrawHideAll"))
			{
				renderMeshes = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->renderMeshes() && 
					!Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getIsDefault();
			}
		}

		renderMeshes = renderMeshes && (Project::getInstance()->getCalibration() != NO_CALIBRATION);

		if (renderMeshes){
			if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
			{
				if (!detailedView)
				{
					if (rigidbodyBufferUndistorted)
					{
						rigidbodyBufferUndistorted->bindFrameBuffer();

						glClearColor(1.0, 1.0, 1.0, 0.0);
						glClearDepth(1.0f);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						glEnable(GL_DEPTH_TEST); // Enables Depth Testing
						glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Really Nice Perspective Calculations
						if (renderTransparentModels){
							glDepthFunc(GL_ALWAYS); // The Type Of Depth Testing To Do
							glEnable(GL_BLEND);
							glBlendFunc(GL_ZERO, GL_SRC_COLOR);
						}
						else
						{
							glDepthFunc(GL_LEQUAL);
						}
						glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient); // Setup The Ambient Light
						glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse); // Setup The Diffuse Light
						glLightfv(GL_LIGHT1, GL_POSITION, LightPosition_front); // Position The Light
						glEnable(GL_LIGHT1);
						glLightfv(GL_LIGHT2, GL_AMBIENT, LightAmbient); // Setup The Ambient Light
						glLightfv(GL_LIGHT2, GL_DIFFUSE, LightDiffuse); // Setup The Diffuse Light
						glLightfv(GL_LIGHT2, GL_POSITION, LightPosition_back); // Position The Light
						glEnable(GL_LIGHT2);

						glEnable(GL_LIGHTING);

						glEnable(GL_COLOR_MATERIAL);
						glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

						glViewport(0, 0, rigidbodyBufferUndistorted->getWidth(), rigidbodyBufferUndistorted->getHeight());
						double proj[16];
						double model[16];

						camera->getGLTransformations(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getReferenceCalibrationImage(), &proj[0], &model[0]);

						glMatrixMode(GL_PROJECTION);
						glLoadMatrixd(&proj[0]);

						glMatrixMode(GL_MODELVIEW);
						glLoadMatrixd(&model[0]);

						Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->drawRigidBodiesMesh();

						glDisable(GL_BLEND);
						glDisable(GL_LIGHTING);
						glDisable(GL_DEPTH_TEST);
						rigidbodyBufferUndistorted->unbindFrameBuffer();
					}

					distortionShader->draw(rigidbodyBufferUndistorted->getTextureID(), rigidbodyBufferUndistorted->getDepthTextureID(), transparency);
					doDistortion = distortionShader->canRender();
				}
			}
		}
	}
	qreal devicePixelRatio = this->devicePixelRatio();
	glViewport(0, 0, window_width * devicePixelRatio, window_height * devicePixelRatio);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (detailedView && Settings::getInstance()->getBoolSetting("CenterDetailView")) centerViewToPoint();

	double effectiveWidth = window_width * devicePixelRatio;
	double effectiveHeight = window_height * devicePixelRatio;
	
	glOrtho(-0.5 * (zoomRatio * effectiveWidth) - x_offset - 0.5,
	        0.5 * (zoomRatio * effectiveWidth) - x_offset - 0.5,
	        0.5 * (zoomRatio * effectiveHeight) - y_offset - 0.5,
	        -0.5 * (zoomRatio * effectiveHeight) - y_offset - 0.5,
	        -1000, 1000);

	gluLookAt(0, 0, 1, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glColor3f(1.0, 1.0, 1.0);
	drawTexture();

	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (Settings::getInstance()->getBoolSetting("TrialDrawFiltered") && (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getCutoffFrequency() <= 0 || Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() <= 0)){
		 setFont(QFont(this->font().family(), 12));
		 QFontMetrics fm(this->font());
		 QString string("Rendering of filtered data is enabled, but framrate and cutoff are not set correctly");
		 renderText(-zoomRatio * fm.horizontalAdvance(string) * 0.5 - x_offset,
           -zoomRatio * (fm.height() - window_height) * 0.5 - y_offset,
           0.0, string, QColor(255, 0, 0));
		}

		if (renderMeshes){
			if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
			{
				if (!detailedView)
				{
					if (!doDistortion){
						setFont(QFont(this->font().family(), 12));
						QFontMetrics fm(this->font());
						QString string("Rigid body models are not distorted! XMALab is currently computing the distortion!");
						renderText(-zoomRatio * fm.horizontalAdvance(string) * 0.5 - x_offset,
           -zoomRatio * (fm.height() - window_height) * 0.5 - y_offset,
           0.0, string, QColor(255, 0, 0));
						blendShader->draw(camera_width, camera_height, transparency, rigidbodyBufferUndistorted->getTextureID(), rigidbodyBufferUndistorted->getDepthTextureID(), true);
					}
					else
					{
						blendShader->draw(camera_width, camera_height, transparency, distortionShader->getTextureID(), distortionShader->getDepthTextureID(), false);
					}
				}
			}
		}
	}

	if ((bias != 0.0 || scale != 1.0) && GLSharedWidget::getInstance()->getHasBlendSubtract())
	{
		/* NOTE: The blending approach does not allow negative
		scales.  The blending approach also fails if the
		partial scaling or biasing results leave the 0.0 to
		1.0 range (example, scale=5.47, bias=-1.2). */

		glEnable(GL_BLEND);
		if (scale > 1.0)
		{
			float remainingScale;

			remainingScale = scale;
			if (GLSharedWidget::getInstance()->getHasBlendExt())
			{
				glBlendEquationEXT(GL_FUNC_ADD_EXT);
			}
			glBlendFunc(GL_DST_COLOR, GL_ONE);
			if (remainingScale > 2.0)
			{
				/* Clever cascading approach.  Example: if the
				scaling factor was 9.5, do 3 "doubling" blends
				(8x), then scale by the remaining 1.1875. */
				glColor4f(1, 1, 1, 1);
				while (remainingScale > 2.0)
				{
					drawQuad();
					remainingScale /= 2.0;
				}
			}
			glColor4f(remainingScale - 1, remainingScale - 1, remainingScale - 1, 1);
			drawQuad();
			glBlendFunc(GL_ONE, GL_ONE);
			if (bias != 0)
			{
				if (bias > 0)
				{
					glColor4f(bias, bias, bias, 0.0);
				}
				else
				{
					if (GLSharedWidget::getInstance()->getHasBlendSubtract())
					{
						glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
					}
					glColor4f(-bias, -bias, -bias, 0.0);
				}
				drawQuad();
			}
		}
		else
		{
			if (bias > 0)
			{
				if (GLSharedWidget::getInstance()->getHasBlendExt())
				{
					glBlendEquationEXT(GL_FUNC_ADD_EXT);
				}
				glColor4f(bias, bias, bias, scale);
			}
			else
			{
				if (GLSharedWidget::getInstance()->getHasBlendSubtract())
				{
					glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
				}
				glColor4f(-bias, -bias, -bias, scale);
			}
			glBlendFunc(GL_ONE, GL_SRC_ALPHA);
			drawQuad();
		}
		glDisable(GL_BLEND);
	}
	if (State::getInstance()->getWorkspace() == UNDISTORTION)
	{
		if (camera->hasUndistortion())
		{
			camera->getUndistortionObject()->drawData(State::getInstance()->getUndistortionVisPoints());
		}
	}
	else if ((State::getInstance()->getWorkspace() == CALIBRATION && Project::getInstance()->getCalibration() == INTERNAL))
	{
		camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->draw(State::getInstance()->getCalibrationVisPoints());
		if (State::getInstance()->getCalibrationVisText() > 0 ||
			((!camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() == 1)
				&& WizardDockWidget::getInstance()->manualCalibrationRunning()))
		{
			renderPointText(true);
		}
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (!Settings::getInstance()->getBoolSetting("TrialDrawHideAll")){
			if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
			{

				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->drawPoints(this->camera->getID(), detailedView);

				if (!detailedView)
				{
					if (Settings::getInstance()->getBoolSetting("TrialDrawRigidBodyConstellation"))
						Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->drawRigidBodies(this->camera);
				}
				if (!detailedView || Settings::getInstance()->getBoolSetting("ShowIDsInDetail"))
				{
					if (Settings::getInstance()->getBoolSetting("TrialDrawMarkerIds"))
					{
						renderPointText(false);
					}
				}
			}
		}
	}
	if (State::getInstance()->getActiveCamera() == this->camera->getID())
	{
		WizardDockWidget::getInstance()->draw();
	}

	glFlush();
}

void GLCameraView::setZoomRatio(double newZoomRation, bool newAutozoom)
{
	newZoomRation = (newZoomRation > 100.0 / 999.0) ? newZoomRation : 100.0 / 999.0;
	if (zoomRatio != newZoomRation)
	{
		zoomRatio = newZoomRation;
		int newZoom = floor(100.0 / zoomRatio + 0.5);
		emit zoomChanged(newZoom);
	}
	if (autozoom != newAutozoom)
	{
		autozoom = newAutozoom;
		emit autozoomChanged(autozoom);
	}
}

#endif // !XMA_USE_PAINTER

