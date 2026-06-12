#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include <iostream>
#include <memory>
#include <string>
#include <chrono>

#include <QThread>
#include <QAudioInput>
#include <QAudioDecoder>

#include "kiss_fft.h"

class AudioInput;

#include "input.h"

using namespace std;

#define AUDIOINPUT_THRESHOLD 0.1

struct FeaturePoint {
    float pitch;
    float amplitude;
};

class AutoThreshold
{
private:
    float range;
    int resolution;
    int sampleCount;
    int *samples;
    int currentThreshold;

public:
    bool is(float value);
    void updateThreshold();

    AutoThreshold(float maxValue, int resolution);
    ~AutoThreshold();
};

class AudioAnalyser : public QIODevice
{
    Q_OBJECT

public:
    AudioAnalyser(int sampleRate, long timeOffset);
    ~AudioAnalyser();

    const static int fftSize = 4096;
    const static int convolutionSize = 800;
    const static int eQSize = 128;
    const static int overtones = 8; // numbers of overtones to look for
    const static int noteRadius = 2;
    const static int fftDistance = 44100 * 20 / 1000; // 20 ms accuracy // TODO: calculate from samplerate
    const static int minPitch = 23, maxPitch = 72;

    long timeOffset;
    QMap<int, QPointer<Note>> openNoteEvents;
    QMap<int, FeaturePoint> previousFeaturePoints;
    kiss_fft_cfg fftConfig;
    float sampleBuffer[fftSize];
    kiss_fft_cpx *fftBuffer = NULL;

    // for smoother note detection
    float *slowConvolution = NULL;
    float noteDecay = 200000;

    // automatic EQ
    float minAmplitudes[eQSize] = {0};
    float maxAmplitudes[eQSize] = {0};

    // for automatic callibration
    float narrowingSpeed = 0; // turned off for now...

    AutoThreshold noteThreshold = AutoThreshold((float)20, 256); // 32768 = 2^15 = max amplitude

    int sampleBufferPosition = 0;
    int fftBufferPosition = 0;
    qint64 sampleCount = 0;

    int sampleRate;
    double frequencyFactor;

    void processBuffer(long time);
    float getFrequencyAmplitude(int index);
    bool getFrequencyQuality(int index);
    void updateEQ();

signals:
    void noteOn(Note *note);
    void noteOff(Note *note);

    // QIODevice interface
public:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
};

class AudioInput : public Input
{
    Q_OBJECT

private:
    QPointer<AudioAnalyser> analyser;
    QPointer<QAudioInput> audioInput;
    QString name;
    chrono::time_point<chrono::steady_clock, chrono::milliseconds> startTime;

public:
    AudioInput(QObject *parent, QAudioInput *input, QString name);
    ~AudioInput();

public:
    QAudioDecoder *decoder;

    // Input interface
public:
    bool listen();
    void stop();
    bool isListening();
    QString getName();
    InputType getType();
};

#endif // AUDIOINPUT_H
