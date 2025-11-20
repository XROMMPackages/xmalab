//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
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
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//  ----------------------------------
//
///\file WorldViewDockGLWidget.cpp
///\author Benjamin Knorlein (original OpenGL implementation)
///\author GitHub Copilot (Qt Quick integration)

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "ui/WorldViewDockGLWidget.h"

#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QMatrix3x3>
#include <QQuaternion>
#include <QDebug>
#include <QQmlContext>
#include <QQuickView>
#include <QUrl>
#include <QVariant>
#include <QVector3D>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QtGlobal>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

#include <opencv2/opencv.hpp>

#include "core/Camera.h"
#include "core/CalibrationImage.h"
#include "core/Marker.h"
#include "core/Project.h"
#include "core/RigidBody.h"
#include "core/Settings.h"
#include "core/Trial.h"
#include "ui/State.h"

namespace
{
constexpr const char* kWorldViewQmlUrl = "qrc:/qml/WorldViewScene.qml";

constexpr float kDefaultSceneRadius = 10.0f;
constexpr float kMinSceneRadius = 1.0f;

#ifdef XMA_ENABLE_QUICK3D_WORLD_VIEW
struct SceneAccumulator
{
	QVector3D min{0.0f, 0.0f, 0.0f};
	QVector3D max{0.0f, 0.0f, 0.0f};
	bool initialized{false};

	void include(const QVector3D& point)
	{
		if (!std::isfinite(point.x()) || !std::isfinite(point.y()) || !std::isfinite(point.z()))
		{
			return;
		}
		if (!initialized)
		{
			min = max = point;
			initialized = true;
		}
		else
		{
			min.setX(std::min(min.x(), point.x()));
			min.setY(std::min(min.y(), point.y()));
			min.setZ(std::min(min.z(), point.z()));
			max.setX(std::max(max.x(), point.x()));
			max.setY(std::max(max.y(), point.y()));
			max.setZ(std::max(max.z(), point.z()));
		}
	}

	QVector3D center() const
	{
		return initialized ? (min + max) * 0.5f : QVector3D();
	}

	float radius() const
	{
		if (!initialized)
		{
			return kDefaultSceneRadius;
		}
		const float candidate = (max - min).length() * 0.5f;
		if (!std::isfinite(candidate) || candidate < kMinSceneRadius)
		{
			return kDefaultSceneRadius;
		}
		return candidate;
	}
};

static inline bool isValidPoint(const cv::Point3d& point)
{
	return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

static inline QVector3D toVector3D(const cv::Point3d& point)
{
	return QVector3D(static_cast<float>(point.x), static_cast<float>(point.y), static_cast<float>(point.z));
}

static QVariantMap buildCameraEntry(xma::Camera* camera, int referenceIndex, SceneAccumulator& bounds)
{
	if (!camera || !camera->isCalibrated() || !camera->isVisible())
	{
		return QVariantMap();
	}

	const auto& calibrationImages = camera->getCalibrationImages();
	if (calibrationImages.empty())
	{
		return QVariantMap();
	}

	if (referenceIndex < 0 || referenceIndex >= static_cast<int>(calibrationImages.size()))
	{
		referenceIndex = 0;
	}

	xma::CalibrationImage* calibrationImage = calibrationImages[static_cast<size_t>(referenceIndex)];
	if (!calibrationImage)
	{
		return QVariantMap();
	}

	cv::Mat rotationVector = calibrationImage->getRotationVector();
	cv::Mat translationVector = calibrationImage->getTranslationVector();
	if (rotationVector.empty() || translationVector.empty())
	{
		return QVariantMap();
	}

	cv::Mat rotationMatrix;
	cv::Rodrigues(rotationVector, rotationMatrix);
	cv::Mat rotationMatrixT = rotationMatrix.t();

	cv::Mat cameraCenterMat = -rotationMatrixT * translationVector;
	cv::Point3d cameraCenter(cameraCenterMat.at<double>(0, 0), cameraCenterMat.at<double>(1, 0), cameraCenterMat.at<double>(2, 0));
	if (!isValidPoint(cameraCenter))
	{
		return QVariantMap();
	}

	const QVector3D position = toVector3D(cameraCenter);
	bounds.include(position);

	QMatrix3x3 orientationMatrix;
	for (int row = 0; row < 3; ++row)
	{
		for (int column = 0; column < 3; ++column)
		{
			orientationMatrix(row, column) = static_cast<float>(rotationMatrixT.at<double>(row, column));
		}
	}
	const QQuaternion orientation = QQuaternion::fromRotationMatrix(orientationMatrix);

	QVariantMap entry;
	entry.insert(QStringLiteral("name"), camera->getName());
	entry.insert(QStringLiteral("id"), camera->getID());
	entry.insert(QStringLiteral("position"), QVariant::fromValue(position));
	entry.insert(QStringLiteral("orientation"), QVariant::fromValue(orientation));
	entry.insert(QStringLiteral("color"), QVariant::fromValue(QColor(0x3F, 0xA8, 0xFF)));
	entry.insert(QStringLiteral("visible"), camera->isVisible());
	return entry;
}
#endif // XMA_ENABLE_QUICK3D_WORLD_VIEW
}

namespace xma
{
class WorldViewDockGLWidget::ViewModel : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int frame READ frame WRITE setFrame NOTIFY frameChanged)
	Q_PROPERTY(bool useCustomTimeline READ useCustomTimeline WRITE setUseCustomTimeline NOTIFY useCustomTimelineChanged)
	Q_PROPERTY(double focalPlaneDistance READ focalPlaneDistance WRITE setFocalPlaneDistance NOTIFY focalPlaneDistanceChanged)
	Q_PROPERTY(int workspace READ workspace NOTIFY workspaceChanged)
	Q_PROPERTY(bool quick3dEnabled READ quick3dEnabled CONSTANT)
	Q_PROPERTY(QVariantList cameras READ cameras NOTIFY camerasChanged)
	Q_PROPERTY(QVariantList markers READ markers NOTIFY markersChanged)
	Q_PROPERTY(QVariantList rigidPoints READ rigidPoints NOTIFY rigidPointsChanged)
	Q_PROPERTY(QVariantList rigidMeshes READ rigidMeshes NOTIFY rigidMeshesChanged)
	Q_PROPERTY(QVector3D sceneCenter READ sceneCenter NOTIFY sceneBoundsChanged)
	Q_PROPERTY(double sceneRadius READ sceneRadius NOTIFY sceneBoundsChanged)
	Q_PROPERTY(bool hasSceneData READ hasSceneData NOTIFY sceneChanged)
public:
	explicit ViewModel(QObject* parent = nullptr);

	int frame() const { return m_frame; }
	bool useCustomTimeline() const { return m_useCustomTimeline; }
	double focalPlaneDistance() const { return m_focalPlaneDistance; }
	int workspace() const { return static_cast<int>(State::getInstance()->getWorkspace()); }

	bool quick3dEnabled() const
	{
#ifdef XMA_ENABLE_QUICK3D_WORLD_VIEW
		return true;
#else
		return false;
#endif
	}

	const QVariantList& cameras() const { return m_cameras; }
	const QVariantList& markers() const { return m_markers; }
	const QVariantList& rigidPoints() const { return m_rigidPoints; }
	const QVariantList& rigidMeshes() const { return m_rigidMeshes; }
	QVector3D sceneCenter() const { return m_sceneCenter; }
	double sceneRadius() const { return m_sceneRadius; }
	bool hasSceneData() const { return m_hasSceneData; }

	void setFrame(int value);
	void setUseCustomTimeline(bool value);
	void setFocalPlaneDistance(double value);

signals:
	void frameChanged(int frame);
	void useCustomTimelineChanged(bool useCustomTimeline);
	void focalPlaneDistanceChanged(double focalPlaneDistance);
	void workspaceChanged(int workspace);
	void camerasChanged();
	void markersChanged();
	void rigidPointsChanged();
	void rigidMeshesChanged();
	void sceneBoundsChanged();
	void sceneChanged();

private:
	int clampFrameToActiveTrial(int value) const;
	void updateSceneData();
	void clearSceneData();

	int m_frame;
	bool m_useCustomTimeline;
	double m_focalPlaneDistance;

	QVariantList m_cameras;
	QVariantList m_markers;
	QVariantList m_rigidPoints;
	QVariantList m_rigidMeshes;
	QVector3D m_sceneCenter;
	double m_sceneRadius;
	bool m_hasSceneData;
};
} // namespace xma

using namespace xma;

WorldViewDockGLWidget::ViewModel::ViewModel(QObject* parent)
	: QObject(parent),
	  m_frame(0),
	  m_useCustomTimeline(false),
	  m_focalPlaneDistance(200.0),
	  m_sceneCenter(QVector3D()),
	  m_sceneRadius(kDefaultSceneRadius),
	  m_hasSceneData(false)
{
	State* state = State::getInstance();
	connect(state, &State::workspaceChanged, this, [this](work_state) {
		emit workspaceChanged(workspace());
		updateSceneData();
	});
	connect(state, &State::activeTrialChanged, this, [this](int) {
		updateSceneData();
	});
	connect(state, &State::activeFrameTrialChanged, this, [this](int activeFrame) {
		if (!m_useCustomTimeline)
		{
			setFrame(activeFrame);
		}
	});

	updateSceneData();
}

void WorldViewDockGLWidget::ViewModel::setFrame(int value)
{
	const int clamped = clampFrameToActiveTrial(value);
	m_frame = clamped;
	emit frameChanged(m_frame);
	updateSceneData();
}

void WorldViewDockGLWidget::ViewModel::setUseCustomTimeline(bool value)
{
	if (m_useCustomTimeline == value)
	{
		return;
	}

	m_useCustomTimeline = value;
	emit useCustomTimelineChanged(m_useCustomTimeline);

	if (!m_useCustomTimeline)
	{
		setFrame(State::getInstance()->getActiveFrameTrial());
	}
	else
	{
		updateSceneData();
	}
}

void WorldViewDockGLWidget::ViewModel::setFocalPlaneDistance(double value)
{
	if (qFuzzyCompare(1.0 + m_focalPlaneDistance, 1.0 + value))
	{
		return;
	}
	m_focalPlaneDistance = value;
	emit focalPlaneDistanceChanged(m_focalPlaneDistance);
}

int WorldViewDockGLWidget::ViewModel::clampFrameToActiveTrial(int value) const
{
	if (value < 0)
	{
		value = 0;
	}

	Project* project = Project::getInstance();
	if (!project)
	{
		return value;
	}

	const auto& trials = project->getTrials();
	const int trialIndex = State::getInstance()->getActiveTrial();
	if (trialIndex < 0 || trialIndex >= static_cast<int>(trials.size()))
	{
		return value;
	}

	Trial* trial = trials[trialIndex];
	if (!trial)
	{
		return value;
	}

	const int frameCount = trial->getNbImages();
	if (frameCount <= 0)
	{
		return 0;
	}

	if (value >= frameCount)
	{
		value = frameCount - 1;
	}

	return std::max(0, value);
}

void WorldViewDockGLWidget::ViewModel::clearSceneData()
{
	if (!m_cameras.isEmpty())
	{
		m_cameras.clear();
		emit camerasChanged();
	}
	if (!m_markers.isEmpty())
	{
		m_markers.clear();
		emit markersChanged();
	}
	if (!m_rigidPoints.isEmpty())
	{
		m_rigidPoints.clear();
		emit rigidPointsChanged();
	}
	if (!m_rigidMeshes.isEmpty())
	{
		m_rigidMeshes.clear();
		emit rigidMeshesChanged();
	}

	if (m_sceneCenter != QVector3D() || !qFuzzyCompare(1.0 + m_sceneRadius, 1.0 + kDefaultSceneRadius))
	{
		m_sceneCenter = QVector3D();
		m_sceneRadius = kDefaultSceneRadius;
		emit sceneBoundsChanged();
	}

	if (m_hasSceneData)
	{
		m_hasSceneData = false;
		emit sceneChanged();
	}
}

void WorldViewDockGLWidget::ViewModel::updateSceneData()
{
#ifndef XMA_ENABLE_QUICK3D_WORLD_VIEW
	clearSceneData();
	return;
#else
	Project* project = Project::getInstance();
	if (!project)
	{
		clearSceneData();
		return;
	}

	const auto& trials = project->getTrials();
	const int trialIndex = State::getInstance()->getActiveTrial();
	if (trialIndex < 0 || trialIndex >= static_cast<int>(trials.size()))
	{
		clearSceneData();
		return;
	}

	Trial* trial = trials[trialIndex];
	if (!trial || trial->getNbImages() <= 0)
	{
		clearSceneData();
		return;
	}

	const int frame = clampFrameToActiveTrial(m_frame);
	if (frame != m_frame)
	{
		m_frame = frame;
		emit frameChanged(m_frame);
	}

	SceneAccumulator sceneBounds;
	SceneAccumulator contentBounds;
	SceneAccumulator cameraBounds;
	QVariantList cameras;
	QVariantList markers;
	QVariantList rigidPoints;
	QVariantList rigidMeshes;

	const auto& cameraList = project->getCameras();
	const int referenceCalibration = trial->getReferenceCalibrationImage();
	for (Camera* camera : cameraList)
	{
		QVariantMap entry = buildCameraEntry(camera, referenceCalibration, cameraBounds);
		if (!entry.isEmpty())
		{
			const QVector3D cameraPosition = entry.value(QStringLiteral("position")).value<QVector3D>();
			sceneBounds.include(cameraPosition);
			cameras.append(entry);
		}
	}

	const auto& markerList = trial->getMarkers();
	for (Marker* marker : markerList)
	{
		if (!marker)
		{
			continue;
		}
		const auto& points3D = marker->getPoints3D();
		const auto& status3D = marker->getStatus3D();
		if (frame >= static_cast<int>(points3D.size()) || frame >= static_cast<int>(status3D.size()))
		{
			continue;
		}
		const auto status = status3D[frame];
		if (status < xma::PREDICTED)
		{
			continue;
		}
		const cv::Point3d& point = points3D[frame];
		if (!isValidPoint(point))
		{
			continue;
		}
		const QVector3D position = toVector3D(point);
		sceneBounds.include(position);
		contentBounds.include(position);

		QVariantMap markerEntry;
		markerEntry.insert(QStringLiteral("name"), marker->getDescription());
		markerEntry.insert(QStringLiteral("position"), QVariant::fromValue(position));
		markerEntry.insert(QStringLiteral("color"), QVariant::fromValue(QColor(0xFF, 0x88, 0x00)));
		markerEntry.insert(QStringLiteral("size"), 0.18);
		markers.append(markerEntry);
	}

	const auto& bodies = trial->getRigidBodies();
	const bool drawMeshesSetting = Settings::getInstance()->getBoolSetting("TrialDrawRigidBodyMeshmodels");
	const bool hideAllSetting = Settings::getInstance()->getBoolSetting("TrialDrawHideAll");
	const bool filteredSetting = Settings::getInstance()->getBoolSetting("TrialDrawFiltered");
	const bool trialMeshesEnabled = trial->renderMeshes() && !trial->getIsDefault();
	const bool calibrationReady = project->getCalibration() != NO_CALIBRATION;
	const bool allowMeshes = drawMeshesSetting && !hideAllSetting && trialMeshesEnabled && calibrationReady;
	for (RigidBody* body : bodies)
	{
		if (!body || !body->getVisible())
		{
			continue;
		}
		const auto& referencePoints = body->getReferencePoints();
		if (referencePoints.empty())
		{
			continue;
		}
		const QColor bodyColor = body->getColor().isValid() ? body->getColor() : QColor(0x4C, 0xAF, 0x50);
		for (const cv::Point3d& localPoint : referencePoints)
		{
			cv::Point3d worldPoint;
			if (!body->transformPoint(localPoint, worldPoint, frame, false))
			{
				continue;
			}
			if (!isValidPoint(worldPoint))
			{
				continue;
			}
			const QVector3D position = toVector3D(worldPoint);
			sceneBounds.include(position);
			contentBounds.include(position);

			QVariantMap rigidPoint;
			rigidPoint.insert(QStringLiteral("name"), body->getDescription());
			rigidPoint.insert(QStringLiteral("position"), QVariant::fromValue(position));
			rigidPoint.insert(QStringLiteral("color"), QVariant::fromValue(bodyColor));
			rigidPoint.insert(QStringLiteral("size"), 0.22);
			rigidPoint.insert(QStringLiteral("opacity"), 0.75);
			rigidPoints.append(rigidPoint);
		}

		if (allowMeshes && body->hasMeshModel() && body->getDrawMeshModel())
		{
			const auto& poseComputed = body->getPoseComputed();
			if (frame < static_cast<int>(poseComputed.size()))
			{
				const bool hasComputedPose = poseComputed[frame];
				const auto& poseFiltered = body->getPoseFiltered();
				const bool hasFilteredPose = filteredSetting && frame < static_cast<int>(poseFiltered.size()) && poseFiltered[frame];
				if (hasComputedPose || hasFilteredPose)
				{
					const bool useFiltered = hasFilteredPose;
					const auto& rotationVectors = body->getRotationVector(useFiltered);
					const auto& translationVectors = body->getTranslationVector(useFiltered);
					if (frame < static_cast<int>(rotationVectors.size()) && frame < static_cast<int>(translationVectors.size()))
					{
						const cv::Vec3d& rotationVector = rotationVectors[frame];
						const cv::Vec3d& translationVector = translationVectors[frame];

						cv::Mat rotationMatrix;
						cv::Rodrigues(rotationVector, rotationMatrix);

						QMatrix3x3 orientationMatrix;
						for (int row = 0; row < 3; ++row)
						{
							for (int column = 0; column < 3; ++column)
							{
								orientationMatrix(row, column) = static_cast<float>(rotationMatrix.at<double>(row, column));
							}
						}
						const QQuaternion orientation = QQuaternion::fromRotationMatrix(orientationMatrix);

						const double tx = translationVector[0];
						const double ty = translationVector[1];
						const double tz = translationVector[2];
						const double px = rotationMatrix.at<double>(0, 0) * -tx + rotationMatrix.at<double>(0, 1) * -ty + rotationMatrix.at<double>(0, 2) * -tz;
						const double py = rotationMatrix.at<double>(1, 0) * -tx + rotationMatrix.at<double>(1, 1) * -ty + rotationMatrix.at<double>(1, 2) * -tz;
						const double pz = rotationMatrix.at<double>(2, 0) * -tx + rotationMatrix.at<double>(2, 1) * -ty + rotationMatrix.at<double>(2, 2) * -tz;
						const QVector3D position(static_cast<float>(px), static_cast<float>(py), static_cast<float>(pz));

						sceneBounds.include(position);
						contentBounds.include(position);

						const float meshScale = static_cast<float>(body->getMeshScale());
						if (meshScale > 0.0f)
						{
							const QVector3D extent(meshScale, meshScale, meshScale);
							sceneBounds.include(position + extent);
							sceneBounds.include(position - extent);
							contentBounds.include(position + extent);
							contentBounds.include(position - extent);
						}

						QString meshPath = body->getMeshModelname();
						if (!meshPath.isEmpty())
						{
							QFileInfo meshInfo(meshPath);
							if (!meshInfo.isAbsolute())
							{
								QFileInfo projectInfo(project->getProjectFilename());
								if (projectInfo.exists())
								{
									const QString candidate = projectInfo.absolutePath() + QDir::separator() + meshPath;
									meshInfo = QFileInfo(candidate);
								}
							}
							const QString resolvedPath = meshInfo.isAbsolute() ? meshInfo.absoluteFilePath() : meshPath;
							const QUrl meshUrl = QUrl::fromLocalFile(QDir::cleanPath(resolvedPath));

							QVariantMap meshEntry;
							meshEntry.insert(QStringLiteral("name"), body->getDescription());
							meshEntry.insert(QStringLiteral("source"), QVariant::fromValue(meshUrl));
							meshEntry.insert(QStringLiteral("position"), QVariant::fromValue(position));
							meshEntry.insert(QStringLiteral("orientation"), QVariant::fromValue(orientation));
							meshEntry.insert(QStringLiteral("scale"), meshScale > 0.0f ? meshScale : 1.0f);
							meshEntry.insert(QStringLiteral("color"), QVariant::fromValue(bodyColor));
							meshEntry.insert(QStringLiteral("opacity"), 0.85);
							meshEntry.insert(QStringLiteral("visible"), true);
							rigidMeshes.append(meshEntry);
						}
					}
				}
			}
		}
	}

	const bool hasScene = !cameras.isEmpty() || !markers.isEmpty() || !rigidPoints.isEmpty() || !rigidMeshes.isEmpty();

	if (m_cameras != cameras)
	{
		m_cameras = cameras;
		emit camerasChanged();
	}
	if (m_markers != markers)
	{
		m_markers = markers;
		emit markersChanged();
	}
	if (m_rigidPoints != rigidPoints)
	{
		m_rigidPoints = rigidPoints;
		emit rigidPointsChanged();
	}
	if (m_rigidMeshes != rigidMeshes)
	{
		m_rigidMeshes = rigidMeshes;
		emit rigidMeshesChanged();
	}

	QVector3D center = QVector3D();
	if (contentBounds.initialized)
	{
		center = contentBounds.center();
	}
	else if (sceneBounds.initialized)
	{
		center = sceneBounds.center();
	}

	double radius = kDefaultSceneRadius;
	if (sceneBounds.initialized)
	{
		radius = sceneBounds.radius();
	}
	else if (cameraBounds.initialized)
	{
		radius = cameraBounds.radius();
	}

	qInfo().nospace() << "[WorldView] frame=" << m_frame
		<< " cameras=" << cameras.size()
		<< " markers=" << markers.size()
		<< " rigidPoints=" << rigidPoints.size()
		<< " rigidMeshes=" << rigidMeshes.size()
		<< " center=" << center
		<< " radius=" << radius;
	bool boundsChanged = false;
	if (m_sceneCenter != center)
	{
		m_sceneCenter = center;
		boundsChanged = true;
	}
	if (!qFuzzyCompare(1.0 + m_sceneRadius, 1.0 + radius))
	{
		m_sceneRadius = radius;
		boundsChanged = true;
	}
	if (boundsChanged)
	{
		emit sceneBoundsChanged();
	}

	if (m_hasSceneData != hasScene)
	{
		m_hasSceneData = hasScene;
		emit sceneChanged();
	}
#endif // XMA_ENABLE_QUICK3D_WORLD_VIEW
}

WorldViewDockGLWidget::WorldViewDockGLWidget(QWidget* parent)
	: QWidget(parent), m_viewModel(new ViewModel(this)), m_quickView(new QQuickView()), m_quickContainer(nullptr)
{
	setMinimumSize(50, 50);
	setFocusPolicy(Qt::StrongFocus);

	m_quickView->setResizeMode(QQuickView::SizeRootObjectToView);
	m_quickView->setColor(QColor(20, 20, 20));
	m_quickView->rootContext()->setContextProperty(QStringLiteral("worldViewModel"), m_viewModel);

	m_quickContainer = QWidget::createWindowContainer(m_quickView, this);
	m_quickContainer->setFocusPolicy(Qt::NoFocus);
	m_quickContainer->setMinimumSize(50, 50);
	m_quickContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	initializeQuickScene();

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_quickContainer);
}

WorldViewDockGLWidget::~WorldViewDockGLWidget()
{
	delete m_quickView;
	m_quickView = nullptr;
	m_quickContainer = nullptr;
}

void WorldViewDockGLWidget::initializeQuickScene()
{
	m_quickView->setSource(QUrl(QLatin1String(kWorldViewQmlUrl)));
}

void WorldViewDockGLWidget::setUseCustomTimeline(bool value)
{
	m_viewModel->setUseCustomTimeline(value);
}

void WorldViewDockGLWidget::setFrame(int value)
{
	m_viewModel->setFrame(value);
}

void WorldViewDockGLWidget::animate()
{
	// Emit the frameChanged signal even when the frame number is unchanged so QML can refresh state.
	m_viewModel->setFrame(m_viewModel->frame());
}

void WorldViewDockGLWidget::setFocalPlaneDistance(float distance)
{
	m_viewModel->setFocalPlaneDistance(distance);
}

#include "WorldViewDockGLWidget.moc"

