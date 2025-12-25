import QtQuick 2.15
import QtQuick.Controls 2.15

GroupBox {
    title: "WiFi"

    property var config: ({
        "ssid": ssidField.text,
        "psk": pskField.text
    })

    Column {
        spacing: 8

        TextField {
            id: ssidField
            placeholderText: "SSID"
        }

        TextField {
            id: pskField
            placeholderText: "Password"
            echoMode: TextInput.Password
        }
    }
}
