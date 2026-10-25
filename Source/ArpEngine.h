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

inline juce::StringArray octNames()
{
    return { "0", "+1", "+2", "+3", "-1", juce::String (juce::CharPointer_UTF8 ("\xc2\xb1" "1")) };   // last = "±1"
}

inline juce::StringArray keyNames() { return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }

inline juce::StringArray scaleNames()
{
    return { "OFF", "MAJOR", "MINOR", "DORIAN", "PHRYGIAN", "LYDIAN", "MIXOLYDIAN",
             "HARM MINOR", "PENTA MAJOR", "PENTA MINOR", "BLUES" };
}

// 12-bit masks, bit n = semitone n above the key is in the scale.
inline int scaleMask (int scale)
{
    auto m = [] (std::initializer_list<int> steps) { int r = 0; for (int s : steps) r |= 1 << s; return r; };
    switch (scale)
    {
        case 1:  return m ({ 0, 2, 4, 5, 7, 9, 11 });
        case 2:  return m ({ 0, 2, 3, 5, 7, 8, 10 });
        case 3:  return m ({ 0, 2, 3, 5, 7, 9, 10 });
        case 4:  return m ({ 0, 1, 3, 5, 7, 8, 10 });
        case 5:  return m ({ 0, 2, 4, 6, 7, 9, 11 });
        case 6:  return m ({ 0, 2, 4, 5, 7, 9, 10 });
        case 7:  return m ({ 0, 2, 3, 5, 7, 8, 11 });
        case 8:  return m ({ 0, 2, 4, 7, 9 });
        case 9:  return m ({ 0, 3, 5, 7, 10 });
        case 10: return m ({ 0, 3, 5, 6, 7, 10 });
        default: return 0xfff;
    }
}

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
        int transpose = 0, steps = 0, key = 0, scale = 0;   // steps 0 = full pattern
        int offset = 0;                                      // pattern start offset in steps
        bool inKey = false;
        bool sameSequence (const Settings& o) const
        { return algo == o.algo && noteRpt == o.noteRpt && octMode == o.octMode && octRpt == o.octRpt
                 && key == o.key && scale == o.scale; }
    };

    void prepare (double sr) { sampleRate = sr; reset(); }

    // UI preview of the running pattern (written on the audio thread, read by the editor).
    static constexpr int previewMax = 16;
    std::array<std::atomic<int>, previewMax> preview {};   // note number, -1 = random, -2 = chord
    std::atomic<int> previewCount { 0 };
    std::atomic<int> previewTick { 0 };
    std::atomic<int> transposeShown { 0 };   // current total transpose, for the panel readout

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
        transposeShown.store (cached.transpose);
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

        // SCALE: replace the held notes with every scale note from the lowest held note
        // up to the highest one (at least one octave), so even one key becomes a run.
        if (cached.scale > 0 && ! sorted.empty())
        {
            const auto base = sorted.front();
            const int lo = base.note, hi = std::max (sorted.back().note, lo + 11);
            const int mask = scaleMask (cached.scale);
            std::vector<Held> pool;
            for (int n = lo; n <= hi && n <= 127; ++n)
                if (mask & (1 << (((n - cached.key) % 12 + 12) % 12)))
                    pool.push_back ({ n, base.vel, base.channel });
            if (! pool.empty()) sorted = pool;
        }

        const auto pat = buildPattern (cached.algo, (int) sorted.size());
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
        const auto st = seq[stepIndex (seqPos++)];

        auto play = [&] (const Held& h)
        {
            const int n = transposed (h.note + 12 * st.oct);
            if (n < 0 || n > 127) return;
            out.addEvent (juce::MidiMessage::noteOn (h.channel, n, (juce::uint8) h.vel), pos);
            sounding.push_back ({ h.channel, n });
        };

        if (st.idx == -2)      for (auto& h : src) play (h);
        else if (st.idx == -1) play (src[(size_t) rng.nextInt ((int) src.size())]);
        else if (st.idx == -3) play (src[(size_t) (src.size() > 1 ? 1 + rng.nextInt ((int) src.size() - 1) : 0)]);
        else                   play (src[(size_t) juce::jmin (st.idx, (int) src.size() - 1)]);
    }

    // TRANSPOSE, then (IN KEY) snap to the nearest note of the selected scale.
    int transposed (int n) const
    {
        n += cached.transpose;
        if (cached.inKey && cached.scale > 0)
        {
            const int mask = scaleMask (cached.scale);
            auto inScale = [&] (int x) { return (mask & (1 << (((x - cached.key) % 12 + 12) % 12))) != 0; };
            for (int d = 0; d < 6; ++d)
            {
                if (inScale (n - d)) return n - d;   // prefer down on a tie
                if (inScale (n + d)) return n + d;
            }
        }
        return n;
    }

    // STEPS: loop only the first N steps of the sequence (0 = whole sequence).
    // OFFSET: start the (looped) pattern N steps later; wraps around.
    size_t stepIndex (juce::int64 pos) const
    {
        const auto size = (juce::int64) seq.size();
        const auto len  = cached.steps > 0 ? (juce::int64) cached.steps : size;
        return (size_t) (((pos + cached.offset) % len) % size);
    }

    void publishPreview (const std::vector<Held>& src)
    {
        const int n = (int) juce::jmin ((size_t) previewMax, seq.size());
        for (int i = 0; i < n; ++i)
        {
            const auto& st = seq[stepIndex (seqPos + i)];
            int v = st.idx;
            if (st.idx >= 0) v = src[(size_t) juce::jmin (st.idx, (int) src.size() - 1)].note + 12 * st.oct;
            if (st.idx >= 0) v = transposed (v);
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
