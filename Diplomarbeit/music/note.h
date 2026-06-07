#ifndef NOTEEVENT_H
#define NOTEEVENT_H

#include <vector>
#include <QObject>
#include <QPointer>

using namespace std;

struct PitchShiftEvent
{
    int time;
    int pitchShift;
    PitchShiftEvent(int time, int pitchShift) : time(time), pitchShift(pitchShift) {}
};

enum LinkingType
{
    none,
    abrupt, // hammer on or pull off
    slide,
    tie
};

class Note : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int time READ getTime WRITE setTime NOTIFY timeChanged)
    Q_PROPERTY(int duration READ getDuration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(int pitch READ getPitch WRITE setPitch NOTIFY pitchChanged)

private:
    long realTime; // number of milliseconds since the program started
    int time; // number of miliseconds since the song started, 0 if unknown
    int duration; // 0 if the note is still playing
    int pitch; // as specified by the MIDI standard, -1 if unknown, 0 is a valid pitch in MIDI
    int fret, string;
    int noteIndex, relativeDuration, tuplet;
    bool dot, grace;
    int velocity; // values from 0 to 127
    bool muted; // display this note, but don't play it
    int tieDuration;

    LinkingType linking;

    QPointer<Note> target;
    int score = 0;

    bool skipPlay = false; // legacy code?

public:
    Note(const Note& noteEvent, QObject* parent);
    Note(const Note& noteEvent);
    Note(int time, int duration, int pitch, int velocity);
    Note(int time, int duration, int fret, int string, int pitch, int velocity);
    Note(int time, int velocity);
    Note(QObject *parent=0);

    QList<PitchShiftEvent> pitchShift; // pitch shift as (time, pitch) pair
    void addPitchShift(int time, int pitchShift);
    int getPitchShift(int time);
    int getTime() const;
    int getDuration() const;
    int getPitch() const;
    int getFret() const;
    int getString() const;
    int getVelocity() const;

    QPointer<Note> getTarget() const;
    int getScore() const;

    void setRelativeDuration(int value);
    void setNoteIndex(int value);
    void setTuplet(int value);
    void setDot(bool value);
    void setGrace(bool value);

    int getRelativeDuration(); // 0 = quarter, -1 = half, 1 = eighth
    int getNoteIndex(); // index of note in bar
    int getTuplet();
    bool getDot();
    bool getGrace();

    long getRealTime() const;
    void setRealTime(long value);

    bool isSetSkip();
    void skip();
    void unskip();

    LinkingType getLinking() const;
    void setLinking(const LinkingType &value);

    bool pitchOk();

    bool getMuted() const;
    void setMuted(bool value);

    int getTieDuration() const;
    void setTieDuration(int value);

public slots:
    void setTime(int value);
    void setDuration(int value);
    void setPitch(int value);
    void setFret(int value);
    void setString(int value);
    void setVelocity(int value);

    void setTarget(QPointer<Note> value);
    void setScore(int value);

signals:
    void timeChanged(int value);
    void durationChanged(int value);
    void pitchChanged(int value);
    void fretChanged(int value);
    void stringChanged(int value);
    void velocityChanged(int value);

    void targetChanged(QPointer<Note> value);
    void scoreChanged(int value);
};

#endif // NOTEEVENT_H
