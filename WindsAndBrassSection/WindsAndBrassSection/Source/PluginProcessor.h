#pragma once

#include "Engine.h"

class WbsProcessor : public juce::AudioProcessor
{
public:
    WbsProcessor();
    ~WbsProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Winds and Brass Section"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

    // Para la interfaz
    std::atomic<int> shownArtic { 0 };
    std::array<std::atomic<float>, wbs::kNumPlayers> activity;

private:
    wbs::Section::Settings readSettings() const;

    wbs::Section section;
    juce::Reverb reverb;

    std::array<std::atomic<float>*, wbs::kNumPlayers> onP {}, lvlP {};
    std::atomic<float> *modeP = nullptr, *articP = nullptr, *rootP = nullptr, *qualP = nullptr,
                       *vibP = nullptr, *humanP = nullptr, *brightP = nullptr,
                       *rvbP = nullptr, *masterP = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WbsProcessor)
};
