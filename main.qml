// main.qml
import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Window {
    width: 1280
    height: 720
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint

    GridLayout {
        anchors.fill: parent
        columns: 2
        rows: 2

        // 四路摄像头视图
        Repeater {
            model: 4

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "black"

                Image {
                    id: cameraImage
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    source: "image://multicamera/" + index
                    cache:false

                    // 当图像更新时重新加载
                    Connections {
                        target: cameraHandler
                        function onCameraImageUpdated(camIndex) {
                            if (camIndex === index) {
                                cameraImage.source = ""
                                cameraImage.source = "image://multicamera/" + index
//                                console.log("get camera data : "+ camIndex + ", " + index)
                            }
                        }
                    }
                }

                Label {
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Camera " + (index + 1)
                    color: "white"
                    font.pixelSize: 16
                }
            }
        }
    }

    Button {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Toggle Cameras"
        onClicked: {
            if (cameraHandler.running) {
                cameraHandler.stopAllCameras()
            } else {
                cameraHandler.startAllCameras()
            }
        }
    }
}
