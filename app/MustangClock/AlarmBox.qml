import QtQuick 2.15
import QtQuick.Controls 2.15

GroupBox {
    title: "Alarm"

    property var config: ({
        "hh": hour.value,
        "mm": minute.value,
        "enabled": enable.checked
    })

    Column {
        spacing: 8

        Row {
            spacing: 8

            SpinBox {
                id: hour
                from: 0
                to: 23
            }

            Text { text: ":" }

            SpinBox {
                id: minute
                from: 0
                to: 59
            }
        }

        CheckBox {
            id: enable
            text: "Enable alarm"
            checked: true
        }
    }
}
