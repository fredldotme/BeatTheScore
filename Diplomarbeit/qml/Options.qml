import QtQuick 2.0
import QtQuick.LocalStorage 2.0

NavigatablePanel {

    property real lineHeight: sectionHeight / 2

    function addSectionsToFocusChain() {
        focusComponents.push(keepPreferencesCheckBox)
        focusComponents.push(showAudioInputsCheckBox)
        focusComponents.push(samplingSlider)
        focusComponents.push(backButton)
        //focusComponents.push(saveButton)
        focusComponents.push(saveAndBackButton)
    }

    function storeOptions() {
        var db = LocalStorage.openDatabaseSync(db_name);
        db.transaction(
            function(tx) {
                writePreferenceToDB(tx, 'KeepPreferences', keepPreferencesCheckBox.checked)

                writePreferenceToDB(tx, 'ShowAudioInputs', showAudioInputsCheckBox.checked)
                mainGame.toggleAudioInputs(showAudioInputsCheckBox.checked);

                writePreferenceToDB(tx, 'AudioLatency', samplingSlider.value)
                mainGame.setLatency(samplingSlider.value);

                if(mainGame.isDesktop()) {
                    writePreferenceToDB(tx, 'Volume', volumeSlider.value)
                    mainGame.setVolume(volumeSlider.value);
                }
            }
        )
    }

    Column {

        CustomCheckBox {
            id: keepPreferencesCheckBox
            text: qsTr("Keep preferences for next session")
            checked: keepPreferences
            height: lineHeight
            width: window.width
        }

        CustomCheckBox {
            id: showAudioInputsCheckBox
            text: qsTr("Show audio inputs")
            checked: showAudioInputs
            visible: true
            height: lineHeight
            width: window.width
        }

        CustomSlider {
            id: samplingSlider
            height: lineHeight / 2
            width: parent.width * 1.5
            fontColor: "white"
            labelFontSize: lineHeight / 3

            useShifts: false
            unit: "ms"
            label: "Sampling rate / audio latency"
            min: 10
            max: 20
            steps: 10
            value: audioLatency
            defaultValue: 10
        }

        CustomSlider {
            id: volumeSlider
            height: lineHeight / 2
            width: parent.width * 1.5
            fontColor: "white"
            labelFontSize: lineHeight / 3
            visible: mainGame.isDesktop()

            useShifts: false
            unit: "%"
            label: "Sound volume"
            min: 0
            max: 100
            value: volume
            defaultValue: 100
        }

        Row {

            CustomButton {
                id: backButton

                width: standardItemWidth
                height: sectionHeight

                fontSize: standardHeaderPixelSize
                fontColor: standardFontColor
                imageSource: "../graphics/back.png"
                mouseHighlightSource: "../graphics/back_selected.png"
                bold: true
                z: 1
                onClicked: {
                    window.loadView("MainMenu.qml")
                }
            }

            CustomButton {
                id: saveAndBackButton
                buttonText: qsTr("Save")

                width: standardItemWidth
                height: sectionHeight
                fontColor: standardFontColor
                bold: true
                z: 1
                onClicked: {
                    storeOptions()
                    window.loadView("MainMenu.qml")
                }
            }
        }
    }


    Component.onCompleted: {
        addSectionsToFocusChain()
        focusComponents[0].focus = true
        focusComponents[0].forceActiveFocus()
    }

    onConfirm: {
        saveAndBackButton.clicked()
    }

    onBack: {
        backButton.clicked()
    }
}
