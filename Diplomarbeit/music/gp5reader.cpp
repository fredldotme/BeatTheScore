#include "gp5reader.h"
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <stdio.h>
#include <QPointer>

GP5Reader::GP5Reader()
{
    // ASSUMES LITTLE-ENDIAN-ENCODING!
    Q_ASSERT(Q_BYTE_ORDER == Q_LITTLE_ENDIAN);
    Q_ASSERT(sizeof(char) == 1);
    Q_ASSERT(sizeof(short) == 2);
    Q_ASSERT(sizeof(int) == 4);
    /*
#ifdef Q_OS_ANDROID
    freopen ("/storage/emulated/0/btslog.txt","w",stdout);
#else
    freopen ("GP5Reader.log","w",stdout);
#endif
*/
}

void readGPInteger(ifstream &stream, int &integer)
{
    stream.read((char*)&integer, 4);
}

string readGPString(ifstream &stream)
{
    unsigned int length = 0;
    stream.read((char*)&length, 1);
    //cout << "read string of length " << length << "-" << stream.rdstate() << endl;
    char* text = new char[length + 1];
    text[length] = '\0';
    if (length > 0)
    {
        stream.read(text, length);
    }
    return string(text);
}

string readdGPStringBlock(ifstream &stream)
{
    stream.ignore(4); // redundant information is redundant!
    return readGPString(stream);
}

void ignoreGPDataBlocks(ifstream &stream, int blocks = 1)
{
    while (blocks > 0)
    {
        int length;
        readGPInteger(stream, length);
        if (length > 500)
        {
            //cout << "error @" << ((int)stream.tellg() - 4) << endl;
        }
        //cout << "ignoring " << length << "bytes" << "-" << stream.rdstate() << endl;
        stream.ignore(length);
        blocks--;
    }
}

void readBendChunk(ifstream &stream, vector<PitchShiftEvent> &pitches, int noteLength)
{
    stream.ignore(5); // TODO
    int pointCount;
    readGPInteger(stream, pointCount);
    int currentTime = 0;

    for (int i=0; i<pointCount; i++)
    {
        int timeDelta;
        readGPInteger(stream, timeDelta);
        currentTime += timeDelta * noteLength / 60;
        int pitchShift;
        readGPInteger(stream, pitchShift);
        stream.ignore(1);
        pitches.push_back(PitchShiftEvent(currentTime, pitchShift));
    }
}

struct bar
{
    int denominator;
    int numerator;
    bool beginRepeat;
    int repeatCount; // 0 if not repeat end
};

string GP5Reader::readHead(QPointer<Score> score, string filename)
{
    ifstream fileStream (filename, ios::in | ios::binary);

    if (fileStream.rdstate() & ifstream::failbit) {
        return "can't open file";
    }

    string version = readGPString(fileStream);
    int versionMajor = version[20] - '0';
    int versionMinor = (version[22] - '0') * 10 + version[23] - '0';
    //cout << "Version = " << versionMajor << "." << versionMinor << "-" << fileStream.rdstate() << endl;
    fileStream.ignore(30 - version.length());

    int tripletFeel = 0;
    int tempo, quarterNoteLength;
    int barCount, trackCount;
    bar *barInfo;

    /// META-INFO
    {
        score->fileName = QString::fromStdString(filename);
        score->title = QString::fromStdString(readdGPStringBlock(fileStream));
        score->subtitle = QString::fromStdString(readdGPStringBlock(fileStream));
        score->artist = QString::fromStdString(readdGPStringBlock(fileStream));
        score->album = QString::fromStdString(readdGPStringBlock(fileStream));
        if (versionMajor >= 5)
        {
            readdGPStringBlock(fileStream);
        }
        score->composer = QString::fromStdString(readdGPStringBlock(fileStream));
        readdGPStringBlock(fileStream);
        score->transcriber = QString::fromStdString(readdGPStringBlock(fileStream));
        readdGPStringBlock(fileStream);
        /// Notes
        {
            int noticeCount;
            readGPInteger(fileStream, noticeCount);
            if (noticeCount > 100)
            {
                return "too many notices";
            }
            //cout << "noticeCount = " << noticeCount << endl;
            ignoreGPDataBlocks(fileStream, noticeCount);
        }
        if (versionMajor <= 4)
            fileStream.read((char*)&tripletFeel, 1);
        if (versionMajor >= 4)
        { // lyrics
            fileStream.ignore(4);
            for (int i=0; i<5; ++i)
            {
                fileStream.ignore(4);
                int length;
                readGPInteger(fileStream, length);
                fileStream.ignore(length);
            }
        }
        if (versionMajor >= 5 && versionMinor > 0)
            fileStream.ignore(19); // Equalizer
        if (versionMajor >= 5)
        { // ignore page setup
            fileStream.ignore(30); // page, score size and margin
            //cout << "@" << fileStream.tellg() << endl;
            ignoreGPDataBlocks(fileStream, 10); // text on page
        }
    }

    fileStream.close();
    return "";
}

string GP5Reader::read(QPointer<Score> score, QString filename)
{
    // TODO: use the readHeader function!
    ifstream fileStream (filename.toStdString(), ios::in | ios::binary);

    if (fileStream.rdstate() & ifstream::failbit) {
        return "can't open file";
    }

    score->fileName = filename;

    string version = readGPString(fileStream);
    int versionMajor = version[20] - '0';
    int versionMinor = (version[22] - '0') * 10 + version[23] - '0';
    //cout << "Version = " << versionMajor << "." << versionMinor << "-" << fileStream.rdstate() << endl;
    fileStream.ignore(30 - version.length());

    int tripletFeel = 0;
    int tempo, quarterNoteLength;
    int barCount, trackCount;
    bar *barInfo;
    int instrument[64];

    /// META-INFO
    {
        score->title = QString::fromStdString(readdGPStringBlock(fileStream));
        score->subtitle = QString::fromStdString(readdGPStringBlock(fileStream));
        score->artist = QString::fromStdString(readdGPStringBlock(fileStream));
        score->album = QString::fromStdString(readdGPStringBlock(fileStream));
        if (versionMajor >= 5)
            readdGPStringBlock(fileStream);
        score->composer = QString::fromStdString(readdGPStringBlock(fileStream));
        readdGPStringBlock(fileStream);
        score->transcriber = QString::fromStdString(readdGPStringBlock(fileStream));
        readdGPStringBlock(fileStream);
        /// Notes
        {
            int noticeCount;
            readGPInteger(fileStream, noticeCount);
            if (noticeCount > 100)
            {
                return "too many notices";
            }
            //cout << "noticeCount = " << noticeCount << endl;
            ignoreGPDataBlocks(fileStream, noticeCount);
        }
        if (versionMajor <= 4)
            fileStream.read((char*)&tripletFeel, 1);
        if (versionMajor >= 4)
        { // lyrics
            fileStream.ignore(4);
            for (int i=0; i<5; ++i)
            {
                fileStream.ignore(4);
                int length;
                readGPInteger(fileStream, length);
                fileStream.ignore(length);
            }
        }
        if (versionMajor >= 5 && versionMinor > 0)
            fileStream.ignore(19); // Equalizer
        if (versionMajor >= 5)
        { // ignore page setup
            fileStream.ignore(30); // page, score size and margin
            //cout << "@" << fileStream.tellg() << endl;
            ignoreGPDataBlocks(fileStream, 10); // text on page
        }
    }

    /// ACTUAL SONG DATA
    {
        if (versionMajor >= 5)
            ignoreGPDataBlocks(fileStream, 1); // ignore text representation of tempo
        readGPInteger(fileStream, tempo);
        quarterNoteLength = 60000 / tempo;
        //cout << "tempo = " << tempo << endl;
        //cout << "quarterNoteLength = " << quarterNoteLength << endl;
        //cout << "@" << fileStream.tellg() << endl;
        if (versionMajor >= 5 && versionMinor > 0)
            fileStream.ignore(1);
        int signature = 0;
        if (versionMajor >= 4)
        {
            fileStream.read((char*)&signature, 1);
            //cout << "signature = " << signature << endl;
            fileStream.ignore(3);
            fileStream.ignore(1); // ignore octave info
        }
        if (versionMajor < 4)
            readGPInteger(fileStream, signature);
        for(int i=0; i<64; i++)
        {
            readGPInteger(fileStream, instrument[i]);
            //cout << instrument[i] << ", ";
            fileStream.ignore(8);
        }
        if (versionMajor >= 5)
        {
            fileStream.ignore(38); // TODO: read musical directions
            fileStream.ignore(4); // ignore reverb
        }
        readGPInteger(fileStream, barCount);
        if (barCount > 1000)
        {
            //cout << "too many bars! " << barCount << endl;
            fileStream.close();
            return "too many bars";
        }
        readGPInteger(fileStream, trackCount);
        if (trackCount > 20)
        {
            //cout << "too many tracks! " << trackCount << endl;
            fileStream.close();
            return "too many tracks";
        }
        //cout << "barCount = " << barCount << ", trackCount = " << trackCount << endl;
        barInfo = new bar[barCount];
    }

    /// BARS
    {
        int numerator = 0;
        int denominator = 0;
        for (int barIndex=0; barIndex<barCount; barIndex++)
        {
            //cout << endl << "bar " << barIndex << " @" << fileStream.tellg() << ":" << endl;
            unsigned char bitmask;
            fileStream.read((char*)&bitmask, 1);
            //cout << "mask = " << (int) bitmask << endl;
            // TODO: some special voodoo for gp2 and earlier
            if (versionMajor >= 3)
            {
                if (bitmask & 1) // numerator
                {
                    fileStream.read((char*)&numerator, 1);
                    //cout << "Numerator = " << numerator << endl;
                }
                barInfo[barIndex].numerator = numerator;
                if (bitmask & 2) // denominator
                {
                    fileStream.read((char*)&denominator, 1);
                    //cout << "Denominator = " << denominator << endl;
                }
                barInfo[barIndex].denominator = denominator;
                if (bitmask & 4) // begin repeat
                {
                    barInfo[barIndex].beginRepeat = true;
                    //cout << "Begin repeat" << endl;
                }
                else
                {
                    barInfo[barIndex].beginRepeat = false;
                }
                if (bitmask & 8) // end repeat
                {
                    int repeatCount = 0;
                    fileStream.read((char*)&repeatCount, 1);
                    //cout << "Repeat " << repeatCount << " times" << endl;
                    barInfo[barIndex].repeatCount = repeatCount;
                }
                else
                {
                    barInfo[barIndex].repeatCount = 0;
                }
                if (versionMajor < 5)
                {
                    if (bitmask & 16) // ending count
                    {
                        int endingCount = 0;
                        fileStream.read((char*)&endingCount, 1);
                        //cout << "endingCount = " << endingCount << endl;
                    }
                    if (bitmask & 32) // section
                    {
                        score->sections.append(new Section(barIndex, 0, 0, QString::fromStdString(readdGPStringBlock(fileStream))));
                        fileStream.ignore(4);
                        //cout << "Section" << endl;
                    }
                }
                else
                {   // yep, they just switched those two fields from gp4 to gp5 -_-
                    if (bitmask & 32) // section
                    {
                        score->sections.append(new Section(barIndex, 0, 0, QString::fromStdString(readdGPStringBlock(fileStream))));
                        fileStream.ignore(4);
                        //cout << "Section" << endl;
                    }
                    if (bitmask & 16) // ending count
                    {
                        int endingCount = 0;
                        fileStream.read((char*)&endingCount, 1);
                        //cout << "endingCount = " << endingCount << endl;
                    }
                }
                if (bitmask & 64)
                {
                    int tonality = 0;
                    fileStream.read((char*)&tonality, 2);
                    //cout << "Tonality = " << tonality << endl;
                }
                if (versionMajor >= 5)
                {
                    if (bitmask & 3)
                    {
                        int beamEightNotesByValues; // No idea what that is...
                        readGPInteger(fileStream, beamEightNotesByValues);
                        //cout << "beamEightNotesByValues = " << beamEightNotesByValues << endl;
                    }
                    fileStream.read((char*)&tripletFeel, 2);
                    //cout << "tripletFeel = " << tripletFeel << endl;
                    fileStream.ignore();
                }
            }
        }
    }

    /// TRACKS
    {
        for (int i=0; i<trackCount; i++)
        {
            QPointer<Track> track = new Track(score);
            track->gMInstrument = instrument[i];
            //cout << endl << "track " << i << " @" << fileStream.tellg() << ":" << endl;
            unsigned char bitmask;
            fileStream.read((char*)&bitmask, 1);
            //cout << "mask = " << (int) bitmask << endl;
            string trackName = readGPString(fileStream);
            track->name = QString::fromStdString(trackName);
            //cout << "name = " << trackName << endl;
            fileStream.ignore(40 - trackName.length());
            readGPInteger(fileStream, track->stringCount);
            if (track->stringCount > 8)
            {
                //cout << "too many strings! " << track->stringCount << endl;
                fileStream.close();
                return "too many strings";
            }
            //cout << "stringCount = " << track->stringCount << endl;
            track->tuning.clear();
            for (int j=0; j<track->stringCount; j++)
            {
                int tune;
                readGPInteger(fileStream, tune);
                track->tuning.push_back(tune);
                //cout << "tuning[" << j << "] = " << track->tuning[j] << endl;
            }
            if (track->stringCount > 0)
            {
                fileStream.ignore(4 * (7 - track->stringCount));
            }
            fileStream.ignore(4);
            int channel;
            readGPInteger(fileStream, channel);
            if (channel == 10)
            {
                track->isPercussion = true;
            }
            fileStream.ignore(16);
            if (versionMajor >= 5) // unecessary meta info
            {
                if (versionMinor > 0)
                {
                    fileStream.ignore(49);
                    ignoreGPDataBlocks(fileStream, 2);
                }
                else
                {
                    fileStream.ignore(45);
                }
            }
            score->tracks.push_back(track);
            if (!track->isPercussion)
            {
                score->playableTracks.push_back(track);
            }
        }
        if (versionMajor >= 5) // padding
        {
            fileStream.ignore();
        }
    }

    /// BEATS
    {
        QList<QMap<int, Note *>> tieStarts; // openTies[trackIndex][stringIndex]
        QList<QMap<int, Note *>> previousNotes; // openTies[trackIndex][stringIndex]
        for (int i=0; i<trackCount; i++)
        {
            previousNotes.append(QMap<int, Note *>());
            tieStarts.append(QMap<int, Note*>());
        }

        int currentSectionIndex = -1;
        int barTime = quarterNoteLength * 4; // one bar pause at the start
        int repeatStart = barTime;
        int quarterNotesCount = 0;
        for (int barIndex=0; barIndex<barCount; barIndex++)
        {
            bool sectionChanged = false;
            if (currentSectionIndex + 1 < score->sections.size() &&
                    barIndex == score->sections[currentSectionIndex + 1]->index)
            {
                currentSectionIndex++;
                if (currentSectionIndex > 0) {
                    score->sections[currentSectionIndex - 1]->length = barTime - score->sections[currentSectionIndex - 1]->time;
                }
                score->sections[currentSectionIndex]->time = barTime;
                sectionChanged = true;
            }

            score->bars.push_back(barTime);
            if (barInfo[barIndex].beginRepeat)
            {
                repeatStart = barTime;
            }
            for (int trackIndex=0; trackIndex<trackCount; trackIndex++)
            {
                if (sectionChanged)
                {
                    score->tracks[trackIndex]->firstNoteInSection.append(score->tracks[trackIndex]->notes.size());
                }

                int voiceCount = 1;
                if (versionMajor >= 5)
                    voiceCount = 2;
                for (int voiceIndex=0; voiceIndex<voiceCount; voiceIndex++)
                {
                    int beatCount;
                    readGPInteger(fileStream, beatCount);
                    if (beatCount > 50)
                    {
                        fileStream.close();
                        //cout << "too many beats! " << beatCount << endl;
                        return "too many beats";
                    }
                    int currentTime = barTime;
                    for (int beatIndex=0; beatIndex<beatCount; beatIndex++)
                    {
                        /*cout << endl
                             << "track " << trackIndex
                             << ", bar " << barIndex
                             << ", voice " << voiceIndex
                             << ", beat " << beatIndex
                             << ": " << currentTime / 1000 / 60 << ":" << (currentTime / 1000) % 60 << "." << currentTime % 1000
                             << " @" << fileStream.tellg() << ":" << endl;*/
                        char bitmask;
                        fileStream.read(&bitmask, 1);
                        //cout << "mask = " << (int)bitmask << endl;
                        if (bitmask & 64) // rest type (TODO)
                        {
                            char restType;
                            fileStream.read(&restType, 1);
                        }
                        signed char duration;
                        fileStream.read((char *)&duration, 1);
                        if (duration < -2 || duration > 4)
                        {
                            //cout << "illegal duration @" << fileStream.tellg() << endl;
                            fileStream.close();
                            return "illegal duration";
                        }
                        int durationTime = quarterNoteLength / pow(2.0, duration);
                        bool dot = false;
                        if (bitmask & 1) // dotted note
                        {
                            dot = true;
                            durationTime = durationTime * 3 / 2;
                        }
                        int tupletSize = 1;
                        if (bitmask & 32) // N-tuplet
                        {
                            readGPInteger(fileStream, tupletSize);
                            durationTime = durationTime * (tupletSize - 1) / tupletSize;
                        }
                        //cout << "timeDuration = " << durationTime << endl;
                        if (bitmask & 2) // chord
                        {
                            char chordVersion;
                            fileStream.read(&chordVersion, 1);
                            if (chordVersion == 0)
                            {
                                //cout << "new chord type" << endl;
                                string chordName = readdGPStringBlock(fileStream);
                                //cout << "chordName = " << chordName;
                                int startFret;
                                readGPInteger(fileStream, startFret);
                                if (startFret != 0)
                                {
                                    for (int stringIndex=0; stringIndex<score->tracks[trackIndex]->stringCount; stringIndex++)
                                    {
                                        int fretNumber;
                                        readGPInteger(fileStream, fretNumber);
                                        if (fretNumber != -1)
                                        {
                                            shared_ptr<Note> note(new Note());
                                            note->setDuration(durationTime);
                                            note->setTime(currentTime);
                                            note->setFret(fretNumber);
                                            note->setString(stringIndex);
                                            note->setPitch(score->tracks[trackIndex]->tuning[stringIndex] + fretNumber);
                                            note->setRelativeDuration(duration);
                                            note->setNoteIndex(beatIndex);
                                            note->setDot(dot);
                                            note->setTuplet(tupletSize);
                                            //cout << "pitch = " << note->getPitch() << endl;
                                            //score->tracks[trackIndex]->addNoteEvent(note);
                                        }
                                    }
                                }
                            }
                            else if (chordVersion == 1) // TODO
                            {
                                //cout << "old chord type" << endl;
                                bool sharp;
                                fileStream.read((char*)&sharp, 1);
                                fileStream.ignore(3);
                                char chordRoot;
                                fileStream.read(&chordRoot, 1);
                                if (versionMajor == 3)
                                {
                                    fileStream.ignore(3);
                                }
                                char chordType;
                                fileStream.read(&chordType, 1);
                                if (versionMajor == 3)
                                {
                                    fileStream.ignore(3);
                                }
                                char n;
                                fileStream.read(&n, 1);
                                if (versionMajor == 3)
                                {
                                    fileStream.ignore(3);
                                }
                                int bassNote;
                                readGPInteger(fileStream, bassNote);
                                char sign;
                                fileStream.read(&sign, 1);
                                string chordName = readdGPStringBlock(fileStream);
                                fileStream.ignore(20 - chordName.length());
                                //cout << "chordName = " << chordName << endl;
                                fileStream.ignore(2);
                                char tonality5;
                                fileStream.read(&tonality5, 1);
                                if (versionMajor == 3)
                                {
                                    fileStream.ignore(3);
                                }
                                char tonality9;
                                fileStream.read(&tonality9, 1);
                                if (versionMajor == 3)
                                {
                                    fileStream.ignore(3);
                                }
                                char tonality11;
                                fileStream.read(&tonality11, 1);
                                if (versionMajor == 3)
                                {
                                    fileStream.ignore(3);
                                }
                                int baseFret;
                                readGPInteger(fileStream, baseFret);
                                for (int stringIndex=0; stringIndex < 7; stringIndex++)
                                {
                                    int fretNumber;
                                    readGPInteger(fileStream, fretNumber);
                                    //cout << stringIndex << ": " << fretNumber << " ";
                                    if (fretNumber != -1 && stringIndex < score->tracks[trackIndex]->stringCount)
                                    {
                                        shared_ptr<Note> note(new Note());
                                        note->setDuration(durationTime);
                                        note->setTime(currentTime);
                                        note->setFret(fretNumber);
                                        note->setString(stringIndex);
                                        note->setPitch(score->tracks[trackIndex]->tuning[stringIndex] + fretNumber);
                                        note->setRelativeDuration(duration);
                                        note->setNoteIndex(beatIndex);
                                        note->setDot(dot);
                                        note->setTuplet(tupletSize);
                                        //cout << score->tracks[trackIndex]->tuning[stringIndex] << ", pitch = " << note->getPitch();
                                        //score->tracks[trackIndex]->addNoteEvent(note);
                                    }
                                    //cout << endl;
                                }
                                fileStream.ignore(16); // barre
                                fileStream.ignore(8); // intervals
                                fileStream.ignore(8); // fingering
                            }
                        }
                        if (bitmask & 4) // text
                        {
                            readdGPStringBlock(fileStream);
                        }
                        vector<PitchShiftEvent> strokePitchShift;
                        if (bitmask & 8) // effect
                        {
                            int effectBitmask = 0;
                            fileStream.read((char*)&effectBitmask, 1);
                            if (versionMajor >= 4)
                            {
                                fileStream.read((char*)&effectBitmask + 1, 1);
                            }
                            // effectBitmask & 1 = Vibrato
                            // effectBitmask & 2 = Wide vibrato
                            // effectBitmask & 4 = Natural harmonic
                            // effectBitmask & 8 = Artificial harmonic
                            // effectBitmask & 16 = Fade in
                            if (effectBitmask & 32) // string effect
                            {
                                char stringEffect;
                                fileStream.read(&stringEffect, 1);
                                // 0 = Tremolo bar
                                // 1 = Tapping
                                // 2 = Slapping
                                // 3 = Popping (bass guitar)
                                if (versionMajor < 4)
                                {
                                    int effectValue;
                                    readGPInteger(fileStream, effectValue);
                                }
                            }
                            if (effectBitmask & 1024) // Tremolo bar
                            {
                                readBendChunk(fileStream, strokePitchShift, durationTime);
                            }
                            if (effectBitmask & 64) // Stroke effect
                            {
                                fileStream.ignore(2);
                                //cout << "stroke effect ignored!" << endl;
                            }
                            // effectBitmask & 256 = Rasguedo
                            if (effectBitmask & 512) // Pickstroke
                            {
                                char strokeDirection;
                                fileStream.read(&strokeDirection, 1);
                                // 1 = up
                                // 2 = down
                                // 0 = none?
                            }
                        }
                        if (bitmask & 16) // mix table
                        {
                            fileStream.ignore(1);
                            if (versionMajor >= 5)
                            {
                                fileStream.ignore(16);
                            }
                            char newVolume;
                            fileStream.read(&newVolume, 1);
                            char newPan;
                            fileStream.read(&newPan, 1);
                            char newChorus;
                            fileStream.read(&newChorus, 1);
                            char newReverb;
                            fileStream.read(&newReverb, 1);
                            char newPhaser;
                            fileStream.read(&newPhaser, 1);
                            char newTremolo;
                            fileStream.read(&newTremolo, 1);
                            if (versionMajor >= 5)
                            {
                                readdGPStringBlock(fileStream);
                            }
                            int newTempo;
                            readGPInteger(fileStream, newTempo);
                            if (newVolume != -1)
                                fileStream.ignore(1);
                            if (newPan != -1)
                                fileStream.ignore(1);
                            if (newChorus != -1)
                                fileStream.ignore(1);
                            if (newReverb != -1)
                                fileStream.ignore(1);
                            if (newPhaser != -1)
                                fileStream.ignore(1);
                            if (newTremolo != -1)
                                fileStream.ignore(1);
                            if (newTempo != -1) {
                                tempo = newTempo;
                                quarterNoteLength = 60000 / tempo;
                                //cout << "tempo = " << tempo << endl;
                                fileStream.ignore(1); // transition length
                            }
                            fileStream.ignore(2);
                            if (versionMajor >= 5 && versionMinor > 0)
                            {
                                ignoreGPDataBlocks(fileStream, 2);
                            }
                        }
                        char stringBitmask;
                        fileStream.read(&stringBitmask, 1);
                        //cout << "stringBitmask = " << (int)stringBitmask << endl;
                        for (int stringIndex=0; stringIndex<score->tracks[trackIndex]->stringCount; stringIndex++)
                        {
                            //cout << stringIndex << ": ";
                            if (stringBitmask & (1 << (6 - stringIndex)))
                            {
                                char noteBitmask;
                                fileStream.read(&noteBitmask, 1);
                                //cout << "noteBitmask = " << (int)noteBitmask << ", ";

                                bool tie = false;
                                if (noteBitmask & 32) // note type
                                {
                                    char noteType;
                                    fileStream.read(&noteType, 1);
                                    // 1 = normal, 2 = tie, 3 = dead (muted)
                                    if (noteType == 2)
                                    {
                                        tie = true;
                                    }
                                }
                                if (versionMajor < 5 && noteBitmask & 1)
                                {
                                    fileStream.ignore(2); // unknown
                                }
                                int velocity = 6 * 16;
                                if (noteBitmask & 16) // velocity
                                {
                                    char velocityData;
                                    fileStream.read(&velocityData, 1);
                                    velocity = velocityData * 16;
                                    //cout << "velocity = " << velocity << ", ";
                                }
                                int fretNumber = 0;
                                if (noteBitmask & 32) // fret
                                {
                                    fileStream.read((char*)&fretNumber, 1);
                                }
                                //cout << fretNumber << " ";
                                if (noteBitmask & 128)
                                {
                                    fileStream.ignore(2);
                                }
                                if (versionMajor >= 5 && noteBitmask & 1)
                                {
                                    fileStream.ignore(8);
                                }
                                if (versionMajor >= 5)
                                {
                                    fileStream.ignore(1);
                                }
                                LinkingType linking = LinkingType::none;
                                vector<PitchShiftEvent> stringPitchShift;
                                if (noteBitmask & 8) // effect
                                {
                                    int noteEffectBitmask = 0;
                                    fileStream.read((char*)&noteEffectBitmask, 1);
                                    if (versionMajor >= 4)
                                    {
                                        fileStream.read((char*)&noteEffectBitmask + 1, 1);
                                    }
                                    //cout << "noteEffectBitmask = " << noteEffectBitmask << " ";
                                    // 1 bend present
                                    // 2 hammer on/pull off
                                    // 4 slide 1
                                    // 8 let ring
                                    // 16 grace note
                                    // 256 staccato
                                    // 512 palm mute
                                    // 1024 tremolo picking
                                    // 2048 slide 2
                                    // 4096 harmonic note
                                    // 8192 trill
                                    // 16384 vibrato
                                    if (noteEffectBitmask & 1) // bend
                                    {
                                        readBendChunk(fileStream, stringPitchShift, durationTime);
                                    }
                                    if (noteEffectBitmask & 2) // hammer on/pull off
                                    {
                                        linking = LinkingType::abrupt;
                                    }
                                    if (noteEffectBitmask & 4)
                                    {
                                        linking = LinkingType::slide;
                                    }
                                    if (noteEffectBitmask & 16) // grace note
                                    {
                                        char graceFret;
                                        fileStream.read(&graceFret, 1);
                                        char graceVelocity;
                                        fileStream.read(&graceVelocity, 1);
                                        char transitionType = 0; // 0: none, 1: slide, 2: bend, 3: hammer
                                        if (versionMajor >= 5)
                                        {
                                            fileStream.read(&transitionType, 1);
                                        }
                                        else
                                        {
                                            fileStream.ignore(1); // unknown
                                        }
                                        char graceDuration;
                                        fileStream.read(&graceDuration, 1);
                                        char graceNoteBitmask;
                                        // 0: before the beat, 2: on the beat, 1: dead note before the beat, 3: dead note on the beat
                                        fileStream.read(&graceNoteBitmask, 1);
                                        int graceTime = currentTime;
                                        int graceLength = quarterNoteLength / pow(2.0, graceDuration);
                                        if (!(graceNoteBitmask & 2))
                                        {
                                            graceTime -= graceLength;
                                        }
                                        QPointer<Note> note(new Note());
                                        note->setDuration(graceLength);
                                        note->setTime(graceTime);
                                        note->setFret(graceFret);
                                        note->setString(stringIndex);
                                        note->setPitch(score->tracks[trackIndex]->tuning[stringIndex] + graceFret);
                                        note->setRelativeDuration(graceDuration);
                                        note->setNoteIndex(beatIndex);
                                        note->setGrace(true);
                                        //cout << "pitch = " << note->getPitch() << " ";
                                        score->tracks[trackIndex]->addNote(note);
                                    }
                                    if (noteEffectBitmask & 1024) // tremolo picking
                                    {
                                        fileStream.ignore(1); // TODO
                                    }
                                    if (noteEffectBitmask & 2048) // slide 2 (TODO)
                                    {
                                        fileStream.ignore(1);
                                        linking = LinkingType::slide;
                                    }
                                    if (noteEffectBitmask & 4096) // harmonic note (TODO)
                                    {
                                        char harmonicType;
                                        fileStream.read(&harmonicType, 1);
                                        if (harmonicType == 2) // artificial
                                        {
                                            fileStream.ignore(3);
                                        }
                                        if (harmonicType == 3) // tapped
                                        {
                                            fileStream.ignore(1);
                                        }
                                    }
                                    if (noteEffectBitmask & 8192) // trill (TODO)
                                    {
                                        fileStream.ignore(2);
                                    }
                                }

                                if (!(bitmask & 64))
                                {
                                    QPointer<Note> note(new Note());
                                    int pitch = score->tracks[trackIndex]->tuning[stringIndex] + fretNumber;

                                    note->setDuration(durationTime);
                                    note->setTime(currentTime);
                                    note->setFret(fretNumber);
                                    note->setString(stringIndex);
                                    note->setPitch(pitch);
                                    note->setRelativeDuration(duration);
                                    note->setNoteIndex(beatIndex);
                                    note->setDot(dot);
                                    note->setTuplet(tupletSize);
                                    note->setLinking(linking);

                                    note->setTieDuration(durationTime);
                                    if (tie)
                                    {
                                        Note *previousNote = previousNotes[trackIndex][stringIndex];
                                        Note *tieStart = tieStarts[trackIndex][stringIndex];
                                        if (previousNote != NULL && tieStart != NULL)
                                        {
                                            tieStart->setTieDuration(tieStart->getTieDuration() + note->getDuration());
                                            note->setMuted(true);
                                            note->setPitch(tieStart->getPitch()); // because pitch isn't stored for tied note
                                            previousNote->setLinking(LinkingType::tie);
                                            //cout << note->getPitch() << ": " << tieStart->getTime() << "_" << previousNote->getTime() << "_" << note->getTime() << endl;
                                        }
                                    }
                                    else
                                    {
                                        tieStarts[trackIndex].insert(stringIndex, note);
                                    }
                                    //cout << "pitch = " << note->getPitch() << " ";
                                    for (size_t i=0; i<stringPitchShift.size(); i++)
                                    {
                                        auto time = stringPitchShift[i].time;
                                        auto pitchShift = stringPitchShift[i].pitchShift;
                                        note->addPitchShift(time, pitchShift);
                                    }
                                    previousNotes[trackIndex].insert(stringIndex, note);
                                    score->tracks[trackIndex]->addNote(note);
                                }
                            }
                            //cout << endl;
                        }

                        if (versionMajor >= 5) // transpose notes (TODO)
                        {
                            int noteTransposeBitmask = 0;
                            fileStream.read((char*)&noteTransposeBitmask, 2);
                            if (noteTransposeBitmask & 2048) // extra byte
                            {
                                fileStream.ignore(1);
                            }
                        }

                        currentTime += durationTime;

                        if (fileStream.rdstate() != 0)
                        {
                            fileStream.close();
                            //cout << "error while reading!" << endl;
                            return "error while reading";
                        }
                    }
                }

                if (versionMajor >= 5)
                {
                    fileStream.ignore(1);
                }
            }

            // quarterNoteLength * 4 = length of whole note
            barTime += quarterNoteLength * 4 * barInfo[barIndex].numerator / barInfo[barIndex].denominator;
            quarterNotesCount += 4 * barInfo[barIndex].numerator / barInfo[barIndex].denominator;

            if (barInfo[barIndex].repeatCount > 1)
            {
                int repeatLength = barTime - repeatStart;
                for (int trackIndex=0; trackIndex<trackCount; trackIndex++)
                {
                    QPointer<Track> track = score->tracks[trackIndex];
                    int noteCount = track->notes.size();
                    for (int noteIndex=0; noteIndex<noteCount; ++noteIndex)
                    {
                        QPointer<Note> note = track->notes[noteIndex];
                        if (note->getTime() >= repeatStart)
                        {
                            for (int repetition=0; repetition<barInfo[barIndex].repeatCount; ++repetition)
                            {
                                QPointer<Note> newNote(new Note(*note));
                                newNote->setTime(newNote->getTime() + repeatLength * (repetition + 1));
                                track->addNote(newNote);
                            }
                        }
                    }
                    for (int bar=0; bar<score->bars.size(); ++bar)
                    {
                        if (score->bars[bar] >= repeatStart)
                        {
                            for (int repetition=0; repetition<barInfo[barIndex].repeatCount; ++repetition)
                            {
                                score->bars.push_back(barTime + repeatLength * (repetition + 1));
                            }
                        }
                    }
                }

                barTime += repeatLength * (barInfo[barIndex].repeatCount - 1);
            }
        }

        score->length = barTime;
        score->bpm = 60 * 1000 * quarterNotesCount / barTime;
        // TODO: quarterNotesCount doesn't count repeatitions

        if (score->sections.size() > 0)
            score->sections[score->sections.size() - 1]->length =
                    score->length - score->sections[score->sections.size() - 1]->time;
    }

    delete barInfo;

    //cout << "@" << fileStream.tellg() << "-" << fileStream.rdstate() << endl;

    fileStream.close();

    return "";
}
