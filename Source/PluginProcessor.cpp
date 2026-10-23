#include "PluginProcessor.h"
#include "PluginEditor.h"

OmniarpProcessor::OmniarpProcessor()
    : AudioProcessor (BusesProperties()),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout OmniarpProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { "hold", 1 },    "Hold",      false));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "algo", 1 },    "Algorithm", tross::algoNames(), 0));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "gate", 1 },    "Gate",      0.0f, 1.0f, 0.5f));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "swing", 1 },   "Swing",     0.0f, 1.0f, 0.0f));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "rate", 1 },    "Note",      tross::rateNames(), 6));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "noteRpt", 1 }, "Note Rpt",  1, 5, 1));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "oct", 1 },     "Oct",       tross::octNames(), 0));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "octRpt", 1 },  "Oct Rpt",   1, 5, 1));
    return { p.begin(), p.end() };
}

void OmniarpProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    buffer.clear();

    tross::ArpEngine::Settings s;
    s.hold    = apvts.getRawParameterValue ("hold")->load() > 0.5f;
    s.algo    = (int) apvts.getRawParameterValue ("algo")->load();
    s.gate    = apvts.getRawParameterValue ("gate")->load();
    s.swing   = apvts.getRawParameterValue ("swing")->load();
    s.rate    = (int) apvts.getRawParameterValue ("rate")->load();
    s.noteRpt = (int) apvts.getRawParameterValue ("noteRpt")->load();
    s.octMode = (int) apvts.getRawParameterValue ("oct")->load();
    s.octRpt  = (int) apvts.getRawParameterValue ("octRpt")->load();

    double bpm = 120.0, ppq = 0.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            playing = pos->getIsPlaying() && pos->getPpqPosition().hasValue();
        }

    engine.process (midi, buffer.getNumSamples(), s, bpm, ppq, playing);
}

juce::AudioProcessorEditor* OmniarpProcessor::createEditor() { return new OmniarpEditor (*this); }

void OmniarpProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void OmniarpProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OmniarpProcessor(); }
