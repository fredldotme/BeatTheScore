#ifndef INPUT_H
#define INPUT_H

#include <QObject>
#include <memory>

#include "../music/note.h"

enum InputType {
    DEMO,
    MIDI,
    AUDIO
};

class Input : public QObject
{
    Q_OBJECT

public:
    Input();
    virtual ~Input() {}
    virtual bool listen();
    virtual void stop();
    virtual bool isListening();
    virtual QString getName();
    virtual InputType getType();

private:
    bool listening = false;

signals:
    void noteOn(Note *note);
    void noteOff(Note *note);
};

#endif // INPUT_H
