#pragma once

#include <cmath>
#include "audioeffectx.h"

#define kMaxEvents 64

const char kNoteOff = 0x80;
const char kNoteOn = 0x90;
const char kProgramChange = 0xC0;
const char kClock = 0xF8;
const char kStart = 0xFA;
const char kStop = 0xFC;

class EventTransmitter
{
public:
    EventTransmitter(AudioEffectX*);
    ~EventTransmitter();
    VstInt32 getEventCount();
    VstMidiEvent* addMidiEvent(VstInt32, VstInt32, char[4]);
    void sendEvents();

private:
    AudioEffectX* plugin_;
    VstInt32 event_count_;
    VstEvents* out_events_;
};