#pragma once

#include <QObject>
#include <QVariantList>
#include <QVector3D>

namespace xma
{
class WorldViewSceneBridge : public QObject
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
    explicit WorldViewSceneBridge(QObject* parent = nullptr);

    int frame() const { return m_frame; }
    bool useCustomTimeline() const { return m_useCustomTimeline; }
    double focalPlaneDistance() const { return m_focalPlaneDistance; }
    int workspace() const;

    bool quick3dEnabled() const;

    const QVariantList& cameras() const { return m_cameras; }
    const QVariantList& markers() const { return m_markers; }
    const QVariantList& rigidPoints() const { return m_rigidPoints; }
    const QVariantList& rigidMeshes() const { return m_rigidMeshes; }
    QVector3D sceneCenter() const { return m_sceneCenter; }
    double sceneRadius() const { return m_sceneRadius; }
    bool hasSceneData() const { return m_hasSceneData; }

public slots:
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
