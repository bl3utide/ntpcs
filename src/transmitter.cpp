#include "transmitter.h"

EventTransmitter::EventTransmitter(AudioEffectX* plugin)
    : plugin_(plugin)
    , event_count_(0)
{
    // init events
    size_t size = sizeof(VstEvents) + kMaxEvents * sizeof(VstEvent*);
    out_events_ = (VstEvents*)malloc(size);
    for (unsigned int i = 0; i < kMaxEvents; ++i)
    {
        out_events_->events[i] = (VstEvent*)calloc(1, sizeof(VstMidiEvent));
        VstEvent* ev = out_events_->events[i];
        ev->type = kVstMidiType;
        ev->byteSize = sizeof(VstMidiEvent);
    }
}

EventTransmitter::~EventTransmitter()
{
    for (unsigned int i = 0; i < kMaxEvents; ++i)
    {
        free(out_events_->events[i]);
    }
    free(out_events_);
}

VstInt32 EventTransmitter::getEventCount()
{
    return event_count_;
}

VstMidiEvent* EventTransmitter::addMidiEvent(VstInt32 delta_frames, VstInt32 note_length, char midi_data[4])
{
    VstMidiEvent* ev = (VstMidiEvent*)out_events_->events[event_count_];
    ++event_count_;
    ev->deltaFrames = delta_frames;
    ev->noteLength = note_length;
    ev->midiData[0] = midi_data[0];
    ev->midiData[1] = midi_data[1];
    ev->midiData[2] = midi_data[2];
    ev->midiData[3] = midi_data[3];

    return ev;
}

void EventTransmitter::sendEvents()
{
    if (event_count_ > 0)
    {
        out_events_->numEvents = event_count_;
        plugin_->sendVstEventsToHost(out_events_);
        event_count_ = 0;
    }
}