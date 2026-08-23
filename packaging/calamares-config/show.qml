pragma ComponentBehavior: Bound

import QtQuick 2.12

Rectangle {
    id: root
    property bool activatedInCalamares: false
    property int slideIndex: 0
    color: "#062b22"
    clip: true

    function onActivate() {
        slideIndex = 0
        slideTimer.restart()
    }

    function onLeave() {
        slideTimer.stop()
    }

    Image {
        anchors.fill: parent
        source: "influent-stream-wallpaper.png"
        fillMode: Image.PreserveAspectCrop
        opacity: 0.22
    }

    Rectangle {
        anchors.fill: parent
        color: "#062b22"
        opacity: 0.72
    }

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 52
        spacing: 18

        Text {
            text: "INFLUENT DANENONE"
            color: "#7ee7be"
            font.pixelSize: 14
            font.bold: true
            font.letterSpacing: 2
        }

        Text {
            text: [
                "Una instalación clara y controlada.",
                "Plasma como escritorio principal.",
                "Hyprland disponible como sesión avanzada."
            ][root.slideIndex]
            color: "#ffffff"
            font.pixelSize: 34
            font.weight: Font.DemiBold
            wrapMode: Text.WordWrap
            width: parent.width
        }

        Text {
            text: [
                "Revisa cada decisión antes de escribir cambios en el disco.",
                "Una sesión moderna, legible y preparada para el uso diario.",
                "Elige la sesión que mejor se ajuste a tu flujo de trabajo."
            ][root.slideIndex]
            color: "#d7f8ea"
            font.pixelSize: 17
            wrapMode: Text.WordWrap
            width: parent.width * 0.82
        }

        Row {
            spacing: 8
            Repeater {
                model: 3
                Rectangle {
                    required property int index
                    width: index === root.slideIndex ? 34 : 10
                    height: 4
                    radius: 2
                    color: index === root.slideIndex ? "#7ee7be" : "#79ae9b"
                    Behavior on width { NumberAnimation { duration: 160 } }
                }
            }
        }
    }

    Timer {
        id: slideTimer
        interval: 5000
        repeat: true
        running: root.activatedInCalamares
        onTriggered: root.slideIndex = (root.slideIndex + 1) % 3
    }
}
