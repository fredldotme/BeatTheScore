#include <string>
#include <vector>

#include <QList>
#include <QVector>
#include <QStringList>
#include <QString>
#include "RtMidi.h"

#include "midiinputmanager.h"

using namespace std;


MidiInputManager::MidiInputManager()
{
}


/*QStringList MidiInputManager::getInputPortList()
{
    QStringList midiInPorts;
    RtMidiIn  *midiin = 0;

    // RtMidiIn constructor
    try {
        midiin = new RtMidiIn();
    }
    catch ( RtError &error ) {
        error.printMessage();
    }

    // Check inputs.
    unsigned int nPorts = midiin->getPortCount();
    for ( unsigned int i=0; i<nPorts; i++ ) {
        try {
            QString portName = QString::fromStdString(midiin->getPortName(i));
            if(!portName.startsWith("Midi Through")) {
                midiInPorts.append(portName);
            }
        }
        catch ( RtError &error ) {
            error.printMessage();
        }
    }

    delete midiin;
    return midiInPorts;
}*/

QList<QPointer<Input>> MidiInputManager::getInputs(QObject *parent)
{
#if 1
    RtMidiIn  *midiin = 0;

    // RtMidiIn constructor
    try {
        midiin = new RtMidiIn();
    }
    catch ( RtError &error ) {
        error.printMessage();
        return QList<QPointer<Input>>();
    }

    // Check inputs.
    unsigned int nPorts = midiin->getPortCount();
    for ( unsigned int i=0; i<nPorts; i++ ) {
        QString portName = QString::fromStdString(midiin->getPortName(i));

        bool nameClaimed = false;
        for(int j = 0; j < midiInputs.count(); j++) {
            if(!midiInputs[j].isNull() && midiInputs[j]->getName() == portName) {
                nameClaimed = true;
            }
        }
        if(!portName.startsWith("Midi Through") && !nameClaimed) {
            midiInputs.push_back(new MidiInput(parent, i));
        }
    }

    for(int i = midiInputs.size()-1; i >= 0; i--) {
        bool removeFromList = true;
        for(unsigned int j = 0; j < nPorts; j++) {
            QString portName = QString::fromStdString(midiin->getPortName(j));

            if(midiInputs[i]->getName() == portName) {
                removeFromList = false;
            }
        }

        if(removeFromList) {
            midiInputs[i]->stop();
            delete midiInputs[i].data();
            midiInputs.removeAt(i);
        }
    }

    delete midiin;
    return midiInputs;
#else
    return midiInputs;
#endif
}
