#include "ui/quick3d/WorldViewSceneBridge.h"

#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QMatrix3x3>
#include <QQuaternion>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QVector3D>
#include <QtGlobal>
#include <QtMath>

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

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
constexpr float kDefaultSceneRadius = 10.0f;
constexpr float kMinSceneRadius = 1.0f;

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

static QColor colorForMarkerStatus(xma::markerStatus status)
{
    xma::Settings* settings = xma::Settings::getInstance();
    if (!settings)
    {
        return QColor(0xFF, 0x88, 0x00);
    }

    auto fetch = [&](const char* key) -> QColor {
        return QColor(settings->getQStringSetting(QString::fromLatin1(key)));
    };

    switch (status)
    {
        case xma::UNTRACKABLE:
            return fetch("ColorUntrackable");
        case xma::DELETED:
        case xma::LOST:
        case xma::UNDEFINED:
            return fetch("ColorUndefined");
        case xma::PREDICTED:
            return fetch("ColorUndefined");
        case xma::INTERPOLATED:
            return fetch("ColorInterpolated");
        case xma::TRACKED:
            return fetch("ColorTracked");
        case xma::TRACKED_AND_OPTIMIZED:
            return fetch("ColorTrackedAndOpt");
        case xma::SET:
            return fetch("ColorSet");
        case xma::SET_AND_OPTIMIZED:
            return fetch("ColorSetAndOpt");
        case xma::MANUAL:
            return fetch("ColorManual");
        case xma::MANUAL_AND_OPTIMIZED:
            return fetch("ColorManualAndOpt");
    }

    return fetch("ColorUndefined");
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
} // namespace

using namespace xma;

WorldViewSceneBridge::WorldViewSceneBridge(QObject* parent)
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

int WorldViewSceneBridge::workspace() const
{
    return static_cast<int>(State::getInstance()->getWorkspace());
}

bool WorldViewSceneBridge::quick3dEnabled() const
{
#ifdef XMA_ENABLE_QUICK3D_WORLD_VIEW
    return true;
#else
    return false;
#endif
}

void WorldViewSceneBridge::setFrame(int value)
{
    const int clamped = clampFrameToActiveTrial(value);
    m_frame = clamped;
    emit frameChanged(m_frame);
    updateSceneData();
}

void WorldViewSceneBridge::setUseCustomTimeline(bool value)
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

void WorldViewSceneBridge::setFocalPlaneDistance(double value)
{
    if (qFuzzyCompare(1.0 + m_focalPlaneDistance, 1.0 + value))
    {
        return;
    }
    m_focalPlaneDistance = value;
    emit focalPlaneDistanceChanged(m_focalPlaneDistance);
}

int WorldViewSceneBridge::clampFrameToActiveTrial(int value) const
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

void WorldViewSceneBridge::clearSceneData()
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

void WorldViewSceneBridge::updateSceneData()
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
        markerEntry.insert(QStringLiteral("color"), QVariant::fromValue(colorForMarkerStatus(status)));
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
