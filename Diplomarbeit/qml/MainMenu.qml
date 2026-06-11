import QtQuick 2.0
import QtQuick.LocalStorage 2.0

NavigatablePanel {

    width: 1200
    height: 800

    function addSectionsToFocusChain() {
        focusComponents.push(startButton)
        focusComponents.push(optionsButton)
        focusComponents.push(helpButton)
        focusComponents.push(exitButton)
    }

    function loadGlobalOptions() {
        var db = LocalStorage.openDatabaseSync(db_name)
        db.transaction(
            function(tx) {
                // Query Database
                keepPreferences = Number(askDB(tx, 'KeepPreferences', 1))  === Number(1) ? true : false;
                showAudioInputs = Number(askDB(tx, 'ShowAudioInputs', 1))  === Number(1) ? true : false;
                mainGame.toggleAudioInputs(showAudioInputs)
                audioLatency = Number(askDB(tx, 'AudioLatency', 10));
                mainGame.setLatency(audioLatency);

                if(mainGame.isDesktop()) {
                    volume = Number(askDB(tx, 'Volume', 100));
                    mainGame.setVolume(volume);
                }
            }
        )
    }

    Item {
        id: mainView
        visible: true
        anchors.fill: parent


        Image {
            // TODO: reintroduce on Android
            source: "../graphics/logo.png"
            width: mainView.width / 2
            height: mainView.width / 2 *  sourceSize.height /  sourceSize.width
            anchors.top: parent.top
            anchors.left: parent.left
        }

        Column {
            id: menuOptionRow
            spacing: height / 8
            anchors.centerIn: parent
            width: parent.width
            height: parent.height

            CustomButton {
                id: startButton
                buttonText: "Start"
                width: parent.width
                height: parent.height / 8
                onClicked: {
                    window.loadView("GameSettings.qml")
                }
            }

            CustomButton {
                id: optionsButton
                buttonText: "Options"
                height: parent.height / 8
                width: parent.width
                onClicked: {
                    window.loadView("Options.qml")
                }
            }

            CustomButton {
                id: helpButton
                buttonText: "Help"
                width: parent.width
                height: parent.height / 8
                onClicked: {
                    window.loadView("Help.qml")
                }
            }

            CustomButton {
                id: exitButton
                buttonText: "Exit"
                width: parent.width
                height: parent.height / 8
                onClicked:  Qt.quit()
            }
        }

        Component.onCompleted: {
            addSectionsToFocusChain()
            focusComponents[0].focus = true
            focusComponents[0].forceActiveFocus()
            createTableIfNotExistant()
            loadGlobalOptions()
        }
    }

    onConfirm: {
        startButton.clicked()
    }
}



