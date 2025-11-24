import QtQuick
import QtQuick3D
import QtQuick3D.AssetUtils
import QtQuick3D.Helpers

Item {
    id: root
    width: 640
    height: 480

    property var bridge: typeof worldViewModel !== "undefined" ? worldViewModel : null
    readonly property bool hasBridge: bridge !== null && bridge !== undefined
    readonly property bool featureEnabled: hasBridge && bridge.quick3dEnabled
    readonly property bool hasScene: featureEnabled && bridge.hasSceneData
    readonly property real focalPlaneDistance: featureEnabled ? bridge.focalPlaneDistance : 0.0

    readonly property quaternion zUpRotation: Qt.quaternionFromAxisAndAngle(Qt.vector3d(1, 0, 0), -90)
    readonly property quaternion zUpInverse: quaternionConjugate(zUpRotation)

    function sceneCenter() {
        return featureEnabled ? bridge.sceneCenter : Qt.vector3d(0, 0, 0)
    }

    function sceneRadius() {
        const radius = featureEnabled ? bridge.sceneRadius : 10
        return radius > 0 ? radius : 10
    }

    function quaternionConjugate(q) {
        return Qt.quaternion(q.scalar, -q.x, -q.y, -q.z)
    }

    function convertToZUp(vec) {
        if (!vec)
            return Qt.vector3d(0, 0, 0)
        return Qt.vector3d(vec.x, vec.z, -vec.y)
    }

    function applyZUpOrientation(q) {
        if (!q)
            return Qt.quaternion(1, 0, 0, 0)
        return Qt.quaternionMultiply(zUpInverse, q)
    }

    function cameraForwardVector() {
        const target = orbitController.target
        const pos = camera.position
        const dx = target.x - pos.x
        const dy = target.y - pos.y
        const dz = target.z - pos.z
        const length = Math.sqrt(dx * dx + dy * dy + dz * dz)
        if (length <= 0.000001)
            return Qt.vector3d(0, 0, -1)
        return Qt.vector3d(dx / length, dy / length, dz / length)
    }

    function refreshCameraIfNeeded() {
        orbitController.syncCamera()
    }

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
        orbitController.resetCamera()
    }

    Connections {
        target: hasBridge ? bridge : null
        function onSceneChanged() {
            orbitController.onSceneChanged()
            focalPlane.updatePlacement()
        }

        function onSceneBoundsChanged() {
            orbitController.onSceneBoundsChanged()
            focalPlane.updatePlacement()
        }

        function onFocalPlaneDistanceChanged() {
            focalPlane.updatePlacement()
        }

        function onFrameChanged() {
            focalPlane.updatePlacement()
        }
    }

    QtObject {
        id: orbitController
        property bool initialized: false
        property real yaw: 225
        property real pitch: 28
        property real distance: 10
        property real minDistance: 0.05
        property real maxDistance: 10000
        property var target: Qt.vector3d(0, 0, 0)

        function degToRad(value) {
            return value * Math.PI / 180.0
        }

        function clampPitch(value) {
            return Math.max(-85, Math.min(85, value))
        }

        function clampDistance(value) {
            return Math.max(minDistance, Math.min(maxDistance, value))
        }

        function updateTarget() {
            target = root.convertToZUp(root.sceneCenter())
        }

        function updateBounds() {
            const radius = root.sceneRadius()
            minDistance = Math.max(radius * 0.05, 0.05)
            maxDistance = Math.max(radius * 20.0, minDistance + 5.0)
            if (!initialized) {
                distance = Math.max(radius * 2.5, minDistance * 2.0)
            } else {
                distance = clampDistance(distance)
            }
        }

        function forwardVector() {
            const yawRad = degToRad(yaw)
            const pitchRad = degToRad(pitch)
            const cosPitch = Math.cos(pitchRad)
            return Qt.vector3d(
                        Math.sin(yawRad) * cosPitch,
                        Math.sin(pitchRad),
                        Math.cos(yawRad) * cosPitch)
        }

        function syncCamera() {
            if (!root.featureEnabled)
                return
            const forward = forwardVector()
            const pos = Qt.vector3d(
                        target.x - forward.x * distance,
                        target.y - forward.y * distance,
                        target.z - forward.z * distance)
            camera.position = pos
            camera.lookAt(target, Qt.vector3d(0, 1, 0))
            focalPlane.updatePlacement()
        }

        function resetAngles() {
            yaw = 225
            pitch = 28
        }

        function resetCamera() {
            updateTarget()
            updateBounds()
            resetAngles()
            initialized = true
            syncCamera()
        }

        function onSceneChanged() {
            initialized = initialized && root.hasScene
            updateTarget()
            updateBounds()
            if (!initialized) {
                resetAngles()
                initialized = true
            }
            syncCamera()
        }

        function onSceneBoundsChanged() {
            updateTarget()
            updateBounds()
            syncCamera()
        }

        function applyDrag(deltaX, deltaY) {
            if (!root.featureEnabled)
                return
            yaw = (yaw - deltaX * 0.35) % 360
            if (yaw < 0)
                yaw += 360
            pitch = clampPitch(pitch - deltaY * 0.25)
            syncCamera()
        }

        function applyZoom(wheelDelta) {
            if (!root.featureEnabled)
                return
            const factor = Math.exp(-wheelDelta * 0.0015)
            distance = clampDistance(distance * factor)
            syncCamera()
        }
    }

    Component {
        id: runtimeMeshTintMaterial
        PrincipledMaterial {
            metalness: 0.0
            roughness: 0.4
            opacity: 1.0
        }
    }

    function applyRuntimeMeshMaterial(node, color) {
        if (!node || !runtimeMeshTintMaterial)
            return

        const stack = [node]
        while (stack.length > 0) {
            const current = stack.pop()
            if (!current)
                continue

            if (current.materials !== undefined) {
                const existing = current.materials
                if (existing && existing.length) {
                    for (let i = 0; i < existing.length; ++i) {
                        const owned = existing[i]
                        if (owned && owned.parent === current) {
                            owned.destroy()
                        }
                    }
                }
                const material = runtimeMeshTintMaterial.createObject(current, {
                    baseColor: color
                })
                if (material) {
                    current.materials = [material]
                }
            }

            const children = current.children
            if (children && children.length) {
                for (let i = 0; i < children.length; ++i) {
                    stack.push(children[i])
                }
            }
        }
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
            clipNear: 0.1
            clipFar: 6000
            fieldOfView: 40
        }

        Node {
            id: sceneRoot
            rotation: root.zUpRotation

            DirectionalLight {
                id: keyLight
                eulerRotation: Qt.vector3d(-55, 45, 0)
                brightness: root.hasScene ? 350 : 180
            }

            DirectionalLight {
                eulerRotation: Qt.vector3d(35, -110, 0)
                brightness: root.hasScene ? 90 : 60
            }

            AxisHelper {
                scale: Qt.vector3d(0.015, 0.015, 0.015)
            }

            Model {
                id: debugProbe
                source: "#Sphere"
                position: Qt.vector3d(0, 0, 0)
                scale: Qt.vector3d(0.01, 0.01, 0.01)
                materials: PrincipledMaterial {
                    baseColor: Qt.rgba(0.8, 0.2, 0.2, 1.0)
                    roughness: 0.4
                }
                visible: !root.hasScene
                castsShadows: false
            }

            Repeater3D {
                id: cameraRepeater
                model: root.hasScene ? bridge.cameras : []
                delegate: Node {
                    visible: modelData.visible !== false
                    position: modelData.position
                    rotation: root.applyZUpOrientation(modelData.orientation)
                    Model {
                        source: "#Cube"
                        scale: Qt.vector3d(0.15, 0.1, 0.1)
                        materials: PrincipledMaterial {
                            baseColor: modelData.color
                            metalness: 0.1
                            roughness: 0.5
                        }
                        castsShadows: false
                    }
                    Model {
                        source: "#Cone"
                        position: Qt.vector3d(0, 0, -0.2)
                        eulerRotation: Qt.vector3d(90, 0, 0)
                        scale: Qt.vector3d(0.08, 0.08, 0.18)
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
                model: root.hasScene ? bridge.markers : []
                delegate: Model {
                    position: modelData.position
                    source: "#Sphere"
                    scale: {
                        const size = modelData.size !== undefined ? modelData.size : 0.18
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
                model: root.hasScene ? bridge.rigidPoints : []
                delegate: Model {
                    position: modelData.position
                    source: "#Sphere"
                    scale: {
                        const size = modelData.size !== undefined ? modelData.size : 0.22
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
                model: root.hasScene ? bridge.rigidMeshes : []
                delegate: Node {
                    id: meshWrapper
                    visible: modelData.visible !== false
                    opacity: 1.0
                    property color meshColor: modelData.color !== undefined ? modelData.color : Qt.rgba(0.6, 0.6, 0.6, 1.0)
                    position: modelData.position
                    rotation: root.applyZUpOrientation(modelData.orientation !== undefined ? modelData.orientation : Qt.quaternion(1, 0, 0, 0))
                    scale: {
                        let s = modelData.scale !== undefined ? modelData.scale : 1.0
                        if (typeof s === "number") {
                            return Qt.vector3d(s, s, s)
                        }
                        return s
                    }

                    RuntimeLoader {
                        id: runtimeMesh
                        source: modelData.source
                        onStatusChanged: {
                            if (status === RuntimeLoader.Error) {
                                console.warn("[WorldViewQML] mesh load failed", source, errorString)
                            } else if (status === RuntimeLoader.Ready) {
                                root.applyRuntimeMeshMaterial(runtimeMesh.item, meshWrapper.meshColor)
                            }
                        }
                    }
                }
            }
        }

        Node {
            id: focalPlane
            visible: root.featureEnabled && root.hasScene && root.focalPlaneDistance > 0.0
            property real planeExtent: Math.max(root.sceneRadius() * 1.6, 0.05)

            Model {
                id: focalPlaneModel
                source: "#Plane"
                scale: Qt.vector3d(focalPlane.planeExtent, focalPlane.planeExtent, 1.0)
                materials: PrincipledMaterial {
                    baseColor: Qt.rgba(0.25, 0.55, 0.95, 0.32)
                    roughness: 0.85
                    metalness: 0.0
                    alphaMode: PrincipledMaterial.Blend
                }
                castsShadows: false
                receivesShadows: false
            }

            function updatePlacement() {
                planeExtent = Math.max(root.sceneRadius() * 1.6, 0.05)
                if (!visible) {
                    return
                }
                const forward = root.cameraForwardVector()
                const distance = Math.max(0.0, root.focalPlaneDistance)
                const camPos = camera.position
                position = Qt.vector3d(camPos.x + forward.x * distance,
                                       camPos.y + forward.y * distance,
                                       camPos.z + forward.z * distance)
                rotation = camera.rotation
            }

            onVisibleChanged: {
                if (visible) {
                    updatePlacement()
                }
            }
        }

        MouseArea {
            id: orbitMouseArea
            anchors.fill: parent
            enabled: root.featureEnabled
            acceptedButtons: Qt.LeftButton
            hoverEnabled: true
            preventStealing: true
            cursorShape: Qt.DragMoveCursor
            property point lastPos: Qt.point(0, 0)

            onPressed: {
                lastPos = Qt.point(mouse.x, mouse.y)
            }

            onPositionChanged: {
                if (!root.featureEnabled)
                    return
                if (mouse.buttons & Qt.LeftButton) {
                    orbitController.applyDrag(mouse.x - lastPos.x, mouse.y - lastPos.y)
                    lastPos = Qt.point(mouse.x, mouse.y)
                }
            }

            onWheel: {
                if (!root.featureEnabled)
                    return
                const delta = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.pixelDelta.y * 8
                orbitController.applyZoom(delta)
                wheel.accepted = true
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
                  ? "Qt Quick 3D world view is disabled. Rebuild with XMA_ENABLE_QUICK3D_WORLD_VIEW to enable the renderer."
                  : "Scene data is not available for the current frame."
        }
    }
}
