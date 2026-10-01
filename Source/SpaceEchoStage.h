#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// RE-201 Space Echo, modelled from the block diagram / circuit sheets:
//   mic amp -> recording pre-amp -> recording amp -> tape loop (1 record head, 3 playback heads, variable speed motor)
//   -> playback pre-amp -> echo volume;  INTENSITY feeds the playback signal back to the recording pre-amp;
//   a spring reverb is fed from the same send;  BASS / TREBLE shape the echo + reverb return;  the dry path goes
//   straight to the buffer amp and the H/M/L output pad.
// JUCE-free so it can be unit-tested with a plain executable. One instance = the mono tape machine.
namespace re201
{
constexpr int kModes = 12;

struct ModeInfo { bool head[3]; bool reverb; bool echo; };

// Dial positions: 1-4 echo only (heads 1, 2, 3, then 1 + 2), 5-11 echo + spring reverb with the head combinations, 12 reverb only.
inline ModeInfo modeInfo (int mode)
{
    static const ModeInfo table[kModes] = {
        { { true,  false, false }, false, true  },   //  1  head 1
        { { false, true,  false }, false, true  },   //  2  head 2
        { { false, false, true  }, false, true  },   //  3  head 3
        { { true,  true,  false }, false, true  },   //  4  heads 1 + 2
        { { true,  false, false }, true,  true  },   //  5  head 1 + reverb
        { { false, true,  false }, true,  true  },   //  6  head 2 + reverb
        { { false, false, true  }, true,  true  },   //  7  head 3 + reverb
        { { true,  true,  false }, true,  true  },   //  8  heads 1 + 2 + reverb
        { { false, true,  true  }, true,  true  },   //  9  heads 2 + 3 + reverb
        { { true,  false, true  }, true,  true  },   // 10  heads 1 + 3 + reverb
        { { true,  true,  true  }, true,  true  },   // 11  all heads + reverb
        { { false, false, false }, true,  false },   // 12  reverb only
    };
    return table[std::clamp (mode, 1, kModes) - 1];
}

// Head-1 delay in seconds for a REPEAT RATE setting (0 = slowest tape, 1 = fastest). Heads 2 and 3 sit at 2x and 3x.
inline float headOneDelay (float rate) { return 0.30f * std::pow (0.2f, std::clamp (rate, 0.f, 1.f)); }
constexpr float kHeadRatio[3] = { 1.f, 2.f, 3.f };

struct Params
{
    int   mode = 4;
    float repeatRate = 0.5f;
    float intensity = 0.35f;
    float echoVol = 0.6f;
    float reverbVol = 0.3f;
    float bass = 0.5f, treble = 0.5f;     // 0.5 = flat, +/- 12 dB
    bool  echoCancel = false;             // foot switch: kills the echo and repeats (the spring is left alone)
    bool  on = true;
    float wow = 1.f;                      // scales the transport's wow & flutter (0 for tests)
};

class SpaceEcho
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        bufLen = (size_t) (sr * 1.2f) + 8;
        tape.assign (bufLen, 0.f);
        // spring reverb: two springs of slightly different length, each with a dispersive allpass train
        const float springMs[2] = { 29.f, 37.f };
        for (int s = 0; s < 2; ++s)
        {
            spring[s].len = (size_t) std::max (16.f, springMs[s] * 0.001f * sr);
            spring[s].buf.assign (spring[s].len + 4, 0.f);
            spring[s].stages = std::clamp ((int) std::lround (30.f * sr / 48000.f), 12, 120);
            spring[s].ap.assign ((size_t) spring[s].stages, { 0.f });
        }
        reset();
    }

    void reset()
    {
        std::fill (tape.begin(), tape.end(), 0.f);
        w = 0;
        for (auto& s : spring) { std::fill (s.buf.begin(), s.buf.end(), 0.f); s.pos = 0; s.lp = 0.f; for (auto& a : s.ap) a = { 0.f }; s.hp = 0.f; }
        delayNow = headOneDelay (prm.repeatRate) * sr;
        loopLp = loopLp2 = loopHp = fbLp = 0.f; fbHp = 0.f;
        echoSm = revSm = intSm = 0.f; revGate = modeInfo (prm.mode).reverb ? 1.f : 0.f; cancelSm = 1.f; powerSm = prm.on ? 1.f : 0.f;
        bassF.reset(); trebF.reset();
        wowPh = 0.f; flutPh = 0.f; drift = 0.f; seed = 12345u;
        vu = 0.f; peakHold = 0.f;
        updateTone();
    }

    void setParams (const Params& p)
    {
        const bool toneChanged = p.bass != prm.bass || p.treble != prm.treble;
        prm = p;
        if (toneChanged) updateTone();
    }

    // one mono sample in (the send), returns the echo + reverb return (without the dry signal)
    float process (float in)
    {
        const auto mi = modeInfo (prm.mode);

        // slew the front-panel controls
        auto slew = [] (float& s, float target, float c) { s += (target - s) * c; };
        slew (echoSm, 1.6f * prm.echoVol * prm.echoVol, 0.0015f);
        slew (revSm,  1.5f * prm.reverbVol * prm.reverbVol, 0.0015f);
        slew (intSm,  1.08f * std::pow (prm.intensity, 1.2f), 0.0015f);
        slew (cancelSm, prm.echoCancel ? 0.f : 1.f, 0.002f);
        slew (powerSm, prm.on ? 1.f : 0.f, 0.002f);

        // recording pre-amp (the mic amp's soft clip) -- also feeds the VU / peak lamp
        const float pre = softClip (in * 1.0f);
        const float a = std::fabs (pre);
        vu += (a - vu) * (a > vu ? 0.02f : 0.0004f);
        peakHold = std::max (a > 0.82f ? 1.f : 0.f, peakHold - 1.f / (0.25f * sr));

        // transport: the motor slews towards the new speed (so turning REPEAT RATE bends the pitch), plus wow & flutter
        const float target = headOneDelay (prm.repeatRate) * sr;
        delayNow += (target - delayNow) * (1.f / (0.25f * sr));
        float mod = 0.f;
        if (prm.wow > 0.f)
        {
            wowPh  += 0.55f / sr; if (wowPh  >= 1.f) wowPh  -= 1.f;
            flutPh += 7.3f  / sr; if (flutPh >= 1.f) flutPh -= 1.f;
            seed = seed * 1664525u + 1013904223u;
            const float noise = (float) (seed >> 8) / 8388608.f - 1.f;
            drift += (noise * 0.02f - drift) * 0.0002f;
            const float tw = 6.2831853f;
            mod = prm.wow * (0.00020f * std::sin (tw * wowPh) + 0.00004f * std::sin (tw * flutPh) + 0.00050f * drift) * sr;
        }

        // playback: sum of the active heads (each at 1/sqrt(n) so the level does not jump between modes)
        int n = 0; for (int h = 0; h < 3; ++h) n += mi.head[h] ? 1 : 0;
        float play = 0.f;
        if (n > 0 && mi.echo)
        {
            const float g = 1.f / std::sqrt ((float) n);
            for (int h = 0; h < 3; ++h)
                if (mi.head[h]) play += g * readTape (delayNow * kHeadRatio[h] + mod * kHeadRatio[h]);
        }

        // head / tape response: gap + self-erase loss rolls the top off, the transformer rolls the bottom off.
        // Faster tape = more bandwidth.
        const float speedNorm = std::clamp (prm.repeatRate, 0.f, 1.f);
        const float fc = 3800.f + 3600.f * speedNorm;
        const float lpC = 1.f - std::exp (-6.2831853f * fc / sr);
        loopLp  += (play   - loopLp)  * lpC;
        loopLp2 += (loopLp - loopLp2) * lpC;
        const float hpC = 1.f - std::exp (-6.2831853f * 45.f / sr);
        loopHp += (loopLp2 - loopHp) * hpC;
        const float playback = loopLp2 - loopHp;

        // INTENSITY feeds the playback back to the recording pre-amp; the tape saturates before it runs away
        const float fbAmt = intSm * cancelSm * powerSm;
        const float rec = tapeSat (pre + fbAmt * playback);
        tape[w] = rec;
        if (++w >= bufLen) w = 0;

        const float echoOut = playback * echoSm * cancelSm * powerSm;

        // spring reverb, fed from the same send
        slew (revGate, mi.reverb ? 1.f : 0.f, 0.002f);
        const float rev = springReverb (pre * revGate);
        const float revOut = rev * revSm * powerSm * revGate;

        // BASS / TREBLE on the return
        float y = echoOut + revOut;
        y = trebF.process (bassF.process (y));
        return y;
    }

    float vuLevel() const { return vu; }
    float peakLamp() const { return peakHold; }

private:
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        void reset() { z1 = z2 = 0.f; }
        float process (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
        void shelf (bool high, float fs, float f0, float gainDb)
        {
            const float A = std::pow (10.f, gainDb / 40.f), w0 = 6.2831853f * f0 / fs, cw = std::cos (w0), sw = std::sin (w0);
            const float alpha = sw / 2.f * std::sqrt (2.f), sA = 2.f * std::sqrt (A) * alpha;
            float B0, B1, B2, A0, A1, A2;
            if (! high) { B0 = A * ((A + 1) - (A - 1) * cw + sA); B1 = 2 * A * ((A - 1) - (A + 1) * cw); B2 = A * ((A + 1) - (A - 1) * cw - sA);
                          A0 = (A + 1) + (A - 1) * cw + sA;       A1 = -2 * ((A - 1) + (A + 1) * cw);    A2 = (A + 1) + (A - 1) * cw - sA; }
            else        { B0 = A * ((A + 1) + (A - 1) * cw + sA); B1 = -2 * A * ((A - 1) + (A + 1) * cw); B2 = A * ((A + 1) + (A - 1) * cw - sA);
                          A0 = (A + 1) - (A - 1) * cw + sA;       A1 = 2 * ((A - 1) - (A + 1) * cw);     A2 = (A + 1) - (A - 1) * cw - sA; }
            b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
        }
    };
    struct Spring
    {
        std::vector<float> buf; size_t len = 0, pos = 0; float lp = 0.f, hp = 0.f; int stages = 0;
        std::vector<std::array<float, 1>> ap;
    };

    static float softClip (float x) { return std::tanh (x * 0.9f) / 0.9f * 0.98f; }
    static float tapeSat (float x) { return std::tanh (x * 0.8f) / 0.8f; }

    void updateTone()
    {
        bassF.shelf (false, sr, 110.f, (prm.bass - 0.5f) * 24.f);
        trebF.shelf (true,  sr, 3800.f, (prm.treble - 0.5f) * 24.f);
    }

    // cubic (Catmull-Rom) read, d samples behind the write head
    float readTape (float d) const
    {
        d = std::clamp (d, 2.f, (float) bufLen - 4.f);
        const float rp = (float) w - d;
        float fl = std::floor (rp);
        const float t = rp - fl;
        long i = (long) fl;
        auto at = [&] (long k) { long m = k % (long) bufLen; if (m < 0) m += (long) bufLen; return tape[(size_t) m]; };
        const float y0 = at (i - 1), y1 = at (i), y2 = at (i + 1), y3 = at (i + 2);
        return y1 + 0.5f * t * (y2 - y0 + t * (2.f * y0 - 5.f * y1 + 4.f * y2 - y3 + t * (3.f * (y1 - y2) + y3 - y0)));
    }

    // two parallel torsion springs: a delay with a chain of allpasses (the dispersion -> chirpy "boing"), damped, fed back
    float springReverb (float in)
    {
        float out = 0.f;
        const float damp = 1.f - std::exp (-6.2831853f * 4500.f / sr);
        const float hpC  = 1.f - std::exp (-6.2831853f * 150.f / sr);
        for (int s = 0; s < 2; ++s)
        {
            auto& sp = spring[s];
            float tail = sp.buf[sp.pos];
            // dispersive allpass chain, a < 0 -> low frequencies are delayed more than high ones
            for (auto& st : sp.ap) { const float a = -0.68f; const float y = a * tail + st[0]; st[0] = tail - a * y; tail = y; }
            sp.lp += (tail - sp.lp) * damp;
            sp.hp += (sp.lp - sp.hp) * hpC;
            const float fed = sp.lp - sp.hp;
            const float g = (s == 0 ? 0.985f : 0.975f);
            sp.buf[sp.pos] = in * 0.9f + fed * g;
            if (++sp.pos >= sp.len) sp.pos = 0;
            out += fed;
        }
        return out * 0.7f;
    }

    Params prm;
    float sr = 48000.f;
    std::vector<float> tape; size_t bufLen = 0, w = 0;
    float delayNow = 0.f;
    float loopLp = 0, loopLp2 = 0, loopHp = 0, fbLp = 0, fbHp = 0;
    float revGate = 0, echoSm = 0, revSm = 0, intSm = 0, cancelSm = 1, powerSm = 1;
    Biquad bassF, trebF;
    Spring spring[2];
    float wowPh = 0, flutPh = 0, drift = 0; uint32_t seed = 1;
    float vu = 0, peakHold = 0;
};
} // namespace re201
