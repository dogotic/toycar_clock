import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: window
    width: 360
    height: 480
    visible: true
    title: "Clock Config"

    Column {
        anchors.centerIn: parent
        spacing: 16

        WifiBox { id: wifiBox }
        TimeBox { id: timeBox }
        AlarmBox { id: alarmBox }

        // Status labels
        Label {
            id: connectionStatusLabel
            text: "Not connected"
            color: "blue"
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            width: parent.width
        }

        Label {
            id: sendStatusLabel
            text: ""
            color: "green"
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            width: parent.width
        }

        GridLayout {
            columns: 2
            rowSpacing: 12
            columnSpacing: 12
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width * 0.9

            Button {
                text: "Connect"
                onClicked: {
                    if (!bleManager.deviceFoundYet) {
                        bleManager.startScan()
                    } else {
                        bleManager.connectToDevice()
                    }
                }
            }

            Button {
                text: "Send WiFi"
                Layout.fillWidth: true
                onClicked: {
                    bleManager.sendConfig({ "wifi": wifiBox.config })
                    sendStatusLabel.text = "WiFi config sent!"
                }
            }

            Button {
                text: "Send Time"
                Layout.fillWidth: true
                onClicked: {
                    bleManager.sendConfig({ "time": timeBox.config })
                    sendStatusLabel.text = "Time config sent!"
                }
            }

            Button {
                text: "Send Alarm"
                Layout.fillWidth: true
                onClicked: {
                    bleManager.sendConfig({ "alarm": alarmBox.config })
                    sendStatusLabel.text = "Alarm config sent!"
                }
            }
        }
    }

    // Update the connection status from BleManager signals
    Connections {
        target: bleManager
        function onConnected() {
            connectionStatusLabel.text = "Connected to device"
        }
        function onDisconnected() {
            connectionStatusLabel.text = "Disconnected"
        }
        function onDataSent(type) {
            sendStatusLabel.text = type + " config sent!"
        }
    }
}
