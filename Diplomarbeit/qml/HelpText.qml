import QtQuick 2.0

Rectangle {
    anchors.fill: parent
    color: "transparent"
    property string text : clickVerb + " to start!";

    Text {
        text: parent.text;
        anchors.centerIn: parent
        font.pixelSize: window.height / 10
    }

    Image {
        source: "../graphics/help_overlay.png"
        anchors.fill: parent
    }
}
