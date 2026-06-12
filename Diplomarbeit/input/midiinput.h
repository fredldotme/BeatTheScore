#ifndef MIDIINPUT_H
#define MIDIINPUT_H

#include <string>
#include <vector>
#include <memory>

#include <QThread>
#include <QList>
#include <QPointer>
#include "RtMidi.h"

class MidiInput;

#include "input.h"
#include "input-android/androidglue.h"

#ifdef Q_OS_ANDROID
#include <libusb.h>
#endif

using namespace std;

class MidiInputThread : public QThread
{
    Q_OBJECT

public:
    MidiInputThread(RtMidiIn *midiin, int port);
    ~MidiInputThread();

private:
    vector<QPointer<Note>> openNoteEvents;
    void run();
    bool isNoteOn(vector<unsigned char> message);
    bool isNoteOff(vector<unsigned char> message);
    bool isPitchWheel(int statusMessage);
    Note *getCorrespondingEvent(int pitch);
    void applyPitchShiftToOpenEvents(int pitchShift);
    RtMidiIn *midiin = NULL;
    int port;
    bool isRunning = true;
    #ifdef Q_OS_ANDROID
    shared_ptr<AndroidGlue> androidGlue;
    #endif
    int dateCounter;

signals:
    void noteOn(Note *note);
    void noteOff(Note *note);
    void deviceDisconnected();
};

class MidiInput : public Input
{
    Q_OBJECT

private:
    int portNumber;
    MidiInputThread *thread;
    QString name;
    RtMidiIn *midiin;

public slots:
    void deviceDisconnectedBroker();

signals:
    void deviceDisconnected();

public:
    ~MidiInput();

    MidiInput(QObject *parent, int port);
    bool listen();
    void stop();
    bool isListening();
    QString getName();
    InputType getType();
};

#endif // MIDIINPUT_H
