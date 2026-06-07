#include "note.h"

#include <iostream>

int Note::getTime() const
{
    return time;
}

void Note::setTime(int value)
{
    //cout << "setTime " << value << endl;
    if (value != time)
    {
        time = value;
        emit timeChanged(value);
    }
}

int Note::getDuration() const
{
    return duration;
}

void Note::setDuration(int value)
{
    if (value != duration)
    {
        duration = value;
        emit durationChanged(value);
    }
}

int Note::getPitch() const
{
    return pitch;
}

void Note::setPitch(int value)
{
    if (value != pitch)
    {
        pitch = value;
        emit pitchChanged(value);
    }
}



int Note::getFret() const
{
    return fret;
}

void Note::setFret(int value)
{
    if (value != fret)
    {
        fret = value;
        emit fretChanged(value);
    }
}

int Note::getString() const
{
    return string;
}

void Note::setString(int value)
{
    if (value != string)
    {
        string = value;
        emit stringChanged(value);
    }
}

int Note::getVelocity() const
{
    return velocity;
}

void Note::setVelocity(int value)
{
    if (value != time)
    {
        time = value;
        emit timeChanged(value);
    }
}

void Note::setRelativeDuration(int value)
{
    relativeDuration = value;
}

void Note::setDot(bool value)
{
    dot = value;
}

void Note::setGrace(bool value)
{
    grace = value;
}

void Note::setNoteIndex(int value)
{
    noteIndex = value;
}

void Note::setTuplet(int value)
{
    tuplet = value;
}

int Note::getRelativeDuration()
{
    return relativeDuration;
}

int Note::getNoteIndex()
{
    return noteIndex;
}

int Note::getTuplet()
{
    return tuplet;
}

bool Note::getDot()
{
    return dot;
}

bool Note::getGrace()
{
    return grace;
}


QPointer<Note> Note::getTarget() const
{
    return target;
}

void Note::setTarget(QPointer<Note> value)
{
    target = value;
}

int Note::getScore() const
{
    return score;
}

void Note::setScore(int value)
{
    score = value;
}

long Note::getRealTime() const
{
    return realTime;
}

void Note::setRealTime(long value)
{
    realTime = value;
}

bool Note::isSetSkip()
{
    return this->skipPlay;
}

void Note::skip()
{
    this->skipPlay = true;
}

void Note::unskip()
{
    this->skipPlay = false;
}

LinkingType Note::getLinking() const
{
    return linking;
}

void Note::setLinking(const LinkingType &value)
{
    linking = value;
}

bool Note::pitchOk()
{
    if (target != NULL) {
        return this->getPitch() == target->getPitch();
    }
    return true;
}


bool Note::getMuted() const
{
    return muted;
}

void Note::setMuted(bool value)
{
    muted = value;
}

int Note::getTieDuration() const
{
    return tieDuration;
}

void Note::setTieDuration(int value)
{
    tieDuration = value;
}
Note::Note(const Note &noteEvent, QObject* parent) : QObject(parent)
{
    this->time = noteEvent.time;
    this->duration = noteEvent.duration;
    this->fret = noteEvent.fret;
    this->string = noteEvent.string;
    this->pitch = noteEvent.pitch;
    this->velocity = noteEvent.velocity;
    this->grace = false;
    this->dot = false;
    this->muted = false;
    this->linking = LinkingType::none;
}

Note::Note(const Note &noteEvent) : QObject(NULL)
{
    this->time = noteEvent.time;
    this->duration = noteEvent.duration;
    this->fret = noteEvent.fret;
    this->string = noteEvent.string;
    this->pitch = noteEvent.pitch;
    this->velocity = noteEvent.velocity;
    this->grace = false;
    this->dot = false;
    this->muted = false;
    this->linking = LinkingType::none;
}

Note::Note(int time, int duration, int pitch, int velocity)
{
    this->time = time;
    this->duration = duration;
    this->pitch = pitch;
    this->velocity = velocity;
    this->grace = false;
    this->dot = false;
    this->muted = false;
    this->linking = LinkingType::none;
}

Note::Note(int time, int velocity)
{
    this->time = time;
    this->velocity = velocity;
    this->pitch = 0;
    this->duration = 0;
    this->grace = false;
    this->dot = false;
    this->linking = LinkingType::none;
}

Note::Note(int time, int duration, int fret, int string, int pitch, int velocity)
{
    this->time = time;
    this->duration = duration;
    this->fret = fret;
    this->string = string;
    this->pitch = pitch;
    this->velocity = velocity;
    this->grace = false;
    this->dot = false;
    this->muted = false;
    this->linking = LinkingType::none;
}

Note::Note(QObject *parent)
    :QObject(parent)
{
    this->pitch = 0;
    this->duration = 0;
    this->grace = false;
    this->dot = false;
    this->muted = false;
    this->linking = LinkingType::none;
}

void Note::addPitchShift(int time, int shift)
{
    PitchShiftEvent event(time, shift);

#if 0
    for (const auto& existingEvent : pitchShift) {
        if (existingEvent.time == event.time &&
            existingEvent.pitchShift == event.pitchShift)
        {
            return;
        }
    }
#endif

    pitchShift.append(event);
}

int Note::getPitchShift(int time)
{
    if (pitchShift.isEmpty() || pitchShift.size() == 0)
    {
        return 0;
    }

    if (time < pitchShift[0].time)
    {
        return pitchShift[0].pitchShift;
    }

    for (int i=1; i<pitchShift.size(); i++)
    {
        if (time < pitchShift[i].time)
        {
            float position = (time - pitchShift[i - 1].time) /
                    (float)(pitchShift[i].time - pitchShift[i - 1].time);
            return pitchShift[i - 1].pitchShift * (1 - position) + pitchShift[i].pitchShift * position;
        }
    }

    return pitchShift.last().pitchShift;
}
