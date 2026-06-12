#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <chrono>
#include <cmath>
#ifdef Q_WS_WIN
    #include "Windows.h"
#else
    #include <unistd.h>
#endif
#include <QList>
#include <signal.h>
#include "RtMidi.h"
#include "midiinput.h"
#include "../music/note.h"
#include "input-android/androidglue.h"

#ifdef Q_OS_ANDROID
    #include <libusb.h>
    #include <time.h>
    #include <android/log.h>
#endif

using namespace std;

MidiInput::MidiInput(QObject *parent, int port)
{
    setParent(parent);
    portNumber = port;
    midiin = new RtMidiIn();
    //RtMidiIn *midiin = new RtMidiIn();
    //thread = new MidiInputThread(midiin, port);
    thread = NULL;

    #ifndef Q_OS_ANDROID
    this->name = QString::fromStdString(midiin->getPortName(port));
    #else
    this->name = QString("USB-MIDI");
    #endif
    //connect(thread, &MidiInputThread::noteOn, this, &MidiInput::noteOn);
    //connect(thread, &MidiInputThread::noteOff, this, &MidiInput::noteOff);
}

bool MidiInput::listen()
{
    if (!isListening()) {
        thread = new MidiInputThread(midiin, portNumber);
        connect(thread, &MidiInputThread::noteOn, this, &MidiInput::noteOn, Qt::DirectConnection);
        connect(thread, &MidiInputThread::noteOff, this, &MidiInput::noteOff, Qt::DirectConnection);
        connect(thread, SIGNAL(deviceDisconnected()), this, SLOT(deviceDisconnectedBroker()), Qt::DirectConnection);
        thread->start();
    }

    return isListening();
}

void MidiInput::deviceDisconnectedBroker()
{
    emit this->deviceDisconnected();
}

bool MidiInput::isListening()
{
    return thread != NULL;
}

void MidiInput::stop()
{
    if (isListening()) {
        disconnect(thread, &MidiInputThread::noteOn, this, &MidiInput::noteOn);
        disconnect(thread, &MidiInputThread::noteOff, this, &MidiInput::noteOff);
        disconnect(thread, SIGNAL(deviceDisconnected()), this, SLOT(deviceDisconnectedBroker()));
        delete thread;
        thread = NULL;
    }
}

QString MidiInput::getName()
{
    return name;
}

InputType MidiInput::getType()
{
    return MIDI;
}

void MidiInputThread::run()
{
    setTerminationEnabled(true);
     std::vector<unsigned char> message;
     int nBytes, i;
     double stamp = 0.0;

     // Check if port is available
    #ifndef Q_OS_ANDROID
     unsigned int nPorts = midiin->getPortCount();
    #else
     unsigned int nPorts = 1;
     //cout << "Android code path..." << endl;
    #endif
     if (nPorts > port) {
        #ifndef Q_OS_ANDROID
         midiin->openPort(this->port);

         // Don't ignore sysex, timing, or active sensing messages.
         midiin->ignoreTypes(false, false, false);


         while (isRunning) {
           stamp = midiin->getMessage( &message );
           nBytes = message.size();
        #else
         //unsigned char data[4];
         libusb_device_handle *dev_handle;
         libusb_context *ctx = NULL;
         libusb_device **devs;
         ssize_t cnt;

         libusb_init(&ctx);
         cnt = libusb_get_device_list(ctx, &devs);
         libusb_device *selectedDevice = NULL;
         for(i = 0; i < cnt; i++) {
             libusb_device_descriptor desc;
             int r = libusb_get_device_descriptor(devs[i], &desc);
             if (desc.idVendor == this->androidGlue->getVendorID() &&
                     desc.idProduct == this->androidGlue->getProductID()) {
                 selectedDevice = devs[i];
             }
         }

         libusb_open2(selectedDevice, &dev_handle, this->androidGlue->getDescriptor());
         unsigned char *data = new unsigned char[4];
         libusb_claim_interface(dev_handle, 1);
         double lastTime = 0.0;
         double nowTime;
         struct timeval tval;

         while (isRunning) {
           int ret = libusb_bulk_transfer(dev_handle, this->androidGlue->getEndpoint(), data, 4, &nBytes, 3000);
           if(ret == LIBUSB_ERROR_TIMEOUT || ret == LIBUSB_ERROR_OVERFLOW)
               continue;

           /*if(ret == LIBUSB_ERROR_NO_DEVICE || ret == LIBUSB_ERROR_PIPE || ret == LIBUSB_ERROR_OTHER) {
               isRunning = false;
               emit deviceDisconnected();
           }*/

           gettimeofday(&tval, 0);
           nowTime = ((tval.tv_sec * 1000) + (tval.tv_usec / 1000)) * 0.001;

           if(data[1] != 0xD0){ // ignore on channel pressure
                if(lastTime == 0.0) {
                    stamp = 0.0;
                    lastTime = nowTime;
                } else {
                    stamp = (nowTime - lastTime);
                    lastTime = nowTime;
                }
           }

           message.insert(message.begin(), data + sizeof(data[1]), data + sizeof(data[1]) + 3);
           nBytes = (sizeof(data)/sizeof(*data)) - 1;
        #endif
           if (nBytes == 3)
           {
               //int chan = message[0] & 0xf; // last four bits of message[0]
               dateCounter = dateCounter + stamp * 1000;
               int pitch = message[1];
               int vel = message[2];

               // NoteOn Event
               if (isNoteOn(message))
               {
                   Note *corresponding = getCorrespondingEvent(pitch);
                   if (corresponding != NULL) {
                       //cout << "Found orphaned noteOn event!" << endl;
                       emit noteOff(corresponding);
                       openNoteEvents.erase(remove(openNoteEvents.begin(), openNoteEvents.end(), corresponding), openNoteEvents.end());
                   }
                   QPointer<Note> note = new Note(0, 0, pitch, vel);
                   note->setRealTime(
                               chrono::time_point_cast<chrono::milliseconds>(
                                   chrono::steady_clock::now()).time_since_epoch().count());
                   openNoteEvents.push_back(note);
                   emit noteOn(note);
               }

              // NoteOff Event
               else if (isNoteOff(message))
               {
                   // Set duration in noteStream->noteEvents
                   Note *corresponding = getCorrespondingEvent(pitch);
                   if (corresponding != NULL)
                   {
                       emit noteOff(corresponding);

                       // Delete corresponding from openNoteEvents
                       openNoteEvents.erase(remove(openNoteEvents.begin(), openNoteEvents.end(), corresponding), openNoteEvents.end());
                   }
               }

              // PitchWheel Event
               else if (isPitchWheel(message[0]))
               {
                   const int pitchCenter = 0x2000;
                   // as recommended by MIDI Standard, might be different for some devices
                   int halfStepShift = (0x3FFF - 0x2000) / 2;

                   int mostSignificant = message[2] & 0x7f;
                   int leastSignificant = message[1] & 0x7f;
                   int pitchVal = mostSignificant * std::pow(2,7) + leastSignificant;
                   int absolutePitchShift = pitchVal - pitchCenter;
                   float centShift = ((float) absolutePitchShift / halfStepShift) * 100.0;
                   applyPitchShiftToOpenEvents((int) centShift);
               }

           /*for ( i=0; i<nBytes; i++ ) {
               std::cout << "Byte " << i << " = " << (int)message[i] << ", ";
           }*/
           /*if ( nBytes > 0 ) {
             std::cout << "stamp = " << stamp << std::endl;
           }*/
         }
         message.clear();
         }
     }

}


bool MidiInputThread::isNoteOn(std::vector<unsigned char> message)
{
    int mask = 0xf0;
    if ((message[0] & mask) == 0x90 && message[2] > 0)
    {
        return true;
    }
    return false;
}


bool MidiInputThread::isNoteOff(std::vector<unsigned char> message)
{
    int mask = 0xf0;
    if ((message[0] & mask) == 0x80 || ((message[0] & mask) == 0x90 && message[2] == 0))
    {
        return true;
    }
    return false;
}


bool MidiInputThread::isPitchWheel(int statusMessage)
{
    int mask = 0xf0;
    return (statusMessage & mask) == 0xE0;
}

Note *MidiInputThread::getCorrespondingEvent(int pitch)
{
    for (unsigned int i = 0; i < openNoteEvents.size(); i++)
    {
        Note *event = openNoteEvents.at(i);
        if (event->getPitch() == pitch)
        {
            return event;
        }
    }
    return NULL;
}

void MidiInputThread::applyPitchShiftToOpenEvents(int pitchShift)
{
    for (unsigned int i = 0; i < openNoteEvents.size(); i++)
    {
        QPointer<Note> event = openNoteEvents.at(i);
        // TODO: quantisize values
        //event->pitchShift.append(pitchShift);
    }
    //cout << "";
}

MidiInput::~MidiInput()
{
    stop();
}

MidiInputThread::MidiInputThread(RtMidiIn *midiin, int port)
{
    this->midiin = midiin;
    this->port = port;
    dateCounter = 0;
}

MidiInputThread::~MidiInputThread()
{
    isRunning = false;
    wait(50);
    terminate();
    wait();
    #ifndef Q_OS_ANDROID
    midiin->closePort();
    #endif
    /*if (midiin != NULL)
    {
        midiin->closePort();
        delete midiin;
        midiin = NULL;
        #ifdef Q_OS_ANDROID
        //libusb_free_device_list(devs, 1);
        #endif
    }
    delete midiin;*/
}
