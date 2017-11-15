#pragma once

#if _DEBUG
#include "logger.h"
#endif
#include <cmath>
#include "audioeffectx.h"

#define kMaxEvents 64
#define kNumPrograms 1
#define kNumParams 0

const char kNoteOff = 0x80;
const char kNoteOn = 0x90;
const char kProgramChange = 0xC0;
const char kClock = 0xF8;
const char kStart = 0xFA;
const char kStop = 0xFC;

class Ntpcs : public AudioEffectX
{
public:
    Ntpcs(audioMasterCallback);
    ~Ntpcs();
    virtual VstInt32 processEvents(VstEvents*);
    virtual void processReplacing(float**, float**, VstInt32);
    virtual VstInt32 canDo(char*);
    virtual VstInt32 getNumMidiOutputChannels();
    bool getProgramNameIndexed(VstInt32, VstInt32, char*);
    virtual VstPlugCategory getPlugCategory();
    virtual bool getEffectName(char*);
    virtual bool getVendorString(char*);
    virtual bool getProductString(char*);
    virtual VstInt32 getVendorVersion();
    virtual void setParameter(VstInt32, float);
    virtual float getParameter(VstInt32);
    virtual void getParameterLabel(VstInt32, char*);
    virtual void getParameterDisplay(VstInt32, char*);
    virtual void getParameterName(VstInt32, char*);

private:
    VstInt32 event_count_;
    VstEvents* out_events_;
};