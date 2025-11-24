#include "ui/quick3d/CameraOverlaySceneBridge.h"

#include <QColor>
#include <QMatrix3x3>
#include <QQuaternion>
#include <QVariant>
#include <QVector3D>

#include <cmath>

#include <opencv2/opencv.hpp>

#include "core/Project.h"
#include "core/RigidBody.h"
#include "core/Settings.h"
#include "core/Trial.h"
#include "ui/State.h"

namespace xma
{
namespace
{
static inline bool isValidPoint(const cv::Point3d& point)
{
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

static inline QVector3D toVector3D(const cv::Point3d& point)
{
    return QVector3D(static_cast<float>(point.x), static_cast<float>(point.y), static_cast<float>(point.z));
}
}

CameraOverlaySceneBridge::CameraOverlaySceneBridge(QObject* parent)
    : QObject(parent), m_cameraId(-1), m_hasSceneData(false)
{
    auto* state = State::getInstance();
    connect(state, &State::activeTrialChanged, this, [this](int) { rebuildCache(); });
    connect(state, &State::activeFrameTrialChanged, this, [this](int) { rebuildCache(); });
}

bool CameraOverlaySceneBridge::quick3dEnabled() const
{
#ifdef XMA_ENABLE_QRHI_RENDERING
    return true;
#else
    return false;
#endif
}

void CameraOverlaySceneBridge::setCameraId(int cameraId)
{
    if (m_cameraId == cameraId)
    {
        return;
    }
    m_cameraId = cameraId;
    emit cameraChanged(m_cameraId);
    rebuildCache();
}

void CameraOverlaySceneBridge::refresh()
{
    rebuildCache();
}

void CameraOverlaySceneBridge::rebuildCache()
{
#ifndef XMA_ENABLE_QRHI_RENDERING
    if (!m_rigidMeshes.isEmpty() || !m_rigidPoints.isEmpty())
    {
        m_rigidMeshes.clear();
        m_rigidPoints.clear();
        m_hasSceneData = false;
        emit dataChanged();
    }
    return;
#else
    Project* project = Project::getInstance();
    if (!project)
    {
        return;
    }
    const auto& trials = project->getTrials();
    const int trialIndex = State::getInstance()->getActiveTrial();
    if (trialIndex < 0 || trialIndex >= static_cast<int>(trials.size()))
    {
        return;
    }
    Trial* trial = trials[trialIndex];
    if (!trial)
    {
        return;
    }

    const int frame = State::getInstance()->getActiveFrameTrial();

    QVariantList rigidPoints;
    QVariantList rigidMeshes;

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
            QVariantMap rigidPoint;
            rigidPoint.insert(QStringLiteral("name"), body->getDescription());
            rigidPoint.insert(QStringLiteral("position"), QVariant::fromValue(toVector3D(worldPoint)));
            rigidPoint.insert(QStringLiteral("color"), QVariant::fromValue(bodyColor));
            rigidPoint.insert(QStringLiteral("size"), 0.18);
            rigidPoint.insert(QStringLiteral("opacity"), 0.65);
            rigidPoints.append(rigidPoint);
        }

        if (allowMeshes && body->hasMeshModel() && body->getDrawMeshModel())
        {
            const auto& poseComputed = body->getPoseComputed();
            if (frame >= static_cast<int>(poseComputed.size()))
            {
                continue;
            }
            const bool hasComputedPose = poseComputed[frame];
            const auto& poseFiltered = body->getPoseFiltered();
            const bool hasFilteredPose = filteredSetting && frame < static_cast<int>(poseFiltered.size()) && poseFiltered[frame];
            if (!hasComputedPose && !hasFilteredPose)
            {
                continue;
            }
            const bool useFiltered = hasFilteredPose;
            const auto& rotationVectors = body->getRotationVector(useFiltered);
            const auto& translationVectors = body->getTranslationVector(useFiltered);
            if (frame >= static_cast<int>(rotationVectors.size()) || frame >= static_cast<int>(translationVectors.size()))
            {
                continue;
            }

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

            QVariantMap meshEntry;
            meshEntry.insert(QStringLiteral("name"), body->getDescription());
            meshEntry.insert(QStringLiteral("position"), QVariant::fromValue(position));
            meshEntry.insert(QStringLiteral("orientation"), QVariant::fromValue(orientation));
            meshEntry.insert(QStringLiteral("scale"), static_cast<float>(body->getMeshScale()));
            meshEntry.insert(QStringLiteral("color"), QVariant::fromValue(bodyColor));
            meshEntry.insert(QStringLiteral("opacity"), 0.65);
            meshEntry.insert(QStringLiteral("visible"), true);
            rigidMeshes.append(meshEntry);
        }
    }

    const bool changed = (rigidMeshes != m_rigidMeshes) || (rigidPoints != m_rigidPoints);
    if (changed)
    {
        m_rigidMeshes = rigidMeshes;
        m_rigidPoints = rigidPoints;
        m_hasSceneData = !m_rigidMeshes.isEmpty() || !m_rigidPoints.isEmpty();
        emit dataChanged();
    }
#endif // XMA_ENABLE_QRHI_RENDERING
}
} // namespace xma
