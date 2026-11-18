#define JUCE_FORCE_LEGACY_PARAMETER_AUTOMATION_TYPE 1

#include "JuceHeader.h"

#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <string.h>

#include <algorithm>
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::paral()
{
    juce::AudioProcessorValueTreeState::ParameterLayout ley;

    ley.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("P0", 1),
        "P0", 0.0f, 1.0f, 1.0f));
    
    ley.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("P1", 1),
        "P1", 0.0f, 1.0f, 0.3f));

    ley.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("P2", 1),
        "P2", 0.0f, 1.0f, 0.9f));

    ley.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("P3", 1),
        "P3", 0.0f, 1.0f, 0.7f));
        
    ley.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("P4", 1),
        "P4", 0.0f, 1.0f, 0.90f));

    ley.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("P5", 1),
        "P5", 0.1f, 0.5f, 0.1f));
    
    return ley;
}
//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
                       STATE (*this, nullptr, "NITRA", paral())
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return "MoldRoom";
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    auto getP = [&](std::atomic<float>*& p,const juce::StringRef n){
        p = STATE.getRawParameterValue(n);
    };

    auto zero = [&](auto&& buf){
        std::fill(std::begin(buf), std::end(buf), 0.0f);
    };

    getP(P0s, "P0");
    getP(P1s, "P1");
    getP(P2s, "P2");
    getP(P3s, "P3");
    getP(P4s, "P4");
    getP(P5s, "P5");

    combindx =0;
   
    for (uint_fast8_t i=0;i<8;++i){
        zero(COMBL[i]);
        zero(COMBR[i]);
    }

    zero(COMBLP);zero(COMPHPB);zero(COMPHPA);
    
}

void AudioPluginAudioProcessor::releaseResources()
{
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    float sample = buffer.getNumSamples();

    auto* chL = buffer.getWritePointer (0);
    auto* chR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : chL;

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, sample);

    float wet = P1s->load();
    float dry = P0s->load();
    float RoomSize = P2s->load()*0.983f;
    float apfFB = 0.5f+P3s->load()*0.5f;
    float Hcut = 1.0f - P4s->load();
    float Lcut = 1.0f - (P5s->load()*0.09f);

    for (int i=0; i<sample; ++i){
        
        float APL = APFF[0][0].process(chL[i],apfFB,apfindex[0][0]);
        float APR = APFF[1][0].process(chR[i],apfFB,apfindex[1][0]);

        for (uint_fast8_t x=0;x<5;++x){
            APL = APFF[0][1+x].process(APL,apfFB,apfindex[0][1+x]);
            APR = APFF[1][1+x].process(APR,apfFB,apfindex[1][1+x]);
        }

        float outC[16];

        for (uint_fast8_t x=0;x<8;++x){
            COMBLP[7-x]=(COMBLP[7-x]*Hcut)+COMBL[x][(uint_fast16_t)(combindx-combindex[0][x])&4095]*(1.0f-Hcut);
            COMBLP[8+x]=(COMBLP[8+x]*Hcut)+COMBR[x][(uint_fast16_t)(combindx-combindex[1][x])&4095]*(1.0f-Hcut);
            float L = (COMBLP[7-x]-COMPHPA[7-x])+COMPHPB[7-x]*Lcut;
            COMPHPA[7-x]=COMBLP[7-x];
            COMPHPB[7-x]=L;
            float R = (COMBLP[8+x]-COMPHPA[8+x])+COMPHPB[8+x]*Lcut;
            COMPHPA[8+x]=COMBLP[8+x];
            COMPHPB[8+x]=R;

            outC[7-x]=L;
            outC[8+x]=R;
        }

        Hadamard16(outC);

        float LoutFDN =0.0f;float RoutFDN =0.0f;
        for (uint_fast8_t x=0;x<8;++x){
            COMBL[x][combindx]=APL*(1.0f-(RoomSize*0.4f))+outC[7-x]*RoomSize;
            LoutFDN+=outC[7-x];
            COMBR[x][combindx]=APR*(1.0f-(RoomSize*0.4f))+outC[8+x]*RoomSize;
            RoutFDN+=outC[8+x];
        }

        chL[i] = (chL[i]*dry)+ (LoutFDN*wet);
        chR[i] = (chR[i]*dry) +(RoutFDN*wet);

        combindx = (combindx + 1) & 4095;
    }

}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}

inline void AudioPluginAudioProcessor::Hadamard16 (float* Hamadard)
{
    for (int i = 0; i < 16; i += 2) {
        float a = Hamadard[i], b = Hamadard[i+1];
        Hamadard[i] = a + b; Hamadard[i+1] = a - b;
    }
    for (int i = 0; i < 16; i += 4) {
        for (int j = 0; j < 2; ++j) {
            float a = Hamadard[i+j], b = Hamadard[i+j+2];
            Hamadard[i+j] = a + b; Hamadard[i+j+2] = a - b;
        }
    }
    for (int i = 0; i < 16; i += 8) {
        for (int j = 0; j < 4; ++j) {
            float a = Hamadard[i+j], b = Hamadard[i+j+4];
            Hamadard[i+j] = a + b; Hamadard[i+j+4] = a - b;
        }
    }
    for (int j = 0; j < 8; ++j) {
        float a = Hamadard[j], b = Hamadard[j+8];
        Hamadard[j] = a + b; Hamadard[j+8] = a - b;
    }
    const float Nor = 1.0f / 4.0f; 
    for (int i = 0; i < 16; ++i) {
        Hamadard[i] *= Nor;
    }
}


