#include "ntpcs.h"

AudioEffect* createEffectInstance(audioMasterCallback audio_master)
{
    return new Ntpcs(audio_master);
}

Ntpcs::Ntpcs(audioMasterCallback audio_master)
    : AudioEffectX(audio_master, kNumPrograms, kNumParams)
    , event_count_(0)
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

Ntpcs::~Ntpcs()
{
#if _DEBUG
    LOGD << "close";
#endif
    for (unsigned int i = 0; i < kMaxEvents; ++i)
    {
        free(out_events_->events[i]);
    }
    free(out_events_);
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
                << inEv->midiData[0] << " "
                << inEv->midiData[1] << " "
                << inEv->midiData[2] << " "
                << inEv->midiData[3];
            LOGD << "   deltaFrames: " << inEv->deltaFrames;
#endif
            // Receive NOTE OFF message (accept all channels)
            if (kNoteOff <= int(inEv->midiData[0]) && int(inEv->midiData[0]) <= kNoteOff + 0x0f)
            {
#if _DEBUG
                LOGD << "Received NOTE OFF";
#endif
                if (event_count_ < kMaxEvents)
                {
                    // STOP message
                    VstMidiEvent* evStop = (VstMidiEvent*)out_events_->events[event_count_];
                    ++event_count_;
                    evStop->deltaFrames = inEv->deltaFrames;
                    evStop->noteLength = 0;
                    evStop->midiData[0] = kStop;
                    evStop->midiData[1] = 0;
                    evStop->midiData[2] = 0;
                    evStop->midiData[3] = 0;
                }
            }
            // Received NOTE ON message (accept all channels)
            else if (kNoteOn <= int(inEv->midiData[0]) && int(inEv->midiData[0]) <= kNoteOn + 0x0f)
            {
#if _DEBUG
                LOGD << "Received NOTE ON";
#endif
                if (event_count_ < kMaxEvents - 1)    // set two messages
                {
                    // PROGRAM CHANGE message
                    char channel = inEv->midiData[0] - kNoteOn;
#if _DEBUG
                    LOGD << "Received channel: " << int(channel);
#endif
                    VstMidiEvent* evPrgChg = (VstMidiEvent*)out_events_->events[event_count_];
                    ++event_count_;
                    evPrgChg->deltaFrames = inEv->deltaFrames;
                    evPrgChg->noteLength = 0;
                    evPrgChg->midiData[0] = kProgramChange + channel;
                    evPrgChg->midiData[1] = inEv->midiData[1];        // program number
                    evPrgChg->midiData[2] = 0;
                    evPrgChg->midiData[3] = 0;

                    // START message
                    VstMidiEvent* evStart = (VstMidiEvent*)out_events_->events[event_count_];
                    ++event_count_;
                    evStart->deltaFrames = inEv->deltaFrames;
                    evStart->noteLength = 0;
                    evStart->midiData[0] = kStart;
                    evStart->midiData[1] = 0;
                    evStart->midiData[2] = 0;
                    evStart->midiData[3] = 0;
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

    double tempo = time_info->tempo;
    double sample_rate = time_info->sampleRate;
    double clocks_per_second = tempo * 24.0 / 60.0;
    double samples_per_clock = sample_rate * (1.0 / clocks_per_second);

    // host is playing
    if (time_info->flags & kVstTransportPlaying)
    {
#if _DEBUG
        LOGD << "      ppqPos: " << time_info->ppqPos;
        LOGD << "sampleFrames: " << sample_frames;
#endif

        VstInt32 samples_to_next_clock = time_info->samplesToNextClock;
        double next_clock_sample_frame;

        if (samples_to_next_clock < 0)
        {
            next_clock_sample_frame = samples_per_clock + samples_to_next_clock;
        }
        else
        {
            next_clock_sample_frame = samples_to_next_clock;
        }

        while (next_clock_sample_frame < sample_frames)
        {
            VstMidiEvent* ev = (VstMidiEvent*)out_events_->events[event_count_];
            ++event_count_;
            ev->deltaFrames = (VstInt32)next_clock_sample_frame;
            ev->noteLength = 0;
            ev->midiData[0] = kClock;
            ev->midiData[1] = 0;
            ev->midiData[2] = 0;
            ev->midiData[3] = 0;

            next_clock_sample_frame = next_clock_sample_frame + samples_per_clock;
        }
    }

    // send events
    if (event_count_ > 0)
    {
        out_events_->numEvents = event_count_;
        sendVstEventsToHost(out_events_);
        event_count_ = 0;
    }
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