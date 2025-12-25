import QtQuick 2.15
import QtQuick.Controls 2.15

GroupBox {
    title: "Current Time"

    property var config: ({
        "hh": hour.value,
        "mm": minute.value
    })

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
}
