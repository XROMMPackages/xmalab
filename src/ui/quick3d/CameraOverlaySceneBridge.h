#pragma once

#include <QObject>
#include <QVariantList>

namespace xma
{
class Camera;
class Trial;

class CameraOverlaySceneBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int cameraId READ cameraId WRITE setCameraId NOTIFY cameraChanged)
    Q_PROPERTY(bool quick3dEnabled READ quick3dEnabled CONSTANT)
    Q_PROPERTY(bool hasSceneData READ hasSceneData NOTIFY dataChanged)
    Q_PROPERTY(QVariantList rigidMeshes READ rigidMeshes NOTIFY dataChanged)
    Q_PROPERTY(QVariantList rigidPoints READ rigidPoints NOTIFY dataChanged)

public:
    explicit CameraOverlaySceneBridge(QObject* parent = nullptr);

    int cameraId() const { return m_cameraId; }
    bool quick3dEnabled() const;
    bool hasSceneData() const { return m_hasSceneData; }
    const QVariantList& rigidMeshes() const { return m_rigidMeshes; }
    const QVariantList& rigidPoints() const { return m_rigidPoints; }

public slots:
    void setCameraId(int cameraId);
    void refresh();

signals:
    void cameraChanged(int cameraId);
    void dataChanged();

private:
    void rebuildCache();

    int m_cameraId;
    bool m_hasSceneData;
    QVariantList m_rigidMeshes;
    QVariantList m_rigidPoints;
};
} // namespace xma
