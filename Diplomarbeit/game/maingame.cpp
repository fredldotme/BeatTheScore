#include <QMutex>
#include <QMutexLocker>
#include <QDir>
#include <iostream>
#include <vector>
#include "maingame.h"
#include "../music/gp5reader.h"
#include "../sound/noteoutput.h"
#include "../sound/midinoteoutput.h"
#include "../music/track.h"
#include "../game/evaluator.h"
#include "../input/midiinput.h"
#include "../input/audioinput.h"
#include "../input/midiinputmanager.h"
#include "../input/audioinputmanager.h"
#include "../menu/soundnavigationhandler.h"
#include "../menu/scoremanager.h"
#include <stdlib.h>

MainGame::MainGame()
{
    noteOutput = new NoteOutput(this);
    midiOutput = new MidiNoteOutput(noteOutput->audioBuffer, this);

    refreshTimer = new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(refreshInputList()));
    refreshTimer->start(3000); // has to be on same thread as constructor of QTimer

    //connect(checker, SIGNAL(started()), this, SLOT(inputCheckerThread()), Qt::DirectConnection);

    selectedTrack = 0;
    selectedInput = 0;
    selectedSection = 0;
}

void MainGame::cleanup()
{
    checkerRunning = false;
    noteOutput->stopOutput();
    for(int i = 0; i < inputs.count(); i++) {
        inputs[i]->stop();
    }
    refreshTimer->stop();
}

#ifdef Q_OS_ANDROID

void MainGame::setAndroidGlue(AndroidGlue* glue)
{
    this->androidGlue = glue;
}

#endif

void MainGame::deviceDisconnectedHandler()
{
    #ifdef Q_OS_ANDROID
    inputRefreshLock.lock();
    if(!inputs[1].isNull()) {
        androidGlue->reset();
        inputs[1]->stop();
        delete inputs[1].data();
        inputs.removeAt(1);
        emit availableInputsChanged();
    }
    inputRefreshLock.unlock();
    #endif
}

// Interface between C++ & QML for disconnecting the device
bool MainGame::getDisconnect()
{
    return deviceDisconnected;
}

void MainGame::setDisconnect(bool disconnect)
{
    deviceDisconnected = disconnect;
    emit disconnectHandlerChanged();
}

qreal MainGame::loadingProgress()
{
    return progress / (128.0 * 3);
}

void MainGame::refreshInputList()
{
    bool emitChange = false;

    QStack<QString> existingInputs;

    QPointer<Input> demoInput(new Input());
    if(!isInputInList(demoInput)) {
        inputs.append(demoInput);
        emitChange = true;
    }
    existingInputs.push(demoInput->getName());

#ifndef Q_OS_ANDROID
        QList<QPointer<Input>> midiInputs = midiInputManager.getInputs(0);
        for(int i = 0; i < midiInputs.count(); i++) {
            if(!isInputInList(midiInputs[i])) {
                inputs.append(midiInputs[i]);
                if(state != PLAYING && state != WAITING && state != PAUSED && state != RESULT)
                    midiInputs[i]->listen();
                connect(midiInputs[i].data(), SIGNAL(noteOn(Note*)), midiOutput, SLOT(noteOn(Note*)), Qt::DirectConnection);
                connect(midiInputs[i].data(), SIGNAL(noteOff(Note*)), midiOutput, SLOT(noteOff(Note*)), Qt::DirectConnection);
                emitChange = true;
            }
            existingInputs.push(midiInputs[i]->getName());
        }
#else
        if(androidGlue != NULL && androidGlue->isConnected()) {
            QPointer<Input> androidMidiQPtr = QPointer<Input>(new MidiInput(0, 0));
            if (!isInputInList(androidMidiQPtr)) {
                inputs.append(androidMidiQPtr);
                connect(androidMidiQPtr.data(), &Input::noteOn, midiOutput, &MidiNoteOutput::noteOn, Qt::DirectConnection);
                connect(androidMidiQPtr.data(), &Input::noteOff, midiOutput, &MidiNoteOutput::noteOff, Qt::DirectConnection);
                connect(androidMidiQPtr.data(), SIGNAL(deviceDisconnected()), this, SLOT(deviceDisconnectedHandler()), Qt::DirectConnection);
                if(state != PLAYING && state != WAITING && state != PAUSED && state != RESULT)
                    androidMidiQPtr->listen();
                emitChange = true;
            }
            existingInputs.push(androidMidiQPtr->getName());
        }
#endif

    // Append AFTER loop; do not connect Audio Inputs to NoteOutput
    if(/*isDesktop() &&*/ showAudioInputs) {
        QList<QPointer<Input>> audioInputs = AudioInputManager::getInputs(0);
        for(int i = 0; i < audioInputs.size(); i++) {
            if(!isInputInList(audioInputs[i])) {
                /*if (audioInputs[i]->thread() == checker)
                {
                    cout << "moving " << audioInputs[i]->getName().toStdString() << " to main thread" << endl;
                    audioInputs[i]->moveToThread(this->thread());
                }*/
                cout << audioInputs[i]->getName().toStdString() << "->thread() " << audioInputs[i]->thread() << endl;
                inputs.append(audioInputs[i]);
                emitChange = true;
            }
            existingInputs.push(audioInputs[i]->getName());
        }
    }

    int removedBeforeSelected = 0;
    for(int i = inputs.count() - 1; i >= 0; i--) {
        if(inputs[i].isNull() || !existingInputs.contains(inputs[i]->getName())) {
            if(!inputs[i].isNull() && inputs[i]->isListening())
                inputs[i]->stop();

            inputs.removeAt(i);
            emitChange = true;

            if(i == selectedInput) {
                switch(getState()){

                case PLAYING:
                case WAITING:
                case PAUSED:
                case RESULT:
                    // "shit just got real" http://bit.ly/1nLNzlS
                    // emit signal to abort the running game and show message box
                    setDisconnect(true);
                    break;
                default:
                    break;
                }
            }

            if(i <= selectedInput)
                removedBeforeSelected++;
        }
    }

    selectedInput = (selectedInput - removedBeforeSelected > 0) ? (selectedInput - removedBeforeSelected) : 0;

    if(emitChange)
        emit availableInputsChanged();
}

bool MainGame::isInputInList(QPointer<Input> input)
{
    for(int i = 0; i < inputs.count(); i++) {
        if(inputs[i]->getName().compare(input->getName()) == 0) {
            return true;
        }
    }
    return false;
}

void MainGame::toggleAudioInputs(bool start)
{
    inputRefreshLock.lock();
    showAudioInputs = start;
    refreshInputList();
    inputRefreshLock.unlock();
}

void MainGame::setLatency(int value)
{
    noteOutput->setLatency(value);
}

QPointer<SoundNavigationHandler> MainGame::getUiCommandHandler() const
{
    return uiCommandHandler;
}

void MainGame::setUiCommandHandler(const QPointer<SoundNavigationHandler> &value)
{
    uiCommandHandler = value;
}


void MainGame::start()
{
    isPitchingDone = false;
    NoteSamples *pianoSamples = new NoteSamples(MusicInstrument::Piano);
    NoteSamples *guitarSamples = new NoteSamples(MusicInstrument::Guitar);
    NoteSamples *eGuitarSamples = new NoteSamples(MusicInstrument::EGuitar);

    connect(pianoSamples, SIGNAL(pitchChanged()), this, SLOT(progressHandler()), Qt::DirectConnection);
    connect(guitarSamples, SIGNAL(pitchChanged()), this, SLOT(progressHandler()), Qt::DirectConnection);
    connect(eGuitarSamples, SIGNAL(pitchChanged()), this, SLOT(progressHandler()), Qt::DirectConnection);

    pianoSamples->startPitching();
    guitarSamples->startPitching();
    eGuitarSamples->startPitching();

    // Pass the samples
    noteOutput->passSamples(pianoSamples);
    noteOutput->passSamples(guitarSamples);
    noteOutput->passSamples(eGuitarSamples);

    midiOutput->passSamples(pianoSamples);
    midiOutput->passSamples(guitarSamples);
    midiOutput->passSamples(eGuitarSamples);

    cout << "MainGame started" << endl;
    noteOutput->startOutput();
    connect(this, SIGNAL(stateChanged(State)), this, SLOT(pauseMenuHandler(State)), Qt::DirectConnection);
    connect(this, SIGNAL(stateChanged(State)), this, SLOT(resultScreenHandler(State)), Qt::DirectConnection);

    /*
    for (int i=0; i < inputs.size() && isDesktop(); i++)
    {
        cout << "listen... " << i << endl;
        inputs[i]->listen();
        connectToUI(inputs[i]);
    }*/

    // Load file headers
    ScoreManager cadaster;
    availableScores = cadaster.getScoresInFolder(getScoresPath());

    progress = 999;
    emit loadingProgressChanged();

    isPitchingDone = true;

    // Hoffentlich OK
    songSelection();
}

void MainGame::progressHandler()
{
    this->progress += 1.0;
    //cout << "progress " << this->progress << endl;
    emit loadingProgressChanged();
}

void MainGame::mainMenu()
{
    delete evaluator;
    gameClock.reset();
    noteOutput->clear();

    delete score;

    setState(MAIN_MENU);
}

void MainGame::songSelection()
{
    // just in case
    delete evaluator;
    gameClock.reset();
    noteOutput->clear();

    QThread::msleep(100);

    // listen to all inputs again
    for(int i = 0; i < inputs.count(); i++) {
        cout << inputs.size() << endl;
        cout << inputs[i] << endl;
        if(i != selectedInput) {
            inputs[i]->listen();

            if (qobject_cast<MidiInput*>(inputs[i]) != NULL)
            {
                connectToUI(inputs[i]);
            }
        }
    }

    setState(SONG_SELECTION);
}

void MainGame::startSong()
{
    Q_ASSERT(!score.isNull());

    if(isDesktop() && getLicenseState() == UNLICENSED) {
        cleanup();
        exit(1); // Tod- und Mordschlag
    }

    score->setTempo(tempoFactor);

    // let NoteOutput handle training mode orchestration (pause, play, continue)
    noteOutput->clear();
    gameClock.reset();
    if(getGameMode() == GameMode::TRAINING) {
        score->tracks[selectedTrack]->isInputTrack = true;
        noteOutput->assignTrack(score->tracks[selectedTrack]);
    }

    MusicInstrument instrument = NoteSamples::getInstrumentFromGM(score->tracks[selectedTrack]->gMInstrument);
    midiOutput->setMusicInstrument(instrument);

    for(int i = 0; i < score->tracks.size(); i++) {
        if(i != selectedTrack) {
            score->tracks[i]->isInputTrack = false;
            noteOutput->assignTrack(score->tracks[i]);
        }
    }

    //Unlisten unselected input devices
    for(int i = 0; i < inputs.count(); i++) {
        if(i != selectedInput) {
            inputs[i]->stop();
        } else {
            inputs[i]->listen();
        }
    }

    //noteOutput->calculateTimeToLastNote();

    if (!evaluator.isNull()) {
        delete evaluator;
    }
    if (!playerTrack.isNull()) {
        delete playerTrack;
    }
    connectPlayer(inputs[selectedInput], selectedTrack);

    #ifndef Q_OS_ANDROID
    Q_ASSERT(!playerTrack.isNull());
    Q_ASSERT(!playerInput.isNull());
    #endif

    if(getSelectedSectionIndex() > 0) {
        int tmpTime = score->sections[getSelectedSectionIndex()]->getTime();
        gameClock.setGameTime(tmpTime*tempoFactor);
    }

    awaitedNoteTime = 0;
    playedNoteTime = 0;
    setState(GAME_STARTED);
}

void MainGame::startReplay()
{
    delete evaluator;
    gameClock.reset();

    //noteOutput->calculateTimeToLastNote();
    setGameMode(REPLAY);
    awaitedNoteTime = 0;
    playedNoteTime = 0;
    setState(GAME_STARTED);
}

void MainGame::startAgain()
{
    gameClock.reset();
    noteOutput->clear();
    selectSong(selectedFilename);
    awaitedNoteTime = 0;
    playedNoteTime = 0;
    startSong();
}

QString MainGame::selectSong(QString filename)
{
    cout << "Loading song " << filename.toStdString() << endl;

    this->selectedFilename = filename;
    /*if (!score.isNull())
    {
        delete score;
    }*/
    score = QPointer<Score>(new Score());
    GP5Reader reader;
    return QString::fromStdString(reader.read(score, filename));
}

void MainGame::selectInput(int index)
{
    selectedInput = index;
}

void MainGame::selectTrack(int index)
{
    selectedTrack = index;
    if (score.isNull() || score->sections.size() == 0)
    {
        selectedSection = 0;
    }
}

void MainGame::selectSection(int index)
{
    selectedSection = index;
}

void MainGame::setTempo(float factor)
{
    this->tempoFactor = factor;
}

QString MainGame::getSelectedScore()
{
    if (!score.isNull())
    {
        return score->fileName;
    }
    return "";
}

void MainGame::pauseGameClock() {
    gameClock.pause();
}

void MainGame::resumeGameClock() {
    gameClock.resume();
}

void MainGame::pause()
{
    gameClock.pause();
    setState(PAUSED);
}

void MainGame::resume()
{
    gameClock.resume();
    setState(PLAYING);
}

void MainGame::togglePause()
{
    if(mode != TRAINING) {
        if (gameClock.isRunning()) {
            pause();
        } else {
            resume();
        }
    } else {
        cout << "STATE: " << state << endl;
        cout << "OLDSTATE: " << oldState << endl;
        if (state == GAME_STARTED) {
            resume();
        } else if (state == PLAYING && oldState == GAME_STARTED) {
            pause();
        } else if (state == PAUSED && oldState == PLAYING) {
            resume();
        } else if (state == PLAYING && oldState == PAUSED) {
            pause();
        } else if (state == WAITING) {
            setState(PAUSED);
        } else if (state == PAUSED && oldState == WAITING) {
            setState(WAITING);
        }
    }
}

QString MainGame::getScoresPath()
{
    #ifdef Q_OS_ANDROID
    QAndroidJniObject mediaDir = QAndroidJniObject::callStaticObjectMethod("android/os/Environment", "getExternalStorageDirectory", "()Ljava/io/File;");
    QAndroidJniObject mediaPath = mediaDir.callObjectMethod( "getAbsolutePath", "()Ljava/lang/String;" );
    QString dataAbsPath = mediaPath.toString()+"/Scores/";
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        // Handle exception here.
        env->ExceptionClear();
    }
    return dataAbsPath;
    #else
    #ifdef Q_OS_LINUX
    QDir scorePath(QString(getenv("HOME")) + "/Beat the Score");
    if(!scorePath.exists()) {
        scorePath.mkdir(scorePath.path());
    }
    return scorePath.absolutePath();
    #else
    QDir scorePath(QString(getenv("HOMEDRIVE")) + QString(getenv("HOMEPATH")) + "/Beat the Score");
    if(!scorePath.exists()) {
        scorePath.mkdir(scorePath.path());
    }
    return scorePath.absolutePath();
    #endif
#endif
}

void MainGame::increaseVolume()
{
    noteOutput->increaseVolume();
}

void MainGame::decreaseVolume()
{
    noteOutput->decreaseVolume();
}

void MainGame::setVolume(int volume)
{
    noteOutput->setVolume(volume);
}

bool MainGame::isRunning()
{
    return state == State::PLAYING || state == State::WAITING;
}

const bool MainGame::pauseMenuVisible()
{
    return showPauseMenu;
}

const bool MainGame::resultScreenVisible()
{
    return showResultScreen;
}

int MainGame::getScorePercentage()
{
    if (!evaluator.isNull())
    {
        int maxScorePerNote = 100;
        int achieved = (evaluator->totalScore * 100) / (maxScorePerNote * score->tracks.at(getSelectedTrackIndex())->notes.size());
        scoreInReplay = achieved;
        return achieved;
    }

    return scoreInReplay;
}

QStringList MainGame::getAvailableTracks()
{
    if (!score.isNull())
    {
        return score->getTrackNames();
    }
    return QStringList();
}

QStringList MainGame::getAvailableInputs()
{
    inputRefreshLock.lock();
    QStringList list;

    for (int i=0; i<inputs.size(); i++) {
        list.append(inputs[i]->getName());
    }
    inputRefreshLock.unlock();
    return list;
}

QList<QObject*> MainGame::getAvailableSections()
{
    QList<QObject*> sections;

    if (score) {
        for (int i = 0; i < score->sections.length(); i++) {
            sections.append((QObject*) score->sections.at(i));
        }
    }

    return sections;
}

QList<QObject *> MainGame::getAvailableScores()
{
    QList<QObject*> scores;

    for (int i = 0; i < availableScores.length(); i++) {
        scores.append((QObject*) availableScores.at(i));
    }

    return scores;
}

QList<QObject *> MainGame::getAvailableScoresFiltered(QString filterText)
{
    QList<QObject*> scores;
    // TODO: Remove code duplicate
    // TODO: Implement adaptive search
    for (int i = 0; i < availableScores.length(); i++) {
        if (
                availableScores.at(i)->getTitle().toLower().contains(filterText.toLower()) ||
                availableScores.at(i)->getArtist().toLower().contains(filterText.toLower())) {
            scores.append((QObject*) availableScores.at(i));
        }
    }

    return scores;
}

QStringList MainGame::getAvailableModes()
{
    QStringList list;
    list.append(QString("Piano Roll"));
    list.append(QString("Score"));
    if(isDesktop()) {
        list.append(QString("Tablature"));
    }
    return list;
}

int MainGame::getLengthOfSelectedSong()
{
    // Work around, to be replaced with better design
    return score->getLength();
}

void MainGame::connectPlayer(Input *input, int trackIndex)
{
    evaluator = new Evaluator(this, this, score->playableTracks[trackIndex]);
    playerInput = input;
    connect(playerInput.data(), &Input::noteOn, evaluator.data(), &Evaluator::noteOn, Qt::UniqueConnection);
    connect(playerInput.data(), &Input::noteOff, evaluator.data(), &Evaluator::noteOff, Qt::UniqueConnection);
    playerTrack = new Track(this);
    connect(evaluator.data(), &Evaluator::noteAdded, playerTrack.data(), &Track::addNote);
}

void MainGame::connectToUI(Input *input)
{
    if (!input)
        return;

    connect(input, &Input::noteOn, this->getUiCommandHandler().data(), &SoundNavigationHandler::noteOn, Qt::UniqueConnection);
    connect(input, &Input::noteOff, this->getUiCommandHandler().data(), &SoundNavigationHandler::noteOff, Qt::UniqueConnection);
}

void MainGame::disconnectFromUI(Input *input)
{
    if (!input)
        return;

    disconnect(input, &Input::noteOn, this->getUiCommandHandler().data(), &SoundNavigationHandler::noteOn);
    disconnect(input, &Input::noteOff, this->getUiCommandHandler().data(), &SoundNavigationHandler::noteOff);
}

int MainGame::getSelectedTrackIndex()
{
    return selectedTrack;
}

int MainGame::getSelectedSectionIndex()
{
    return selectedSection;
}

int MainGame::getGameTime()
{
    return gameClock.getGameTime();
}

void MainGame::setGameTime(int time)
{
    gameClock.setGameTime(time);
}

QPointer<Input> MainGame::getCurrentInput()
{
    return playerInput;
}

QPointer<Score> MainGame::getScore()
{
    return score;
}

QPointer<Track> MainGame::getTargetTrack()
{
    Q_ASSERT(!score.isNull());
    return score->playableTracks[selectedTrack];
}

QPointer<Track> MainGame::getPlayerTrack()
{
    return playerTrack;
}

void MainGame::setState(State state)
{
    stateMutex.lock();
    this->oldState = this->state;
    emit stateChanged(state);
    this->state = state;
    stateMutex.unlock();
}

State MainGame::getState() {
    return this->state;
}

void MainGame::setGameMode(int mode) {
    this->mode = GameMode(mode);
}

GameMode MainGame::getGameMode() {
    return this->mode;
}

int MainGame::getStartIndex(int trackId)
{
    return 0;
}

void MainGame::awaitNote(int time)
{
    awaitedNoteTime = max(awaitedNoteTime, time);
    if (awaitedNoteTime > playedNoteTime)
    {
        pause();
        setState(State::WAITING);
        gameClock.setGameTime(awaitedNoteTime);
    }
}

void MainGame::nextNote(int time)
{
    playedNoteTime = max(playedNoteTime, time);
    if (awaitedNoteTime <= playedNoteTime)
    {
        resume();
        //setState(State::PLAYING);
        //gameClock.setGameTime(playedNoteTime);
    }
}

void MainGame::setLicenseManager(QPointer<LicenseManager> licenseManager)
{
    this->licenseManager = licenseManager;
    connect(licenseManager.data(), SIGNAL(isLicenseValidChanged()), this, SLOT(licenseValidityChanged()));
}

void MainGame::licenseValidityChanged()
{
    if(licenseManager->isLicenseValid() == LICENSE_VALID) {
        this->licenseState = LICENSED;
    } else {
        this->licenseState = UNLICENSED;
    }
}

LicenseState MainGame::getLicenseState()
{
#ifdef Q_OS_WINDOWS
    if(isDesktop()) {
        return this->licenseState;
    }
#else
    {
        /* On Android we could check the actual license via the Play Store,
           but that would involve a lot of JNI headaches. */
        return LicenseState::LICENSED;
    }
#endif
}

void MainGame::pauseMenuHandler(State state)
{
    if(state == PAUSED) {
        showPauseMenu = true;
    } else if (showPauseMenu == true){
        showPauseMenu = false;
    }
    emit pauseMenuToggled();
}

void MainGame::resultScreenHandler(State newState)
{
    if(newState == RESULT && this->state == PLAYING) {
        gameClock.pause();
        showResultScreen = true;
    } else {
        showResultScreen = false;
    }
    emit resultScreenToggled();
}

bool MainGame::isDesktop()
{
#ifdef Q_OS_ANDROID
    return false;
#elif defined(Q_OS_LINUX)
    return true;
#else
    return true;
#endif
}

bool MainGame::isReplayMode()
{
    return mode == REPLAY;
}

bool MainGame::isResultScreen()
{
    return state == RESULT;
}

void MainGame::changeFullScreen()
{
    uiCommandHandler->changeFullScreen();
}

void MainGame::clearAudioBuffer()
{
    noteOutput->audioBuffer->clear();
}
