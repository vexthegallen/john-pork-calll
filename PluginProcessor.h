#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

class JohnPorkCallProcessor : public juce::AudioProcessor
{
public:
    JohnPorkCallProcessor();
    ~JohnPorkCallProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "John Pork Call"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Call controls, used by the editor
    void answer();      // stop ringing, phone-speaker sound
    void decline();     // stop ringing, cut-off sound
    void resetCall();   // hang up and start ringing again

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<bool> ringing { true };
    std::atomic<float> level { 0.0f };
    std::atomic<int> callSamples { 0 };

private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                  juce::dsp::IIR::Coefficients<float>>;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters (int mode);
    void setMode (int mode);
    void loadEmbeddedRingtone();
    void addRingtone (juce::AudioBuffer<float>& buffer);

    Filter highPass, lowPass;
    juce::dsp::Compressor<float> compressor;

    double sr = 44100.0;
    int lastMode = -1;

    // sample-and-hold state for the "decline" bitcrusher
    float held[2] { 0.0f, 0.0f };
    int holdCounter[2] { 0, 0 };

    // ringtone (embedded WAV, with a synthesized ring as a fallback)
    juce::AudioBuffer<float> customRing;
    std::atomic<bool> hasCustomRing { false };
    double customRate = 44100.0;
    double ringPosD = 0.0;
    double phase1 = 0.0, phase2 = 0.0;
    int ringClock = 0;
};
