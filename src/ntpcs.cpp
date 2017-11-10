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
    logger_ = new Logger("./ntpcs_run.log");
    logger_->addOneLine("Loading");
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
    logger_->addOneLine("Closed");
    delete logger_;
#endif
    for (unsigned int i = 0; i < kMaxEvents; ++i)
    {
        free(out_events_->events[i]);
    }
    free(out_events_);
}

VstInt32 Ntpcs::processEvents(VstEvents* events)
{
#if _DEBUG
    logger_->addOneLine("-- START processEvents --");
#endif
    for (int i = 0; i < events->numEvents; ++i)
    {
        if (events->events[i]->type == kVstMidiType)
        {
            VstMidiEvent* inEv = (VstMidiEvent*)(events->events[i]);
#if _DEBUG
            logger_->addFrontDate();
            logger_->addMessage("Input MIDI msg:");
            logger_->addMessage(kLogAlignRight, 5, int(inEv->midiData[0]));
            logger_->addMessage(kLogAlignRight, 5, int(inEv->midiData[1]));
            logger_->addMessage(kLogAlignRight, 5, int(inEv->midiData[2]));
            logger_->addMessage(kLogAlignRight, 5, int(inEv->midiData[3]));
            logger_->addEnd();
#endif
            // Receive NOTE OFF message (accept all channels)
            if (kNoteOff <= int(inEv->midiData[0]) && int(inEv->midiData[0]) <= kNoteOff + 0x0f)
            {
#if _DEBUG
                logger_->addOneLine("Received NOTE OFF");
#endif
                if (event_count_ < kMaxEvents)
                {
                    /*
                    // STOP message
                    VstMidiEvent* evStop = (VstMidiEvent*)outEvents->events[eventCount];
                    ++eventCount;
                    evStop->deltaFrames = inEv->deltaFrames;
                    evStop->noteLength = 0;
                    evStop->midiData[0] = STOP;
                    evStop->midiData[1] = 0;
                    evStop->midiData[2] = 0;
                    evStop->midiData[3] = 0;
                    */
                }
            }
            // Received NOTE ON message (accept all channels)
            else if (kNoteOn <= int(inEv->midiData[0]) && int(inEv->midiData[0]) <= kNoteOn + 0x0f)
            {
#if _DEBUG
                logger_->addOneLine("Received NOTE ON");
#endif
                if (event_count_ < kMaxEvents - 1)    // set two messages
                {
                    // PROGRAM CHANGE message
                    char channel = inEv->midiData[0] - kNoteOn;
#if _DEBUG
                    logger_->addFrontDate();
                    logger_->addMessage(kLogAlignLeft, 0, "Received channel: ");
                    logger_->addMessage(kLogAlignLeft, 0, int(channel));
                    logger_->addEnd();
#endif
                    VstMidiEvent* evPrgChg = (VstMidiEvent*)out_events_->events[event_count_];
                    ++event_count_;
                    evPrgChg->deltaFrames = inEv->deltaFrames;
                    evPrgChg->noteLength = 0;
                    evPrgChg->midiData[0] = kProgramChange + channel;
                    evPrgChg->midiData[1] = inEv->midiData[1];        // program number
                    evPrgChg->midiData[2] = 0;
                    evPrgChg->midiData[3] = 0;

                    /*
                    // START message
                    VstMidiEvent* evStart = (VstMidiEvent*)outEvents->events[eventCount];
                    ++eventCount;
                    evStart->deltaFrames = inEv->deltaFrames;
                    evStart->noteLength = 0;
                    evStart->midiData[0] = START;
                    evStart->midiData[1] = 0;
                    evStart->midiData[2] = 0;
                    evStart->midiData[3] = 0;
                    */
                }
            }
        }
    }

#if _DEBUG
    logger_->addOneLine("-- END processEvents --");
#endif
    return 1;
}

void Ntpcs::processReplacing(float** inputs, float** outputs, VstInt32 sample_frames)
{
#if _DEBUG
    logger_->addOneLine("-- START processReplacing --");

    logger_->addMessageSpace(kLogAlignRight, 20, "sample frames: ");
    logger_->addEnd();
#endif

    // dummy
    float *out1 = outputs[0];
    float *out2 = outputs[1];
    for (VstInt32 i = 0; i < sample_frames; ++i)
    {
        (*out1++) = 0.0f;
        (*out2++) = 0.0f;
    }

#if _DEBUG
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

    if (time_info->flags & kVstTransportPlaying)
    {
        logger_->addFrontDate();
        logger_->addMessage(kLogAlignLeft, 0, "PpqPos: ");
        logger_->addMessage(kLogAlignLeft, 0, time_info->ppqPos);
        if (time_info->samplesToNextClock == 0)
            logger_->addMessage(kLogAlignLeft, 0, " *");
        logger_->addEnd();
    }
#endif

    // send events
    if (event_count_ > 0)
    {
        out_events_->numEvents = event_count_;
        sendVstEventsToHost(out_events_);
        event_count_ = 0;
    }

#if _DEBUG
    logger_->addOneLine("-- END processReplacing --");
#endif
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