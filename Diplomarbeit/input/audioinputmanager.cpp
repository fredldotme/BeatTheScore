#include "audioinputmanager.h"
#include "QList"
#include <QAudioDeviceInfo>

AudioInputManager::AudioInputManager()
{
}

QStringList AudioInputManager::getInputPortList()
{
    QStringList audioInputs;
    #ifndef Q_OS_ANDROID
    QList<QAudioDeviceInfo> inputs = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for ( unsigned int i=0; i<inputs.size(); i++ ) {
        QString name = inputs.at(i).deviceName();
        audioInputs.append(name);
    }
    #endif
    return audioInputs;
}

QList<QPointer<Input>> AudioInputManager::getInputs(QObject *parent)
{
    QAudioFormat format;
    // set up the format you want, eg.
    //format.setSampleRate(192000);
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    QList<QPointer<Input>> audioInputs;
#ifdef Q_OS_WINDOWS
    QList<QAudioDeviceInfo> infos = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (unsigned int i=0; i<infos.size(); i++) {
        if (!infos[i].isFormatSupported(format)) {
            cout << "preffered format not supported try to use nearest" << endl;
            format = infos[i].nearestFormat(format);
        }
        audioInputs.append(new AudioInput(parent, new QAudioInput(infos[i], format), infos[i].deviceName()));
    }
#elif defined(Q_OS_LINUX)
    auto defaultInput = QAudioDeviceInfo::defaultInputDevice();
    if (!defaultInput.isFormatSupported(format)) {
        static bool warnOnce = false;
        if (!warnOnce) {
            cout << "preferred format not supported, try to use nearest" << endl;
            warnOnce = true;
        }
        format = defaultInput.nearestFormat(format);
    }
    audioInputs.append(new AudioInput(parent, new QAudioInput(defaultInput, format), defaultInput.deviceName()));
#endif
    return audioInputs;
}
