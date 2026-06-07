import QtQuick 2.0
import Qt.labs.folderlistmodel 2.0
import QtQuick.Controls 1.0
import QtQuick.LocalStorage 2.0

NavigatablePanel {

    property bool messageBoxVisible: messageBox.visible;

    onMessageBoxVisibleChanged: {
        if(!messageBoxVisible) {
            addSectionsToFocusChain()
            focusComponents[0].focus = true
            focusComponents[0].forceActiveFocus()
        }
    }

    function addSectionsToFocusChain() {
        focusComponents = new Array()
        focusComponents.push(scoreListView);
        focusComponents.push(trackListView);
        if (sectionPreview.listVisible) {
            focusComponents.push(sectionPreview);
        }
        focusComponents.push(startGameButton)
        focusComponents.push(backButton)
        focusComponents.push(deviceListView)
        focusComponents.push(displayOptionsListView)
        focusComponents.push(trainingModeCheckBox)
        focusComponents.push(tempoSlider)
    }

    function requestImage(song, del) {
    }

    function request(url, del, callback) {
            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = (function(myxhr) {

                return function() {
                    // finished
                    if (xhr.readyState === 4) {
                        callback(myxhr, del);
                    }
                }

            })(xhr);
            xhr.open('GET', url, true);
            xhr.send('');
    }

    function indexOf(listView, key) {

        /*if (listView === displayOptionsListView) {
            return listView.model.indexOf(key)
        }*/

        for(var i = 0; i < listView.count; ++i) {
            if (
                listView.model[i] === key ||
                listView.model[i].modelData === key ||
                listView.model[i].getItemId && listView.model[i].getItemId() === key) {
                return i
            }
        }

        console.log("couldn't find key " + key + " in " + listView)
        return 0
    }

    function storePreferences() {
        console.log("storePreferences()")

        var db = LocalStorage.openDatabaseSync(db_name);
        db.transaction(
            function(tx) {
                if (scoreListView.selectedItem)
                    writePreferenceToDB(tx, 'Score', scoreListView.selectedItem.itemId)
                if (trackListView.currentItem)
                    writePreferenceToDB(tx, 'Track', trackListView.currentItem.itemId)
                if (sectionPreview.currentItem)
                    writePreferenceToDB(tx, 'Section', sectionPreview.currentItem.itemId)
                if (deviceListView.currentItem)
                    writePreferenceToDB(tx, 'Device', deviceListView.currentItem.itemId)
                if (displayOptionsListView.currentItem)
                    writePreferenceToDB(tx, 'Display Option', displayOptionsListView.currentItem.itemId)
                writePreferenceToDB(tx, 'Tempo', tempoSlider.value)
                writePreferenceToDB(tx, 'Training Mode', trainingModeCheckBox.checked ? 1 : 0)
            }
        )
    }

    function loadPreferences() {
        console.log("loadPreferences()")

        var db = LocalStorage.openDatabaseSync(db_name)
        var score, track, section, displayOption, device, tempo, trainingMode

        db.transaction(
            function(transaction) {
                // Query Database
                /*score = askDB(transaction, 'Score', '')
                track = askDB(transaction, 'Track', '')
                section = askDB(transaction, 'Section', '')*/
                device = askDB(transaction, 'Device', '')
                displayOption = askDB(transaction, 'Display Option', '')
                tempo = askDB(transaction, 'Tempo', 100)
                trainingMode = askDB(transaction, 'Training Mode', 0)
            }
        )

        /*
        if (mainGame.selectSong(score) === '') {
            sectionPreview.loadSections()
            trackListView.model = mainGame.getAvailableTracks()

            scoreListView.select(indexOf(scoreListView, score))
            trackListView.select(indexOf(trackListView, track))
            sectionPreview.select(indexOf(sectionPreview, section))
        }*/

        deviceListView.select(indexOf(deviceListView, device))
        displayOptionsListView.select(indexOf(displayOptionsListView, displayOption))
        tempoSlider.value = tempo
        trainingModeCheckBox.checked = Number(trainingMode) === Number(1) ? true : false
    }

    function startGame() {

        if (
            scoreListView.selectedItem !== null &&
            trackListView.selectedItem !== null)
        {
            mainGame.setTempo(100 / tempoSlider.value)
            mainGame.selectInput(deviceListView.selectedIndex)
            mainGame.selectTrack(trackListView.selectedIndex)
            mainGame.selectSection(sectionPreview.selectedIndex)

            storePreferences()

            mainGame.setGameMode(0);

            if (trainingModeCheckBox.checked) {
                mainGame.setGameMode(1);
            }

            mainGame.selectInput(deviceListView.currentIndex)

            mainGame.selectTrack(trackListView.currentIndex)
            mainGame.startSong()
            loader.focus = true

            if (displayOptionsListView.currentIndex == 0) {
                window.loadView("PianoRollDemo.qml")
            } else if (displayOptionsListView.currentIndex == 1) {
                window.loadView("ScoreView.qml")
            } else if (displayOptionsListView.currentIndex == 2) {
                window.loadView("TablatureView.qml")
            }
        }
    }

    onConfirm: {
        startGameButton.clicked()
    }

    onBack: {
        backButton.clicked()
    }


    Item {
        id: gameSettings
        visible: true
        anchors.fill: parent
        z: 1

        Column {
            id: gameSettingsColumn
            anchors.fill: parent

            // Song Selection
            Row {
                Column {

                    SelectableListView {
                        id: scoreListView
                        model: mainGame.getAvailableScores()
                        focus: true
                        width: window.width
                        height: sectionHeight
                        itemWidth: standardItemWidth
                        text: qsTr("Score")

                        itemDelegate: SelectableListViewDelegate {

                            id: delegate
                            width: standardItemWidth
                            listView: scoreListView
                            itemId: model.modelData.getItemId()

                            dataItemSource:     Item {
                                id: scoreDataItem
                                anchors.fill: parent
                                z: 1

                                Component.onCompleted: {
                                    requestImage(model.modelData.getTitle(), scoreDataItem)
                                }

                                function setImage(uri) {
                                    delegate.entrySpecificImage.source = uri
                                }

                                Column {

                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.margins: marginInItem


                                    Text {
                                        text: model.modelData.getTitle()
                                        color: standardFontColor
                                        font.bold: highlighted
                                        font.pixelSize: standardFontPixelSize * 3
                                        //elide: Text.ElideRight
                                        wrapMode: Text.WordWrap
                                        width: scoreDataItem.width  - 2 * marginInItem
                                        font.family: headerFont
                                    }

                                    Text {
                                        text: qsTr("by ") + model.modelData.getArtist()
                                        color: standardFontColor
                                        font.pixelSize: standardFontPixelSize
                                        visible: model.modelData.getArtist() !== ""
                                        elide: Text.ElideRight
                                        width: scoreDataItem.width  - 2 * marginInItem
                                    }
                                }
                            }

                        }

                        function applyFilter(text) {
                            model = mainGame.getAvailableScoresFiltered(text)
                        }

                        onSelectedItemChanged: {
                            console.log("onSelectedItemChanged(" + selectedItem + ")")
                            if (scoreListView.selectedItem !== null) {
                                const path = scoreListView.selectedItem.dataOfItem.modelData.getFileName();
                                const result = mainGame.selectSong(path)
                                console.log("result: " + result)
                                if (result === "") {
                                    sectionPreview.loadSections()
                                    addSectionsToFocusChain()
                                    trackListView.model = mainGame.getAvailableTracks()
                                } else {
                                    showMessage(qsTr(
                                        "Sorry, this song contains features that are not supported yet.\n" +
                                        "If you're lucky we're already working on it!") + "\n(" + result + ")");
                                }
                            }
                        }

                        // Keyboard Handling
                        Keys.onPressed: {
                            if (event.key === Qt.Key_Return || event.key === Qt.Key_up) {
                                scoreListView.forceActiveFocus()
                                scoreSearch.text = scoreSearch.text.trim()
                            } else if (scoreSearch.visible && event.key === Qt.Key_Backspace) {
                                scoreSearch.text = scoreSearch.text.substring(0, scoreSearch.text.length - 1)
                                event.accepted = true // backspace shouldn't bring you to prev menu
                            } else if (event.text !== "") {
                                scoreSearch.text += event.text
                                scoreSearch.forceActiveFocus()
                                scoreSearch.visible = true
                            }
                        }

                        Search {
                            id: scoreSearch
                            height: parent.height * (1/5)
                            // aspect ratio of the graphic is 8:1
                            width: height * 8
                            anchors.bottom: parent.bottom
                            visible: alwaysShowSearch
                            onFilterTextChanged: {
                                if (text === "") {
                                    scoreListView.forceActiveFocus()
                                }
                            }
                        }

                        onHeaderDoubleClicked: {
                            scoreSearch.visible = !scoreSearch.visible
                            if (alwaysShowSearch)
                                scoreSearch.visible = true
                            scoreSearch.focus = scoreSearch.visible
                        }

                        /*
                        onFocusChanged: {
                            // hide search when empty
                            if (!focus && !scoreSearch.focus && scoreSearch.text === "")
                                scoreSearch.visible = false
                        }*/
                    }
                }
            }

            // Track selection
            Row {
                SelectableListView {
                    id: trackListView
                    text: qsTr("Track")
                    itemHeight: sectionHeight
                    itemWidth: standardItemWidth
                    hightlightColor: "silver"
                    width: window.width
                    height: sectionHeight
                    model: mainGame.getAvailableTracks()
                }

                onFocusChanged: {
                    if (focus) {
                        trackListView.focus = true
                    }
                }
            }

            // Section selection
            Row {
                SectionPreview {
                    id: sectionPreview
                    text: qsTr("Section")
                    width: window.width
                    height: sectionHeight
                    onSectionSelected: mainGame.selectSection(sectionPreview.selectedIndex)

                    color: "transparent"
                }
            }

            Row {

                width: parent.width

                Column{
                    CustomButton {
                        id: startGameButton
                        buttonText: ""
                        // chordNotes: "CEG"
                        width: standardItemWidth * (2/3)
                        height: sectionHeight * (2/3)
                        z: 1
                        imageSource: "../graphics/play.png"
                        mouseHighlightSource: "../graphics/play_selected.png"
                        onClicked: {
                            startGame()
                        }
                    }


                    CustomButton {
                        id: backButton
                        width: standardItemWidth * (2/3)
                        height: sectionHeight * (1/3)
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

                }

                Column {

                    id: preferenceListViewsColumn

                    width: parent.width - startGameButton.width - optionsColumn.width

                    SelectableListView {
                        id: deviceListView
                        text: qsTr("Device")
                        itemHeight: sectionHeight
                        itemWidth: standardItemWidth
                        hightlightColor: "lightgreen"
                        height: sectionHeight / 2
                        width: parent.width
                        model: mainGame.availableInputs
                    }

                    SelectableListView {
                        id: displayOptionsListView
                        text: qsTr("View")
                        model: mainGame.getAvailableModes()
                        itemWidth: standardItemWidth
                        height: sectionHeight / 2
                        width: parent.width
                    }
                }

                //Column {

                    Item {
                        width: standardItemWidth * (1/32)
                        height: sectionHeight

                    }
                //}

                Column {

                    id: optionsColumn

                    CustomCheckBox {
                        id: trainingModeCheckBox
                        text: qsTr("Training Mode")
                        width: standardItemWidth
                        height: sectionHeight * (3/8)
                    }

                    CustomSlider {
                        id: tempoSlider
                        value: 100
                        min: 50
                        max: 150
                        label: "Tempo"
                        unit: "%  "
                        width: standardItemWidth
                        height: sectionHeight  * (3/8)
                    }
                }
            }
        }

        Component.onCompleted: {
            addSectionsToFocusChain()
            focusComponents[0].focus = true
            focusComponents[0].forceActiveFocus()

            scoreListView.select(indexOf(scoreListView, mainGame.getSelectedScore()))
            trackListView.select(mainGame.getSelectedTrackIndex())
            sectionPreview.select(mainGame.getSelectedSectionIndex())

            if (keepPreferences) {
                loadPreferences()
            }
        }
    }

    property variant disconnectHandler: mainGame.disconnectHandler;

    onDisconnectHandlerChanged: {
        if(disconnectHandler) {
            showMessage(qsTr("Sorry, the connection to your instrument got lost."))
            mainGame.disconnectHandler = false;
        }
    }
}


