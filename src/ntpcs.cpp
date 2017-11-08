#include "ntpcs.h"

AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
    return new Ntpcs(audioMaster);
}

Ntpcs::Ntpcs(audioMasterCallback audioMaster)
    : AudioEffectX(audioMaster, NUM_PROGRAMS, NUM_PARAMS)
    , eventCount(0)
{
#if _DEBUG
    debug.open("./ntpcs.log", std::ios::out | std::ios::app);
    debug << "Loading\n";
    debug.flush();
#endif
    setNumInputs(0);
    setNumOutputs(2);
    setUniqueID(CCONST('n', 't', 'p', 'c'));
    canProcessReplacing(true);
    isSynth(false);

    // init events
    size_t size = sizeof(VstEvents) + MAX_EVENTS * sizeof(VstEvent*);
    outEvents = (VstEvents*)malloc(size);
    for (unsigned int i = 0; i < MAX_EVENTS; ++i)
    {
        outEvents->events[i] = (VstEvent*)calloc(1, sizeof(VstMidiEvent));
        VstEvent* ev = outEvents->events[i];
        ev->type = kVstMidiType;
        ev->byteSize = sizeof(VstMidiEvent);
    }
}

Ntpcs::~Ntpcs()
{
#if _DEBUG
    debug.close();
#endif
    for (unsigned int i = 0; i < MAX_EVENTS; ++i)
    {
        free(outEvents->events[i]);
    }
    free(outEvents);
}

VstInt32 Ntpcs::processEvents(VstEvents* events)
{
    for (int i = 0; i < events->numEvents; ++i)
    {
        if (events->events[i]->type == kVstMidiType)
        {
            VstMidiEvent* inEv = (VstMidiEvent*)(events->events[i]);
#if _DEBUG
            debug << "input MIDI msg: ";
            debug << int(inEv->midiData[0]);
            debug << " ";
            debug << int(inEv->midiData[1]);
            debug << " ";
            debug << int(inEv->midiData[2]);
            debug << " ";
            debug << int(inEv->midiData[3]);
            debug << " ";
            debug << "\n";
            debug.flush();
#endif
            // Receive NOTE OFF message (accept all channels)
            if (NOTE_OFF <= int(inEv->midiData[0]) && int(inEv->midiData[0]) <= NOTE_OFF + 0x0f)
            {
#if _DEBUG
                debug << "Received NOTE OFF";
                debug << "\n";
                debug.flush();
#endif
                if (eventCount < MAX_EVENTS)
                {
                    // STOP message
                    VstMidiEvent* evStop = (VstMidiEvent*)outEvents->events[eventCount];
                    ++eventCount;
                    evStop->deltaFrames = inEv->deltaFrames;
                    evStop->noteLength = 0;
                    evStop->midiData[0] = STOP;
                    evStop->midiData[1] = 0;
                    evStop->midiData[2] = 0;
                    evStop->midiData[3] = 0;
                }

                /*
                // Up one octave (if possible)
                if (_midiEvent->midiData[0] <= 0x73)
                {
                    midiEvent.midiData[1] = _midiEvent->midiData[1] + 12;
                }
                else
                {
                    midiEvent.midiData[1] = _midiEvent->midiData[1];
                }
                */
            }
            // Received NOTE ON message (accept all channels)
            else if (NOTE_ON <= int(inEv->midiData[0]) && int(inEv->midiData[0]) <= NOTE_ON + 0x0f)
            {
#if _DEBUG
                debug << "Received NOTE ON";
                debug << "\n";
                debug.flush();
#endif
                if (eventCount < MAX_EVENTS - 1)    // set two messages
                {
                    // PROGRAM CHANGE message
                    char channel = inEv->midiData[0] - NOTE_OFF;
                    VstMidiEvent* evPrgChg = (VstMidiEvent*)outEvents->events[eventCount];
                    ++eventCount;
                    evPrgChg->deltaFrames = inEv->deltaFrames;
                    evPrgChg->noteLength = 0;
                    evPrgChg->midiData[0] = PROGRAM_CHANGE + channel;
                    evPrgChg->midiData[1] = inEv->midiData[1];        // program number
                    evPrgChg->midiData[2] = 0;
                    evPrgChg->midiData[3] = 0;

                    // START message
                    VstMidiEvent* evStart = (VstMidiEvent*)outEvents->events[eventCount];
                    ++eventCount;
                    evStart->deltaFrames = inEv->deltaFrames;
                    evStart->noteLength = 0;
                    evStart->midiData[0] = START;
                    evStart->midiData[1] = 0;
                    evStart->midiData[2] = 0;
                    evStart->midiData[3] = 0;

                    /*
                    VstMidiEvent* ev = (VstMidiEvent*)outEvents->events[eventCount];
                    ++eventCount;
                    ev->deltaFrames = inEv->deltaFrames;
                    ev->noteLength = 0;
                    ev->midiData[0] = inEv->midiData[0];
                    ev->midiData[1] = inEv->midiData[1];
                    ev->midiData[2] = inEv->midiData[2];
                    ev->midiData[3] = inEv->midiData[3];
                    */
                }
            }
        }
    }

    return 1;
}

void Ntpcs::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
    // dummy
    float *out1 = outputs[0];
    float *out2 = outputs[1];
    for (VstInt32 i = 0; i < sampleFrames; ++i)
    {
        (*out1++) = 0.0f;
        (*out2++) = 0.0f;
    }

    // send events
    if (eventCount > 0)
    {
        outEvents->numEvents = eventCount;
        sendVstEventsToHost(outEvents);
        eventCount = 0;
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