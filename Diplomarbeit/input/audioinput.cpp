#include "audioinput.h"

#include <math.h>

#include "../music/noteutils.h"
#include <kiss_fft.h>

#include <QDebug>

float interpolate(float *array, float index, int size)
{
    if (index + 1 < 0 || index + 2 >= size)
    {
        return 0;
    }

    // Cubic Hermite spline interpolation
    int i = (int)floorf(index);
    float m0 = (array[i + 1] - array[i - 1]) / 2;
    float m1 = (array[i + 2] - array[i]) / 2;
    float y0 = array[i];
    float y1 = array[i + 1];

    float fract = index - i, fract2 = fract * fract, fract3 = fract2 * fract;
    return
            y0 * (2 * fract3 - 3 * fract2 + 1)+
            m0 * (fract3 - 2 * fract2 + fract) +
            y1 * (-2 * fract3 + 3 * fract2) +
            m1 * (fract3 - fract2);
}

AudioAnalyser::AudioAnalyser(int sampleRate, long timeOffset)
{
    this->sampleRate = sampleRate;
    this->timeOffset = timeOffset;

    fftConfig = kiss_fft_alloc(fftSize, 0, NULL, NULL);
    fftBuffer = new kiss_fft_cpx[fftSize];
    //sampleBuffer = new float[fftSize];

    //maxAmplitudes = new float[eQSize];
    //minAmplitudes = new float[eQSize];


    for (int i=0; i<eQSize; i++)
    {
        maxAmplitudes[i] = 5000;
        minAmplitudes[i] = 30;
    }

    frequencyFactor = (double)fftSize / sampleRate;

    slowConvolution = new float[convolutionSize];
}

AudioAnalyser::~AudioAnalyser()
{
    delete fftBuffer;
    //delete sampleBuffer;
    //delete minAmplitudes;
    //delete maxAmplitudes;
    //kiss_fft_cleanup();
    delete slowConvolution;
}

void AudioAnalyser::processBuffer(long time)
{
    kiss_fft_cpx windowBuffer[fftSize];
    float debugWindowBuffer[fftSize];

    // Apply Hann window
    for (int i=0; i<fftSize; i++)
    {
        float base = sin(3.14159265358979323846 * i / fftSize);
        windowBuffer[i] = (kiss_fft_cpx){sampleBuffer[(i + sampleBufferPosition) % fftSize] * base * base, 0};
        debugWindowBuffer[i] = sampleBuffer[(i + sampleBufferPosition) % fftSize] * base * base;
    }

    // Calculate FFT
    kiss_fft(fftConfig, windowBuffer, fftBuffer);

    //updateEQ();

    float spectrum[fftSize / 2];
    for (int i=0; i<fftSize / 2; i++)
    {
        spectrum[i] = getFrequencyAmplitude(i);
    }

    // look for overtones
    float convolution[convolutionSize];
    for (int i=0; i<convolutionSize; i++)
    {
        float spectrumIndex = (float)i / overtones;
        //float fundamentalAmplitude = spectrum[(int)(spectrumIndex)];
        float sum = 0;
        for (int j=1; j<=overtones; j++)
        {
            // high amplitudes of the overtones => more likely a note
            //sum += spectrum[(int)((float)spectrumIndex * j)] / j;
            sum += interpolate(spectrum, spectrumIndex * j, fftSize / 2) / sqrtf(j);
        }

        //float maxAlternativeFundamentalAmplitude = 0;
        for (int j=2; j<overtones; j++)
        {
            // current frequency might be an overtone of another note
            // substract potential base frequencies
            //sum -= spectrum[(int)((float)spectrumIndex / j)] / j;
            sum -= interpolate(spectrum, spectrumIndex / j, fftSize / 2) / sqrtf(j);
        }
        //sum -= maxAlternativeFundamentalAmplitude;
        //sum = min(sum, fundamentalAmplitude);

        convolution[i] = sum;
    }

    for (int i=0; i<convolutionSize; i++)
    {
        convolution[i] = max(convolution[i], slowConvolution[i] - noteDecay);
        slowConvolution[i] = convolution[i];
    }

    QList<FeaturePoint> featurePoints;
    float bestFeaturePointAmplitude = 0;

    // look for maxima
    for (int i=overtones; i<convolutionSize; i++)
    {
        // ignore amplitudes below the threshold
        if (convolution[i] > 200000)
        {
            // calculate pitch for current index
            float frequency = (float)i / overtones / frequencyFactor;
            float pitch = getPitchOfFrequency(frequency);

            // calculate range so that frequencies closer than noteRadius are ignored
            int startIndex = min((int)roundf(getFrequencyOfPitch(pitch - noteRadius) * overtones * frequencyFactor), i - 1);
            int endIndex = max((int)roundf(getFrequencyOfPitch(pitch + noteRadius) * overtones * frequencyFactor), i + 1);

            // check if this is the maximum in the current range
            bool isPitch = true;
            for (int j=startIndex; j<=endIndex; j++)
            {
                if (convolution[j] > convolution[i])
                {
                    isPitch = false;
                    break;
                }
            }

            if (isPitch)
            {
                featurePoints.push_back((FeaturePoint){pitch, convolution[i]});
                bestFeaturePointAmplitude = max(bestFeaturePointAmplitude, convolution[i]);
            }
        }
    }

    bool noise = false;
    {
        QList<FeaturePoint>::iterator i=featurePoints.begin();
        while (i != featurePoints.end())
        {
            /*
            if (
                    i->amplitude > bestFeaturePointAmplitude * 0.6 &&
                    i->amplitude < bestFeaturePointAmplitude * 0.9)
                noise = true;*/

            if (i->amplitude < bestFeaturePointAmplitude * 0.9)
            {
                i = featurePoints.erase(i);
            }
            else
            {
                i++;
            }
        }
    }

    if (/*noise*/ featurePoints.size() > 6) // too many notes, treat as noise
    {
        //featurePoints.clear();
        //cout << "Noise" << endl;
    }

/*
    for (QList<FeaturePoint>::iterator i=featurePoints.begin(); i!=featurePoints.end();)
    {
        int pitch = (int)roundf(i->pitch);
        int delta = i->amplitude;
        if (previousFeaturePoints.contains(pitch))
        {
            delta -= previousFeaturePoints[pitch].amplitude;
            if (abs(delta) > i->amplitude * 0.2)
            {
                i = featurePoints.erase(i);
            }
            else
            {
                i++;
            }
        }
        else
        {
            i++;
        }
    }*/

    QList<int> decimalPitches;

    for (int i=0; i<featurePoints.size(); i++)
    {
        int pitch = (int)roundf(featurePoints[i].pitch);
        QPointer<Note> note = openNoteEvents[pitch];

        if (note.isNull())
        {
            //cout << time << ": NoteOn " << pitch << endl;
            note = QPointer<Note>(new Note(0, 0, pitch, 128));
            note->setRealTime(time);
            emit noteOn(note.data());
            openNoteEvents.insert(pitch, note);
        }
        decimalPitches.push_back(pitch);
    }

    {
        QMap<int, QPointer<Note>>::iterator i=openNoteEvents.begin();
        while (i != openNoteEvents.end())
        {
            int pitch = i.key();
            if (!decimalPitches.contains(pitch))
            {
                QPointer<Note> note = i.value();
                if (note.isNull())
                {
                    cout << "NULL-Note! something went wrong!" << endl;
                }
                else
                {
                    int duration = time - note->getRealTime();
                    if (duration > 10) // if it's shorter it's probably noise
                    {
                        //cout << time << ": NoteOff " << pitch << endl;
                        emit noteOff(note.data());
                    }
                    else
                    {
                        //cout << time << ": DropNote " << pitch << endl;
                        delete note;
                    }
                }
                i = openNoteEvents.erase(i);
            }
            else
            {
                i++;
            }
        }
    }

    noteThreshold.updateThreshold();

    previousFeaturePoints.clear();
    for (QList<FeaturePoint>::Iterator i=featurePoints.begin(); i!=featurePoints.end(); i++)
    {
        previousFeaturePoints.insert((int)roundf(i->pitch), *i);
    }
    //cout << endl;
}

float AudioAnalyser::getFrequencyAmplitude(int index)
{
    if (index >= fftSize / 2 || index == 0)
        return 0;
    return hypot(fftBuffer[index].r, fftBuffer[index].i);
    //return log(hypot(fftBuffer[index].r, fftBuffer[index].i));// * 3151.615406656636862158;// temp

    int eQIndex = index * eQSize * 2 / fftSize;
    return (hypotf(fftBuffer[index].r, fftBuffer[index].i) - minAmplitudes[eQIndex]) /
            (maxAmplitudes[eQIndex] - minAmplitudes[eQIndex]);
}

bool AudioAnalyser::getFrequencyQuality(int index)
{
    return true; // temp
    int eQIndex = index * eQSize / fftSize;
    return maxAmplitudes[eQIndex] - minAmplitudes[eQIndex] > 1000;
}

void AudioAnalyser::updateEQ()
{
    for (int i=0; i<eQSize; i++) {
        maxAmplitudes[i] -= narrowingSpeed;
        minAmplitudes[i] += narrowingSpeed;
    }
    for (int i=0; i<fftSize / 2; i++) {
        int eQIndex = i * eQSize * 2 / fftSize;
        float amplitude = hypot(fftBuffer[i].r, fftBuffer[i].i);
        maxAmplitudes[eQIndex] = max(amplitude, maxAmplitudes[eQIndex]);
        minAmplitudes[eQIndex] = min(amplitude, minAmplitudes[eQIndex]);
    }
}

qint64 AudioAnalyser::writeData(const char *data, qint64 maxSize)
{
    Q_ASSERT(sampleRate > 0);
    //long clock = timeOffset + sampleCount * 1000 / sampleRate; // could cause problems
    long clock =
        chrono::time_point_cast<chrono::milliseconds>(
            chrono::steady_clock::now()).time_since_epoch().count() -
            (fftSize + maxSize) * 1000 / sampleRate -
            30;

    //cout << "AudioInput" << endl;

    for (qint64 i=0; i<maxSize; i+=2)
    {
        sampleBuffer[sampleBufferPosition] = (float)((short*)data)[i / 2];
        sampleBufferPosition++;

        // TODO: fft positions gonna jitter if fftDistance isn't a divisor of fftSize
        if (sampleBufferPosition % fftDistance == 0)
        {
            processBuffer(clock + i * 1000 / sampleRate);
        }

        if (sampleBufferPosition == fftSize)
        {
            sampleBufferPosition = 0;
        }
    }

    sampleCount += maxSize;
    return maxSize;
}

qint64 AudioAnalyser::readData(char *data, qint64 maxlen)
{
    return 0;
}

AudioInput::AudioInput(QObject *parent, QAudioInput *input, QString name)
{
    setParent(parent);
    audioInput = input;
    this->name = name;
}

AudioInput::~AudioInput()
{
    delete analyser;
    //close();
    delete audioInput;
}

QString AudioInput::getName()
{
    return name;
}

InputType AudioInput::getType()
{
    return AUDIO;
}

bool AudioInput::listen()
{
    cout << "listen..." << endl;

    if(!isListening()) {
        long timeOffset =
                chrono::time_point_cast<chrono::milliseconds>(
                    chrono::steady_clock::now()).time_since_epoch().count();
        cout << "listened at " << timeOffset << endl;
        analyser = new AudioAnalyser(audioInput->format().sampleRate(), timeOffset);
        analyser->open(QIODevice::WriteOnly);
        connect(analyser.data(), &AudioAnalyser::noteOn, this, &Input::noteOn);
        connect(analyser.data(), &AudioAnalyser::noteOff, this, &Input::noteOff);
        audioInput->start(analyser);
    }

    return isListening();
}

void AudioInput::stop()
{
    disconnect(analyser.data(), &AudioAnalyser::noteOn, this, &Input::noteOn);
    disconnect(analyser.data(), &AudioAnalyser::noteOff, this, &Input::noteOff);
    //if(audioInput->thread()->isRunning()) {
    //}
    if(analyser != NULL) {
        audioInput->stop();
        analyser->close();
    }
    delete analyser;
    analyser = NULL;
}

bool AudioInput::isListening()
{
    return analyser != NULL;
}

bool AutoThreshold::is(float value)
{
    int index = max(min((int)(value * resolution / range), resolution - 1), 0);
    samples[index]++;
    sampleCount++;

    //return value > 1.2; // temp
    return value * resolution / range > currentThreshold;
}

float average(int *array, int start, int end)
{
    float sum = 0;
    for (int i=start; i<end; i++)
    {
        sum += array[i];
    }
    return sum / (end - start);
}

float varianz(int *array, int start, int end)
{
    float avg = average(array, start, end);
    float sum = 0;
    for (int i=start; i<end; i++)
    {
        float distance = array[i] - avg;
        sum += distance * distance;
    }
    return sum / (end - start);
}

void AutoThreshold::updateThreshold()
{
    int bestThreshold = resolution * resolution;
    float bestDistance = 0;

    for (int i=1; i<resolution-1; i++)
    {
        float distance = varianz(samples, 0, i) + varianz(samples, i, resolution);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestThreshold = i;
        }
    }

    currentThreshold = bestThreshold;

    //currentThreshold = 8;
}

AutoThreshold::AutoThreshold(float maxValue, int resolution)
{
    range = maxValue;
    this->resolution = resolution;
    samples = new int[resolution];
    for (int i=0; i<resolution; i++)
    {
        samples[i] = 0;
    }
}

AutoThreshold::~AutoThreshold()
{
    delete samples;
}
