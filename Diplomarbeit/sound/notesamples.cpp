#include "notesamples.h"
#include <QFile>
#include <QDir>
#include <sstream>
#include <iostream>

#include "SoundTouch.h"

using namespace soundtouch;

NoteSamples::NoteSamples(MusicInstrument instrument)
{
    this->instrument = instrument;
}

void NoteSamples::startPitching()
{
    SoundTouch st;
    st.setChannels(1);
    st.setSampleRate(44100);
    //st.setSetting(SETTING_OVERLAP_MS, 1);
    //st.setSetting(SETTING_SEQUENCE_MS, 5);
    //#ifdef Q_OS_ANDROID
    st.setSetting(SETTING_USE_QUICKSEEK, 1);
    st.setSetting(SETTING_USE_AA_FILTER, 0);
    //#endif

    //st.setTempoChange(0);

    //Pre-initialize
    for(int i = 0; i < 128; i++) {
        NoteSample *newSample = new NoteSample();
        notes.push_back(newSample);
    }

    //Pull in buffers from existing files
    QString dirPath;

#ifdef Q_OS_LINUX
    // Find .wav's in Snap package
    if (getenv("SNAP")) {
        dirPath = QString::fromUtf8(qgetenv("SNAP")) + QStringLiteral("/opt/BeatTheScore/");
    }
#endif

    if (instrument == MusicInstrument::Guitar) {
        dirPath += "wav/guitar/clean/";
    } else if (instrument == MusicInstrument::EGuitar) {
        dirPath += "wav/guitar/electric/";
    } else if (instrument == MusicInstrument::BassGuitar) {
        dirPath += "wav/guitar/bass/";
    } else if (instrument == MusicInstrument::Piano) {
        dirPath += "wav/piano/";
    }

#ifdef Q_OS_ANDROID
    dirPath = "assets:/" + dirPath;
#endif

    QStringList fileFilter;

    fileFilter << "_*.wav";
    QDir wavDir(dirPath);
    wavDir.setNameFilters(fileFilter);

    QStringList files = wavDir.entryList();
    for(int i = 0; i < files.size(); i++) {
        QString filename = files.at(i);

        QFile f(dirPath + filename);
        if(f.exists()) {
            f.open(QIODevice::ReadOnly);
            f.seek(44);
            QByteArray barr = f.readAll();
            f.close();

            filename = filename.remove(0,1);
            filename.chop(4); //remove file ending

            bool parseOk = false;
            const int index = filename.toInt(&parseOk);
            if (!parseOk)
                continue;

            char * tmpBuf = barr.data();
            notes[index]->buffer = new char[barr.size() - 64];
            memcpy(notes[index]->buffer, tmpBuf, barr.size() - 64);
            notes[index]->bufferSize = barr.size() - 64;
        }
    }

    //Find last buffer
    int lastBuffer = 0;
    for(int i=0; i < 128; i++) {
        if(notes[i]->bufferSize != 0) {
            lastBuffer = i;
        }
    }

    //Pitch related notes for non-existing buffers
    for(int i = 0; i < 128; i++) {
        if(notes[i]->bufferSize == 0) {
            bool foundReference = false;
            if(i < lastBuffer) { //probably will be smarter in the future
                for(int j = i+1; j < 128 && !foundReference; j++) {
                    if (notes[j]->bufferSize > 0) {
                        st.setPitchSemiTones(i-j);
                        //std::cout << "pitch " << (i-j) << " semi tones" << std::endl;

                        st.clear();
                        st.putSamples((SAMPLETYPE*)notes[j]->buffer, notes[j]->bufferSize / 2);
                        st.flush();
                        int numSamples = st.numSamples();
                        notes[i]->bufferSize = numSamples * 2; // numSamples = number of 2 byte samples
                        notes[i]->buffer = new char[notes[i]->bufferSize];
                        st.receiveSamples((SAMPLETYPE*)notes[i]->buffer, numSamples);
                        st.flush();

                        foundReference = true;
                    }
                }
            } else {
                st.setPitchSemiTones(i-lastBuffer);
                //std::cout << "pitch " << (i-lastBuffer) << " semi tones" << std::endl;

                st.clear();
                st.putSamples((SAMPLETYPE*)notes[lastBuffer]->buffer, notes[lastBuffer]->bufferSize / 2);
                st.flush();
                int numSamples = st.numSamples();
                notes[i]->bufferSize = numSamples * 2;
                notes[i]->buffer = new char[notes[lastBuffer]->bufferSize];
                st.receiveSamples((SAMPLETYPE*)notes[i]->buffer, numSamples);
                st.flush();
            }
            //lastBuffer++;
        }

        emit pitchChanged();
    }
}

NoteSample* NoteSamples::getNote(int pitch)
{
    if (pitch < 128 && pitch >= 0)
    {
        return notes[pitch];
    }
}

MusicInstrument NoteSamples::getInstrumentFromGM(int gMInstrument)
{
    if (gMInstrument >= 1 && gMInstrument <= 8) {
        return MusicInstrument::Piano;
    } else if (gMInstrument >= 25 && gMInstrument <= 26) {
        return MusicInstrument::Guitar;
    } else if (gMInstrument >= 27 && gMInstrument <= 32) {
        return MusicInstrument::EGuitar;
    } else if (gMInstrument >= 33 && gMInstrument <= 40) {
        return MusicInstrument::BassGuitar;
    }

    return MusicInstrument::Piano;
}

MusicInstrument NoteSamples::getInstrumentType()
{
    return instrument;
}
