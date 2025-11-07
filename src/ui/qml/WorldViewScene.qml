import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 640
    height: 480

    property var bridge: typeof worldViewModel !== "undefined" ? worldViewModel : null
    readonly property bool hasModel: bridge !== null && bridge !== undefined

    function workspaceLabel(value) {
        switch (value) {
        case 0:
            return "Calibration";
        case 1:
            return "Digitization";
        case 2:
            return "Undistort";
        default:
            return "Workspace " + value;
        }
    }

    View3D {
        id: worldView
        anchors.fill: parent
        environment: SceneEnvironment {
            clearColor: Qt.rgba(0.08, 0.08, 0.08, 1.0)
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 8, 22)
            lookAt: Qt.vector3d(0, 0, 0)
            clipFar: 1000
            clipNear: 0.1
            fieldOfView: 40
        }

        DirectionalLight {
            eulerRotation.x: -35
            eulerRotation.y: 45
            brightness: 800
        }

        AxisHelper {
            scale: Qt.vector3d(5, 5, 5)
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 12
        color: Qt.rgba(0, 0, 0, 0.5)
        radius: 4
        border.color: Qt.rgba(1, 1, 1, 0.15)
        border.width: 1
        padding: 8

        Column {
            spacing: 4
            Text {
                text: hasModel ? workspaceLabel(bridge.workspace) : "World View"
                color: "#f0f0f0"
                font.pixelSize: 16
                font.bold: true
            }
            Text {
                text: hasModel ? "Frame " + bridge.frame : "Waiting for data"
                color: "#d0d0d0"
                font.pixelSize: 14
            }
            Text {
                text: hasModel ? (bridge.useCustomTimeline ? "Custom timeline" : "Shared timeline") : ""
                color: "#a0a0a0"
                font.pixelSize: 12
                visible: hasModel
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.5
        height: 80
        radius: 6
        color: Qt.rgba(0, 0, 0, 0.35)
        visible: !hasModel

        Text {
            anchors.centerIn: parent
            width: parent.width - 32
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            color: "#ffffff"
            font.pixelSize: 16
            text: "World view is initializing. When data sources are connected, this view will display cameras, markers, and overlays using Qt Quick 3D."
        }
    }
}
