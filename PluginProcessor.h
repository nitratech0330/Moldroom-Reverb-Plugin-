#define JUCE_FORCE_LEGACY_PARAMETER_AUTOMATION_TYPE 1

#pragma once

#include "JuceHeader.h"
#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
class APF{
    public:
    APF(){
        std::fill(std::begin(b),std::end(b),0.0f);
        std::fill(std::begin(b2),std::end(b2),0.0f);
    };
    inline float process(float in, float c, int x){
        uint_fast16_t N = (n-x)&1023;
        b2[n] = (in*-c)+b[N]+(b2[N]*c);
        b[n]=in;
        return b2[n];
        n = (n+1)&1023;
    };
    private:
    float b[1024];
    float b2[1024];
    uint_fast16_t n =0;
};
class AudioPluginAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    inline void Hadamard16 (float* Hadamard);

    uint_fast16_t combindx =0;

    APF APFF[2][6];

    float COMBL[8][4096];
    float COMBR[8][4096];

    float COMBLP[16];
    float COMPHPA[16];
    float COMPHPB[16];

    static inline constexpr int combindex[2][8]{
		{1109, 1229, 1321, 1487, 1559, 2333, 1871, 2593},
		{1117, 1217, 1531, 1621, 2221, 1699, 2467, 2017}
	};

	static inline constexpr int apfindex[2][6]{
		{233, 331, 457, 521, 673, 773},
		{241, 349, 463, 541, 661, 797}
	};

    static inline const int ReverbLFO[]{0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, -1, -2, -3, -4, -5, -6, -7, -8, -8, -7, -6, -5, -4, -3, -2, -1, 0};
    
    std::atomic<float>* P0s = nullptr;
    std::atomic<float>* P1s = nullptr;
    std::atomic<float>* P2s = nullptr;
    std::atomic<float>* P3s = nullptr;
    std::atomic<float>* P4s = nullptr;
    std::atomic<float>* P5s = nullptr;
    
    juce::AudioProcessorValueTreeState STATE;
    juce::AudioProcessorValueTreeState::ParameterLayout paral();

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
   
};
