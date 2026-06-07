#include "midioutput.h"


#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

#include "../input/RtMidi.h"

using namespace std;


MidiOutput::MidiOutput()
{

}


vector<string> MidiOutput::getOutputPortList()
{
    vector<string> midiOutPorts;
    RtMidiOut *midiout = 0;

    // RtMidiOut constructor
    try {
        midiout = new RtMidiOut();
    }
    catch ( RtError &error ) {
        error.printMessage();
        //exit( EXIT_FAILURE );
    }

    // Check outputs.
    int nPorts = midiout->getPortCount();

    for ( unsigned int i=0; i<nPorts; i++ ) {
        try {
            midiOutPorts.push_back(midiout->getPortName(i));
        }
        catch (RtError &error) {
            error.printMessage();
        }
    }

    return midiOutPorts;
}



