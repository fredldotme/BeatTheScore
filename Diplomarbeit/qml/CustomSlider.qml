import QtQuick 2.0

HighlightableItem {
    id: itemRoot

    property alias fontColor: labelText.color
    property color fontBorderColor: "black"
    property double value: 50
    property double min: 0
    property double max: 100
    property double steps: 1
    property double defaultValue: (min + max) / 2
    property int shift: (max - min) / 10
    property bool useShifts: true
    property string label
    property int labelFontSize : standardHeaderPixelSize
    property string unit
    readonly property bool isOnDisk : true
    backgroundSource: ""
    highlightOn: focus

    signal changed

    function checkStepping(pressed) {
        if(!pressed && steps > 1) {
            if(value % steps != 0) {
                if(value == value < min) {
                    value = min;
                } else {
                    var offset = 0
                    if(value % steps < steps / 2) {
                        offset = 0 - (value % steps);
                    } else {
                        offset = steps - (value % steps);
                    }
                    value += offset;
                }
            }
        }
        changed()
    }

    Row {
        anchors.fill: parent

        Rectangle {

            height: parent.height
            color: "transparent"
            visible: true
            width: Math.min(height * 4, parent.width - labelText.width)

            Image {
                id: sliderBase
                source: itemRoot.isOnDisk ? "../graphics/slider_base.png" : "assets:/graphics/slider_base.png"
                anchors.fill: parent
            }

            Image {
                id: sliderHandle
                source: itemRoot.isOnDisk ? "../graphics/slider_handle.png" : "assets:/graphics/slider_handle.png"

                height: parent.height
                width: height * .5

                x: (value - min) * (parent.width - width) / (max - min)
            }

            MouseArea {
                anchors.fill: parent

                function updateValue() {
                    value = Math.min(
                                Math.max(
                                    (mouseX - sliderHandle.width / 2) *
                                    (max - min) / (parent.width - sliderHandle.width) + min,
                                    min),
                                max)

                    //changed()
                }

                onPressed: {
                    updateValue()
                }

                onDoubleClicked: {
                    value = defaultValue
                }

                onMouseXChanged: {
                    if (pressed) {
                        updateValue()
                    }
                }

                onPressedChanged: {
                    checkStepping(pressed)
                }
            }
        }

        Text {
            id: labelText
            text: label + " " + Math.round(value) + unit
            font.pixelSize: labelFontSize
            anchors.verticalCenter: parent.verticalCenter
            style: Text.Raised
            styleColor: fontBorderColor
            color: standardFontColor
        }
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Left) {
            event.accepted = true

            value = useShifts ? Math.max(min, value - shift) : Math.max(min, value - steps)
            checkStepping(false);
        }
        else if (event.key === Qt.Key_Right) {
            event.accepted = true

            value = useShifts ? Math.min(max, value + shift) : value = Math.min(max, value + steps)
            checkStepping(false);
        }
    }
}
