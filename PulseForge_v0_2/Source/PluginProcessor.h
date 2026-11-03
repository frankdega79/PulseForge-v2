#pragma once
#include <JuceHeader.h>
#include <array>
#include <memory>

class PulseForgeAudioProcessor : public juce::AudioProcessor
{
public:
    PulseForgeAudioProcessor();
    ~PulseForgeAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void setStep (int voice, int step, bool enabled);
    bool getStep (int voice, int step) const;
    void setMutation (float value);
    float getMutation() const { return mutation.load(); }
    void setOutputGain (float value);
    float getOutputGain() const { return outputGain.load(); }
    void setVoiceSampleMode (int voice, bool sampleMode);
    bool getVoiceSampleMode (int voice) const;
    void loadSampleAsync (int voice, const juce::File& file);
    juce::String getSampleName (int voice) const;

private:
    struct SampleData {
        juce::AudioBuffer<float> audio;
        double sampleRate = 44100.0;
        juce::String path;
    };
    struct Voice {
        double phase = 0.0;
        double phase2 = 0.0;
        double samplePosition = 0.0;
        float env = 0.0f;
        float velocity = 0.0f;
        int type = 0;
        bool active = false;
        bool sampleMode = false;
        std::shared_ptr<const SampleData> sample;
    };

    void triggerVoice (int type, float velocity);
    float renderVoice (Voice& v);
    void triggerStep (int step);
    void refreshVoiceSample (int voice);
    std::array<std::array<std::atomic<int>, 16>, 4> pattern;
    std::array<Voice, 32> voices {};
    std::array<std::shared_ptr<const SampleData>, 4> samples {};
    std::array<std::atomic<int>, 4> sampleModes {};
    std::array<juce::String, 4> samplePaths;
    double sr = 44100.0;
    int lastStep = -1;
    std::atomic<float> mutation { 0.25f };
    std::atomic<float> outputGain { 0.85f };
    juce::Random random;
    juce::CriticalSection sampleLock;
    juce::ThreadPool loaderPool { 1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulseForgeAudioProcessor)
};
