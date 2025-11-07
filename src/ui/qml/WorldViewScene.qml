import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 640
    height: 480
    Component.onCompleted: {
        if (hasBridge) {
            console.log("[WorldViewQML] completed frame=", bridge.frame,
                        "cams=", bridge.cameras.length,
                        "markers=", bridge.markers.length,
                        "rigidPoints=", bridge.rigidPoints.length,
                        "meshes=", bridge.rigidMeshes.length,
                        "hasScene=", hasScene)
        } else {
            console.log("[WorldViewQML] completed without bridge")
        }
    }

    Connections {
        target: hasBridge ? bridge : null
        function onSceneChanged() {
            console.log("[WorldViewQML] sceneChanged hasScene=", root.hasScene,
                        "cams=", bridge.cameras.length,
                        "markers=", bridge.markers.length,
                        "rigidPoints=", bridge.rigidPoints.length,
                        "meshes=", bridge.rigidMeshes.length)
            camera.refreshLookAt()
        }

        function onSceneBoundsChanged() {
            camera.refreshLookAt()
        }
    }


    property var bridge: typeof worldViewModel !== "undefined" ? worldViewModel : null
    readonly property bool hasBridge: bridge !== null && bridge !== undefined
    readonly property bool featureEnabled: hasBridge && bridge.quick3dEnabled
    readonly property bool hasScene: featureEnabled && bridge.hasSceneData

    function sceneCenter() {
        return featureEnabled ? bridge.sceneCenter : Qt.vector3d(0, 0, 0)
    }

    function sceneRadius() {
        const radius = featureEnabled ? bridge.sceneRadius : 10
        return radius > 0 ? radius : 10
    }

    View3D {
        id: worldView
        anchors.fill: parent
        visible: featureEnabled
        environment: SceneEnvironment {
            clearColor: Qt.rgba(0.08, 0.08, 0.08, 1.0)
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera {
            id: camera
            position: {
                const center = root.sceneCenter()
                const radius = root.sceneRadius()
                return Qt.vector3d(center.x + radius * 0.9,
                                   center.y + radius * 0.6,
                                   center.z + radius * 1.6)
            }
            clipNear: 0.1
            clipFar: 5000
            fieldOfView: 40

            function refreshLookAt() {
                lookAt(root.sceneCenter())
            }

            Component.onCompleted: refreshLookAt()
        }

        Node {
            id: orbitOrigin
            position: root.sceneCenter()
        }

        OrbitCameraController {
            camera: camera
            origin: orbitOrigin
            enabled: featureEnabled
        }

        DirectionalLight {
            id: keyLight
            eulerRotation: Qt.vector3d(-35, 45, 0)
            brightness: hasScene ? 300 : 150
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(45, -120, 0)
            brightness: hasScene ? 60 : 40
        }

        AxisHelper {
            scale: Qt.vector3d(2, 2, 2)
        }

        Model {
            id: debugProbe
            source: "#Sphere"
            position: Qt.vector3d(0, 0, 0)
            scale: Qt.vector3d(2, 2, 2)
            materials: PrincipledMaterial {
                baseColor: Qt.rgba(0.8, 0.2, 0.2, 1.0)
                roughness: 0.4
            }
            visible: !root.hasScene
            castsShadows: false
        }

        Model {
            id: ground
            source: "#Cube"
            visible: hasScene
            position: Qt.vector3d(root.sceneCenter().x,
                                  root.sceneCenter().y - root.sceneRadius() * 0.02,
                                  root.sceneCenter().z)
            scale: Qt.vector3d(root.sceneRadius(), 0.02, root.sceneRadius())
            materials: PrincipledMaterial {
                baseColor: Qt.rgba(0.12, 0.12, 0.12, 1.0)
                roughness: 0.9
                metalness: 0.0
            }
            castsShadows: false
        }

        Repeater3D {
            id: cameraRepeater
            model: hasScene ? bridge.cameras : []
            delegate: Node {
                visible: modelData.visible !== false
                position: modelData.position
                rotation: modelData.orientation
                Model {
                    source: "#Cube"
                    scale: Qt.vector3d(0.35, 0.25, 0.25)
                    materials: PrincipledMaterial {
                        baseColor: modelData.color
                        metalness: 0.1
                        roughness: 0.5
                    }
                    castsShadows: false
                }
                Model {
                    source: "#Cone"
                    position: Qt.vector3d(0, 0, -0.45)
                    eulerRotation: Qt.vector3d(90, 0, 0)
                    scale: Qt.vector3d(0.2, 0.2, 0.35)
                    materials: PrincipledMaterial {
                        baseColor: modelData.color
                        roughness: 0.3
                        metalness: 0.0
                    }
                    castsShadows: false
                }
            }
        }

        Repeater3D {
            id: markerRepeater
            model: hasScene ? bridge.markers : []
            delegate: Model {
                position: modelData.position
                source: "#Sphere"
                scale: {
                    const size = modelData.size !== undefined ? modelData.size : 0.2
                    return Qt.vector3d(size, size, size)
                }
                materials: PrincipledMaterial {
                    baseColor: modelData.color
                    roughness: 0.2
                    metalness: 0.0
                }
                castsShadows: false
            }
        }

        Repeater3D {
            id: rigidPointRepeater
            model: hasScene ? bridge.rigidPoints : []
            delegate: Model {
                position: modelData.position
                source: "#Sphere"
                scale: {
                    const size = modelData.size !== undefined ? modelData.size : 0.25
                    return Qt.vector3d(size, size, size)
                }
                materials: PrincipledMaterial {
                    baseColor: modelData.color
                    roughness: 0.4
                    metalness: 0.0
                    opacity: modelData.opacity !== undefined ? modelData.opacity : 0.85
                }
                castsShadows: false
            }
        }

        Repeater3D {
            id: rigidMeshRepeater
            model: hasScene ? bridge.rigidMeshes : []
            delegate: Model {
                visible: modelData.visible !== false
                source: modelData.source
                position: {
                    const p = modelData.position
                    return Qt.vector3d(p.x, p.y, p.z)
                }
                rotation: modelData.orientation !== undefined ? modelData.orientation : Qt.quaternion(1, 0, 0, 0)
                scale: {
                    let s = modelData.scale !== undefined ? modelData.scale : 1.0
                    if (typeof s === "number") {
                        return Qt.vector3d(s, s, s)
                    }
                    return s
                }
                materials: PrincipledMaterial {
                    id: meshMaterial
                    baseColor: modelData.color !== undefined ? modelData.color : Qt.rgba(0.6, 0.6, 0.6, 1.0)
                    opacity: modelData.opacity !== undefined ? modelData.opacity : 1.0
                    roughness: 0.4
                    metalness: 0.0
                }
                castsShadows: true
                receivesShadows: true
            }
        }
    }

    Rectangle {
        id: infoPanel
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 12
        visible: featureEnabled
        color: Qt.rgba(0, 0, 0, 0.55)
        radius: 4
        border.color: Qt.rgba(1, 1, 1, 0.15)
        border.width: 1

        Column {
            spacing: 4
            anchors.fill: parent
            anchors.margins: 8
            Text {
                text: featureEnabled ? ("Frame " + bridge.frame) : "World View"
                color: "#f0f0f0"
                font.pixelSize: 16
                font.bold: true
            }
            Text {
                text: hasScene
                             ? (bridge.cameras.length + " cameras · "
                                 + bridge.markers.length + " markers · "
                                 + bridge.rigidPoints.length + " rigid points · "
                                 + bridge.rigidMeshes.length + " meshes")
                      : "No scene data"
                color: "#d0d0d0"
                font.pixelSize: 13
            }
            Text {
                text: featureEnabled
                      ? (bridge.useCustomTimeline ? "Custom timeline" : "Shared timeline")
                      : ""
                visible: featureEnabled
                color: "#a0a0a0"
                font.pixelSize: 12
            }
        }
    }

    Rectangle {
        id: messagePanel
        anchors.centerIn: parent
        width: parent.width * 0.55
        radius: 6
        color: Qt.rgba(0, 0, 0, 0.35)
        visible: !featureEnabled || !hasScene
        implicitHeight: messageText.paintedHeight + 24

        Text {
            id: messageText
            anchors.centerIn: parent
            width: parent.width - 32
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: "#ffffff"
            font.pixelSize: 16
            text: !featureEnabled
                  ? "Qt Quick 3D world view is disabled.\nEnable XMA_ENABLE_QUICK3D_WORLD_VIEW during the build to preview the experimental renderer."
                  : "Scene data is not available for the current frame."
        }
    }
}
