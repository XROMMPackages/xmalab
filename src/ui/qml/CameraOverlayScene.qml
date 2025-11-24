import QtQuick
import QtQuick3D
import QtQuick3D.AssetUtils

Item {
    id: root
    width: 320
    height: 240

    property var bridge: typeof cameraOverlayModel !== "undefined" ? cameraOverlayModel : null
    readonly property bool featureEnabled: bridge !== null && bridge.quick3dEnabled
    readonly property bool hasScene: featureEnabled && bridge.hasSceneData

    View3D {
        id: overlayView
        anchors.fill: parent
        visible: hasScene
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Transparent
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera {
            id: overlayCamera
            position: Qt.vector3d(0, 0, 10)
            clipNear: 0.01
            clipFar: 2000
            fieldOfView: 35

            function aim() {
                lookAt(Qt.vector3d(0, 0, 0))
            }

            Component.onCompleted: aim()
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(-30, 45, 0)
            brightness: hasScene ? 200 : 120
        }

        Repeater3D {
            model: hasScene ? bridge.rigidPoints : []
            delegate: Model {
                position: modelData.position
                source: "#Sphere"
                scale: Qt.vector3d(0.15, 0.15, 0.15)
                materials: PrincipledMaterial {
                    baseColor: modelData.color
                    opacity: modelData.opacity !== undefined ? modelData.opacity : 0.65
                    roughness: 0.25
                }
                castsShadows: false
            }
        }

        Repeater3D {
            model: hasScene ? bridge.rigidMeshes : []
            delegate: Node {
                property var meshSource: modelData.source !== undefined ? modelData.source : ""
                visible: modelData.visible !== false && meshSource !== ""
                opacity: modelData.opacity !== undefined ? modelData.opacity : 0.65
                position: modelData.position
                rotation: modelData.orientation !== undefined ? modelData.orientation : Qt.quaternion(1, 0, 0, 0)
                scale: {
                    const s = modelData.scale !== undefined ? modelData.scale : 1.0
                    return typeof s === "number" ? Qt.vector3d(s, s, s) : s
                }

                RuntimeLoader {
                    id: overlayRuntimeMesh
                    source: meshSource
                    onStatusChanged: {
                        if (status === RuntimeLoader.Error && meshSource !== "") {
                            console.warn("[CameraOverlayQML] mesh load failed", source, errorString)
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.6
        visible: !hasScene
        color: Qt.rgba(0.05, 0.05, 0.05, 0.65)
        radius: 4
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.15)
        Text {
            anchors.centerIn: parent
            text: featureEnabled ? "No overlay data" : "Quick3D overlay disabled"
            color: "#ffffff"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
        }
    }
}
