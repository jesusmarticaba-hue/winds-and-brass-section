#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

WbsProcessor::WbsProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < wbs::kNumPlayers; ++i)
    {
        const String id (wbs::specs()[(size_t) i].id);
        onP[(size_t) i]  = apvts.getRawParameterValue ("on_" + id);
        lvlP[(size_t) i] = apvts.getRawParameterValue ("lvl_" + id);
        activity[(size_t) i] = 0.f;
    }
    modeP   = apvts.getRawParameterValue ("mode");
    articP  = apvts.getRawParameterValue ("artic");
    rootP   = apvts.getRawParameterValue ("root");
    qualP   = apvts.getRawParameterValue ("quality");
    vibP    = apvts.getRawParameterValue ("vibrato");
    humanP  = apvts.getRawParameterValue ("human");
    brightP = apvts.getRawParameterValue ("bright");
    rvbP    = apvts.getRawParameterValue ("reverb");
    masterP = apvts.getRawParameterValue ("master");
}

AudioProcessorValueTreeState::ParameterLayout WbsProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode",
                                                        StringArray { "Unison", "Poly Section", "Auto Section" }, 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "artic", 1 }, "Articulation",
                                                        wbs::articulationNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "root", 1 }, "Chord Root", wbs::rootNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "quality", 1 }, "Chord Quality", wbs::chordNames(), 0));

    auto addFloat = [&layout] (const char* id, const char* name, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                           NormalisableRange<float> (0.f, 1.f), def));
    };
    addFloat ("vibrato", "Vibrato", 0.5f);
    addFloat ("human",   "Humanize", 0.4f);
    addFloat ("bright",  "Brightness", 0.6f);
    addFloat ("reverb",  "Reverb", 0.25f);
    addFloat ("master",  "Master", 0.8f);

    for (const auto& sp : wbs::specs())
    {
        const String id (sp.id), name (sp.name);
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { "on_" + id, 1 }, name + " On", true));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "lvl_" + id, 1 }, name + " Level",
                                                           NormalisableRange<float> (0.f, 1.f), 0.8f));
    }
    return layout;
}

void WbsProcessor::prepareToPlay (double sampleRate, int)
{
    section.prepare (sampleRate);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
}

bool WbsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

wbs::Section::Settings WbsProcessor::readSettings() const
{
    wbs::Section::Settings s;
    s.mode    = (int) modeP->load();
    s.artic   = (int) articP->load();
    s.root    = (int) rootP->load();
    s.quality = (int) qualP->load();
    s.vibrato = vibP->load();
    s.human   = humanP->load();
    s.bright  = brightP->load();
    for (size_t i = 0; i < (size_t) wbs::kNumPlayers; ++i)
    {
        s.on[i]    = onP[i]->load() > 0.5f;
        s.level[i] = lvlP[i]->load();
    }
    return s;
}

void WbsProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2) return;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    const auto s = readSettings();

    // Reasigna por si cambiaron modo / activación / acordes desde la interfaz
    section.refresh (s);

    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = jlimit (0, numSamples, meta.samplePosition);
        if (t > pos)
        {
            section.render (L + pos, R + pos, t - pos, s);
            pos = t;
        }
        section.handleMidi (meta.getMessage(), s);
    }
    if (pos < numSamples)
        section.render (L + pos, R + pos, numSamples - pos, s);

    // Reverb de sala
    const float mix = rvbP->load();
    Reverb::Parameters rp;
    rp.roomSize = 0.55f; rp.damping = 0.45f; rp.width = 1.f; rp.freezeMode = 0.f;
    rp.wetLevel = mix * 0.35f;
    rp.dryLevel = 1.f - mix * 0.25f;
    reverb.setParameters (rp);
    reverb.processStereo (L, R, numSamples);

    // Master + limitador suave
    const float g = masterP->load() * 0.45f;
    for (int i = 0; i < numSamples; ++i)
    {
        L[i] = std::tanh (L[i] * g);
        R[i] = std::tanh (R[i] * g);
    }

    shownArtic = section.getEffectiveArtic();
    for (int p = 0; p < wbs::kNumPlayers; ++p)
        activity[(size_t) p] = section.getPlayerLevel (p);
}

AudioProcessorEditor* WbsProcessor::createEditor() { return new WbsEditor (*this); }

void WbsProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void WbsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WbsProcessor(); }
