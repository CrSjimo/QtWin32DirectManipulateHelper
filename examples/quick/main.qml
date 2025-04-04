import QtQml
import QtQuick

Window {
    width: 800
    height: 600
    visible: true

    Rectangle {
        id: rectangle
        color: "blue"
        width: 200
        height: 150
        scale: 1
        anchors.centerIn: parent
    }

    PinchArea {
        anchors.fill: parent
        onPinchStarted: (pinch) => {
            console.log("Pinch center:", pinch.center)
        }
        onPinchUpdated: (pinch) => {
            rectangle.scale *= pinch.scale / pinch.previousScale
        }
    }
}