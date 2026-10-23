#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <array>
#include <atomic>

namespace tross
{
enum Algo { Up, Down, UpDown, Foldback, UpSkip, DownSkip, PedalUp, PedalDown,
            PedalRand, Random, Chord };

inline juce::StringArray algoNames()
{
    return { "UP", "DOWN", "UP/DOWN", "FOLDBACK", "UP SKIP", "DOWN SKIP", "PEDAL UP",
             "PEDAL DOWN", "PEDAL RAND", "RAND", "CHORD" };
}

inline juce::StringArray rateNames()
{
    return { "1/4", "1/4T", "1/8.", "1/8", "1/8T", "1/16.", "1/16", "1/16T", "1/32" };
}

inline double rateQuarters (int i)
{
    static const double q[] = { 1.0, 2.0 / 3.0, 0.75, 0.5, 1.0 / 3.0, 0.375, 0.25, 1.0 / 6.0, 0.125 };
    return q[juce::jlimit (0, 8, i)];
}

inline juce::StringArray octNames() { return { "0", "+1", "+2", "+3", "-1", "-/+1" }; }

inline std::vector<int> octaveList (int mode)
{
    switch (mode)
    {
        case 1:  return { 0, 1 };
        case 2:  return { 0, 1, 2 };
        case 3:  return { 0, 1, 2, 3 };
        case 4:  return { 0, -1 };
        case 5:  return { -1, 0, 1 };
        default: return { 0 };
    }
}

// Indices into the sorted note list. -1 = random note, -2 = whole chord, -3 = random non-pedal note.
inline std::vector<int> buildPattern (int algo, int n)
{
    std::vector<int> p;
    if (n <= 0) return p;
    auto mirror = [n] (std::vector<int> v) { for (auto& i : v) i = n - 1 - i; return v; };

    switch (algo)
    {
        case Down:
            for (int i = n - 1; i >= 0; --i) p.push_back (i);
            break;
        case UpDown:
            for (int i = 0; i < n; ++i) p.push_back (i);
            for (int i = n - 2; i > 0; --i) p.push_back (i);
            break;
        case Foldback:
            for (int i = 0; i < n; ++i) p.push_back (i);
            for (int i = n - 1; i >= 0; --i) p.push_back (i);
            break;
        case UpSkip:
        case DownSkip:
            for (int i = 0; i < n; ++i) { p.push_back (i); if (i + 2 < n) p.push_back (i + 2); }
            if (algo == DownSkip) p = mirror (p);
            break;
        case PedalUp:
            if (n == 1) p.push_back (0);
            for (int i = 1; i < n; ++i) { p.push_back (0); p.push_back (i); }
            break;
        case PedalDown:
            if (n == 1) p.push_back (0);
            for (int i = n - 2; i >= 0; --i) { p.push_back (n - 1); p.push_back (i); }
            break;
        case PedalRand:
            if (n == 1) p.push_back (0);
            for (int i = 1; i < n; ++i) { p.push_back (0); p.push_back (-3); }
            break;
        case Random:
            p.assign ((size_t) n, -1);
            break;
        case Chord:
            p.push_back (-2);
            break;
        default: // Up
            for (int i = 0; i < n; ++i) p.push_back (i);
            break;
    }
    return p;
}

class ArpEngine
{
public:
    struct Settings
    {
        bool hold = false; int algo = 0; float gate = 0.5f, swing = 0.0f;
        int rate = 6, noteRpt = 1, octMode = 0, octRpt = 1;
        bool sameSequence (const Settings& o) const
        { return algo == o.algo && noteRpt == o.noteRpt && octMode == o.octMode && octRpt == o.octRpt; }
    };

    void prepare (double sr) { sampleRate = sr; reset(); }

    // UI preview of the running pattern (written on the audio thread, read by the editor).
    static constexpr int previewMax = 16;
    std::array<std::atomic<int>, previewMax> preview {};   // note number, -1 = random, -2 = chord
    std::atomic<int> previewCount { 0 };
    std::atomic<int> previewTick { 0 };

    void reset()
    {
        held.clear(); physical.clear(); sounding.clear(); seq.clear(); previewCount.store (0);
        seqPos = 0; lastTrigQ = -1.0e9; lastEndQ = -1.0e9; internalQ = 0.0; dirty = true;
    }

    void process (juce::MidiBuffer& midi, int numSamples, const Settings& s,
                  double bpm, double hostQ, bool playing)
    {
        juce::MidiBuffer out;

        for (const auto m : midi)
        {
            const auto msg = m.getMessage();
            if (msg.isNoteOn())
            {
                // With HOLD on, a new chord starts when a key is pressed after all keys were released.
                if (s.hold && physical.empty()) { held.clear(); seqPos = 0; dirty = true; }
                physical.push_back (msg.getNoteNumber());
                addHeld (msg.getNoteNumber(), msg.getVelocity(), msg.getChannel());
            }
            else if (msg.isNoteOff())
            {
                const int n = msg.getNoteNumber();
                physical.erase (std::remove (physical.begin(), physical.end(), n), physical.end());
                if (! s.hold) removeHeld (n);
            }
            else
            {
                if (msg.isAllNotesOff() || msg.isAllSoundOff()) { held.clear(); physical.clear(); dirty = true; }
                out.addEvent (msg, m.samplePosition);
            }
        }

        // HOLD switched off: drop latched notes that are no longer physically held.
        if (wasHold && ! s.hold)
            for (auto it = held.begin(); it != held.end();)
            {
                if (std::find (physical.begin(), physical.end(), it->note) == physical.end()) { it = held.erase (it); dirty = true; }
                else ++it;
            }
        wasHold = s.hold;

        if (! s.sameSequence (cached)) dirty = true;
        cached = s;

        const double spq    = sampleRate * 60.0 / juce::jlimit (20.0, 999.0, bpm);
        const double startQ = playing ? hostQ : internalQ;
        const double endQ   = startQ + numSamples / spq;

        if (startQ < lastEndQ - 1.0e-4 || startQ > lastEndQ + 0.05)   // transport jump / loop
        {
            release (out, 0);
            lastTrigQ = -1.0e9;
        }
        lastEndQ = internalQ = endQ;

        const double stepQ   = rateQuarters (s.rate);
        const double swingQ  = s.swing * 0.5 * stepQ;
        auto stepStart = [&] (juce::int64 k) { return (double) k * stepQ + ((k & 1) ? swingQ : 0.0); };
        auto toSample  = [&] (double q) { return juce::jlimit (0, numSamples - 1, (int) ((q - startQ) * spq)); };

        auto k = (juce::int64) std::floor (startQ / stepQ) - 1;
        while (stepStart (k) < startQ) ++k;

        for (;; ++k)
        {
            const double t = stepStart (k);
            if (t >= endQ) break;
            if (! sounding.empty() && offQ <= t) release (out, toSample (offQ));
            if (t <= lastTrigQ + 1.0e-9) continue;

            lastTrigQ = t;
            const int pos = toSample (t);
            release (out, pos);
            trigger (out, pos);
            offQ = t + std::max (0.02f, s.gate) * stepQ;
        }

        if (! sounding.empty() && offQ < endQ) release (out, toSample (offQ));
        midi.swapWith (out);
    }

private:
    struct Held  { int note, vel, channel; };
    struct Step  { int idx, oct; };
    struct Sound { int channel, note; };

    void addHeld (int note, int vel, int ch)
    {
        if (held.empty()) seqPos = 0;
        removeHeld (note);
        held.push_back ({ note, vel, ch });
        dirty = true;
    }

    void removeHeld (int note)
    {
        held.erase (std::remove_if (held.begin(), held.end(), [note] (const Held& h) { return h.note == note; }), held.end());
        dirty = true;
    }

    void rebuild()
    {
        sorted = held;
        std::sort (sorted.begin(), sorted.end(), [] (const Held& a, const Held& b) { return a.note < b.note; });
        const auto pat = buildPattern (cached.algo, (int) held.size());
        seq.clear();
        for (int o : octaveList (cached.octMode))
            for (int r = 0; r < cached.octRpt; ++r)
                for (int i : pat)
                    for (int nr = 0; nr < cached.noteRpt; ++nr)
                        seq.push_back ({ i, o });
        dirty = false;
    }

    void trigger (juce::MidiBuffer& out, int pos)
    {
        if (held.empty()) { previewCount.store (0); return; }
        if (dirty) rebuild();
        if (seq.empty()) return;

        const auto& src = sorted;
        publishPreview (src);
        const auto st = seq[(size_t) (seqPos++ % (juce::int64) seq.size())];

        auto play = [&] (const Held& h)
        {
            const int n = h.note + 12 * st.oct;
            if (n < 0 || n > 127) return;
            out.addEvent (juce::MidiMessage::noteOn (h.channel, n, (juce::uint8) h.vel), pos);
            sounding.push_back ({ h.channel, n });
        };

        if (st.idx == -2)      for (auto& h : src) play (h);
        else if (st.idx == -1) play (src[(size_t) rng.nextInt ((int) src.size())]);
        else if (st.idx == -3) play (src[(size_t) (src.size() > 1 ? 1 + rng.nextInt ((int) src.size() - 1) : 0)]);
        else                   play (src[(size_t) juce::jmin (st.idx, (int) src.size() - 1)]);
    }

    void publishPreview (const std::vector<Held>& src)
    {
        const int n = (int) juce::jmin ((size_t) previewMax, seq.size());
        for (int i = 0; i < n; ++i)
        {
            const auto& st = seq[(size_t) ((seqPos + i) % (juce::int64) seq.size())];
            int v = st.idx;
            if (st.idx >= 0) v = src[(size_t) juce::jmin (st.idx, (int) src.size() - 1)].note + 12 * st.oct;
            preview[(size_t) i].store (v, std::memory_order_relaxed);
        }
        previewCount.store (n);
        previewTick.fetch_add (1);
    }

    void release (juce::MidiBuffer& out, int pos)
    {
        for (auto& sn : sounding) out.addEvent (juce::MidiMessage::noteOff (sn.channel, sn.note), pos);
        sounding.clear();
    }

    double sampleRate = 44100.0, lastTrigQ = -1.0e9, lastEndQ = -1.0e9, internalQ = 0.0, offQ = 0.0;
    std::vector<Held> held, sorted;
    std::vector<Step> seq;
    std::vector<Sound> sounding;
    juce::int64 seqPos = 0;
    bool dirty = true, wasHold = false;
    std::vector<int> physical;
    Settings cached;
    juce::Random rng;
};
} // namespace tross
