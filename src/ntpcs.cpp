#include "ntpcs.h"

AudioEffect* createEffectInstance(audioMasterCallback audio_master)
{
    return new Ntpcs(audio_master);
}

Ntpcs::Ntpcs(audioMasterCallback audio_master)
    : AudioEffectX(audio_master, kNumPrograms, kNumParams)
    , last_note_on_(0)
    , polyphony_(0)
    , prev_remainder_sample_frames_(0)
    , sent_first_clock_(false)
{
#if _DEBUG
    plog::init<plog::NtpcsLogFormatter>(plog::debug, "ntpcs.debug.log");
    LOGD << "init";
#endif
    setNumInputs(0);
    setNumOutputs(2);
    setUniqueID(CCONST('n', 't', 'p', 'c'));
    canProcessReplacing(true);
    isSynth(false);

    transmitter = new EventTransmitter(this);
}

Ntpcs::~Ntpcs()
{
#if _DEBUG
    LOGD << "close";
#endif
    delete transmitter;
}

VstInt32 Ntpcs::processEvents(VstEvents* events)
{
    for (int i = 0; i < events->numEvents; ++i)
    {
        if (events->events[i]->type == kVstMidiType)
        {
            VstMidiEvent* inEv = (VstMidiEvent*)(events->events[i]);
#if _DEBUG
            LOGD << "Input MIDI msg: "
                << int(inEv->midiData[0]) << " "
                << int(inEv->midiData[1]) << " "
                << int(inEv->midiData[2]) << " "
                << int(inEv->midiData[3]);
            LOGD << "   deltaFrames: " << inEv->deltaFrames;
#endif
            // Receive NOTE OFF message (accept all channels)
            if (kNoteOff <= int(inEv->midiData[0]) && int(inEv->midiData[0]) <= kNoteOff + 0x0f)
            {
#if _DEBUG
                LOGD << "Received NOTE OFF";
#endif
                if (transmitter->getEventCount() < kMaxEvents)
                {
                    if (polyphony_ == 1)
                    {
                        // STOP message
                        char midi_data_stop[4] = { kStop, 0, 0, 0 };
                        VstMidiEvent* evStop = transmitter->addMidiEvent(inEv->deltaFrames, 0, midi_data_stop);
#if _DEBUG
                        LOGD << "<< SEND MIDI EVENT: STOP >> "
                            << "deltaFrames: " << evStop->deltaFrames;
#endif
                    }

                    if (polyphony_ > 0)
                        --polyphony_;
                }
            }
            // Received NOTE ON message (accept all channels)
            else if (kNoteOn <= int(inEv->midiData[0]) && int(inEv->midiData[0]) <= kNoteOn + 0x0f)
            {
#if _DEBUG
                LOGD << "Received NOTE ON";
#endif
                if (transmitter->getEventCount() < kMaxEvents - 1)    // set two messages
                {
                    // PROGRAM CHANGE message
                    char channel = inEv->midiData[0] - kNoteOn;
#if _DEBUG
                    LOGD << "Received channel: " << int(channel);
#endif
                    char midi_data_prg_chg[4] = { kProgramChange + channel, inEv->midiData[1], 0, 0 };
                    VstMidiEvent* evPrgChg = transmitter->addMidiEvent(inEv->deltaFrames, 0, midi_data_prg_chg);
#if _DEBUG
                    LOGD << "<< SEND MIDI EVENT: PROGRAM CHANGE >> "
                        << "deltaFrames: " << evPrgChg->deltaFrames;
#endif

                    if (polyphony_ == 0)
                    {
                        // START message
                        char midi_data_start[4] = { kStart, 0, 0, 0 };
                        VstMidiEvent* evStart = transmitter->addMidiEvent(inEv->deltaFrames, 0, midi_data_start);
#if _DEBUG
                        LOGD << "<< SEND MIDI EVENT: START >> "
                            << "deltaFrames: " << evStart->deltaFrames;
#endif
                    }

                    last_note_on_ = inEv->midiData[1];
                    ++polyphony_;
                }
            }
        }
    }

    return 1;
}

void Ntpcs::processReplacing(float** inputs, float** outputs, VstInt32 sample_frames)
{
    // dummy
    float *out1 = outputs[0];
    float *out2 = outputs[1];
    for (VstInt32 i = 0; i < sample_frames; ++i)
    {
        (*out1++) = 0.0f;
        (*out2++) = 0.0f;
    }

    VstTimeInfo* time_info = getTimeInfo(
        kVstTransportPlaying |
        kVstTransportCycleActive |
        kVstNanosValid |
        kVstPpqPosValid |
        kVstTempoValid |
        kVstBarsValid |
        kVstCyclePosValid |
        kVstClockValid |
        0
    );

    // host is playing
    if (time_info->flags & kVstTransportPlaying)
    {
#if _DEBUG
        LOGD << "      ppqPos: " << time_info->ppqPos;
        LOGD << "sampleFrames: " << sample_frames;
#endif

        // send timing clock
        double current_sample_cursor = 0.0;
        double tempo = time_info->tempo;
        double sample_rate = time_info->sampleRate;
        double clocks_per_second = tempo * 24.0 / 60.0;
        double samples_per_clock = sample_rate * (1.0 / clocks_per_second);

        if (!sent_first_clock_)
        {
            char midi_data_first[4] = { kClock, 0, 0, 0 };
            VstMidiEvent* ev = transmitter->addMidiEvent(0, 0, midi_data_first);
#if _DEBUG
            LOGD << "<< SEND MIDI EVENT: CLOCK >> "
                << "deltaFrames: " << ev->deltaFrames;
#endif
            sent_first_clock_ = true;
        }

        while (prev_remainder_sample_frames_ + sample_frames - current_sample_cursor > samples_per_clock)
        {
            double delta_frames = 0.0;
            if (prev_remainder_sample_frames_ != 0)
            {
                delta_frames = samples_per_clock - abs(prev_remainder_sample_frames_);
            }
            else
            {
                delta_frames = current_sample_cursor + samples_per_clock;
            }

            char midi_data_clock[4] = { kClock, 0, 0, 0 };
            VstMidiEvent* ev = transmitter->addMidiEvent((VstInt32)delta_frames, 0, midi_data_clock);
#if _DEBUG
            LOGD << "<< SEND MIDI EVENT: CLOCK >> "
                << "deltaFrames: " << ev->deltaFrames;
#endif
            current_sample_cursor = delta_frames;
            prev_remainder_sample_frames_ = 0;
        }
        prev_remainder_sample_frames_ += sample_frames - current_sample_cursor;
#if _DEBUG
        LOGD << "remainderSampleFrames: " << prev_remainder_sample_frames_;
#endif
    }

    // send events
    transmitter->sendEvents();
}

VstInt32 Ntpcs::canDo(char* text)
{
    // plug-in will send Vst events to Host
    if (strcmp(text, "sendVstEvents") == 0)
    {
        return 1;
    }

    // plug-in will send MIDI events to Host
    if (strcmp(text, "sendVstMidiEvent") == 0)
    {
        return 1;
    }

    // plug-in can receive MIDI events from Host
    if (strcmp(text, "receiveVstEvents") == 0)
    {
        return 1;
    }

    // plug-in can receive MIDI events from Host
    if (strcmp(text, "receiveVstMidiEvent") == 0)
    {
        return 1;
    }

    // plug-in can receive Time info from Host
    if (strcmp(text, "receiveVstTimeInfo") == 0)
    {
        return 0;
    }

    // plug-in supports offline functions (offlineNotify, offlinePrepare, offlineRun)
    if (strcmp(text, "offline") == 0)
    {
        return 0;
    }

    // plug-in supports function getMidiProgramName()
    if (strcmp(text, "midiProgramNames") == 0)
    {
        return 0;
    }

    // plug-in supports function setBypass()
    if (strcmp(text, "bypass") == 0)
    {
        return 0;
    }

    return 0;
}

VstInt32 Ntpcs::getNumMidiOutputChannels()
{
    return 2;
}

bool Ntpcs::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text)
{
    vst_strncpy(text, "Default", kVstMaxProgNameLen);
    return true;
}

VstPlugCategory Ntpcs::getPlugCategory()
{
    return kPlugCategUnknown;
}

bool Ntpcs::getEffectName(char* name)
{
    vst_strncpy(name, "Note Clock", kVstMaxEffectNameLen);
    return true;
}

bool Ntpcs::getVendorString(char* text)
{
    vst_strncpy(text, "phocaenoides", kVstMaxVendorStrLen);
    return true;
}

bool Ntpcs::getProductString(char* text)
{
    return getEffectName(text);
}

VstInt32 Ntpcs::getVendorVersion()
{
    return 1001;
}

void Ntpcs::setParameter(VstInt32 index, float value)
{
    // no parameters
}

float Ntpcs::getParameter(VstInt32 index)
{
    // ignore
    return 0.0;
}

void Ntpcs::getParameterLabel(VstInt32 index, char* label)
{
    strcpy(label, "");
}

void Ntpcs::getParameterDisplay(VstInt32 index, char* text)
{
    strcpy(text, "");
}

void Ntpcs::getParameterName(VstInt32 index, char* text)
{
    strcpy(text, "");
}