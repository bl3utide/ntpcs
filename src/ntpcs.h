#pragma once

#if _DEBUG
#include "logger.h"
#endif
#include <cmath>
#include "audioeffectx.h"
#include "transmitter.h"

#define kNumPrograms 1
#define kNumParams 0

// NOTE: Enable GUI operation in the future
#define kMidiClockTransmitterMode 0

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
    EventTransmitter* transmitter;
    char last_note_on_;         // note number of last pressed key
    unsigned int polyphony_;    // number of notes pressed simultaneously
    double prev_remainder_sample_frames_;   // previous remainder samples
    bool sent_first_clock_;                 // true if this plugin sent first timing clock
};