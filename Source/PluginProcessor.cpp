#include "PluginProcessor.h"
#include "PluginEditor.h"

OmniarpProcessor::OmniarpProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
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
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "transpose", 1 }, "Transpose", -12, 12, 0,
        AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return v > 0 ? "+" + String (v) : String (v); })));
    p.push_back (std::make_unique<AudioParameterInt>    (ParameterID { "steps", 1 }, "Steps", 0, 16, 0,
        AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return v == 0 ? String ("OFF") : String (v); })));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 },   "Key",   tross::keyNames(), 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale", tross::scaleNames(), 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "tzone", 1 },   "Transpose Zone",   tross::zoneNames(), 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "ttiming", 1 }, "Transpose Timing", tross::timingNames(), 0));
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
    s.transpose = (int) apvts.getRawParameterValue ("transpose")->load();
    s.steps     = (int) apvts.getRawParameterValue ("steps")->load();
    s.key       = (int) apvts.getRawParameterValue ("key")->load();
    s.scale     = (int) apvts.getRawParameterValue ("scale")->load();
    s.zone      = (int) apvts.getRawParameterValue ("tzone")->load();
    s.timing    = (int) apvts.getRawParameterValue ("ttiming")->load();

    double bpm = 120.0, ppq = 0.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            if (auto ts = pos->getTimeSignature())
                s.beatsPerBar = juce::jmax (1, ts->numerator * 4 / juce::jmax (1, ts->denominator));
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
