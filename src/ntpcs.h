#pragma once

#include <fstream>
#include "audioeffectx.h"

#define MAX_EVENTS 64

#define NUM_PROGRAMS 1
#define NUM_PARAMS 0

enum MIDI_MSG
{
    NOTE_OFF        = 0x80,
    NOTE_ON         = 0x90,
    PROGRAM_CHANGE  = 0xc0
};

class Ntpcs : public AudioEffectX
{
public:
    Ntpcs(audioMasterCallback audioMaster);
    ~Ntpcs();
    virtual VstInt32 processEvents(VstEvents* events);
    virtual void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames);
    virtual VstInt32 canDo(char* text);
    virtual VstInt32 getNumMidiOutputChannels();
    bool getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text);
    virtual VstPlugCategory getPlugCategory();
    virtual bool getEffectName(char* name);
    virtual bool getVendorString(char* text);
    virtual bool getProductString(char* text);
    virtual VstInt32 getVendorVersion();
    virtual void setParameter(VstInt32 index, float value);
    virtual float getParameter(VstInt32 index);
    virtual void getParameterLabel(VstInt32 index, char* label);
    virtual void getParameterDisplay(VstInt32 index, char* text);
    virtual void getParameterName(VstInt32 index, char* text);

private:
#if _DEBUG
    std::ofstream debug;
#endif

    VstInt32 eventCount;
    VstEvents* outEvents;
};