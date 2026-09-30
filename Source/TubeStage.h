#pragma once
#include <algorithm>
#include <cmath>

// JUCE-free DSP for the KTG-1 style two-channel tube guitar preamp.
//
// Panel-derived structure (from photos of the real unit):
//   INPUT -> shared BASS / MID / TREBLE
//   CHANNEL 1: OVERDRIVE (pull boost), MASTER
//   CHANNEL 2: OVERDRIVE (pull boost), MASTER (pull crunch)
//   OUTPUT, CHANNEL SELECTOR, ON/OFF
//
// There is no schematic behind this, so it is an "inspired by" model built from
// generic triode-stage behaviour (asymmetric saturation, coupling caps, Miller
// low-pass), not a component-level model of the real circuit. The signal order
// (gain stages -> master -> shared tone stack -> output) and the boost / crunch
// voicings are estimates.
namespace ktg1
{
struct OnePole
{
    float a = 0.f, z = 0.f;
    void setCutoff (float fc, float fs) { a = 1.f - std::exp (-2.f * 3.14159265f * fc / fs); }
    float lp (float x) { z += a * (x - z); return z; }
    float hp (float x) { return x - lp (x); }
    void reset() { z = 0.f; }
};

// One triode gain stage: pre-clip low-pass, asymmetric saturation, DC block.
struct TriodeStage
{
    OnePole miller, dcBlock;
    float bias = 0.12f;      // grid bias offset => even harmonics

    void prepare (float fs)
    {
        miller.setCutoff (14000.f, fs);
        dcBlock.setCutoff (18.f, fs);   // coupling capacitor
        reset();
    }
    void reset() { miller.reset(); dcBlock.reset(); }

    static float shape (float v)
    {
        // Positive half compresses harder than negative (plate/grid asymmetry).
        return v >= 0.f ? std::tanh (v) : 1.25f * std::tanh (0.8f * v);
    }

    float process (float x, float gain)
    {
        const float v = miller.lp (x) * gain + bias;
        const float y = shape (v) - shape (bias);   // remove static offset
        return -dcBlock.hp (y);                      // triode inverts phase
    }
};

// RBJ biquad (transposed direct form II).
struct Biquad
{
    float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f, z1 = 0.f, z2 = 0.f;
    void reset() { z1 = z2 = 0.f; }
    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void set (float nb0, float nb1, float nb2, float a0, float na1, float na2)
    {
        b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0; a1 = na1 / a0; a2 = na2 / a0;
    }
    void lowShelf (float fs, float f, float dB)
    {
        const float A = std::pow (10.f, dB / 40.f), w = 2.f * 3.14159265f * f / fs;
        const float cw = std::cos (w), alpha = std::sin (w) / 2.f * std::sqrt (2.f), t = 2.f * std::sqrt (A) * alpha;
        set (A * ((A + 1) - (A - 1) * cw + t), 2 * A * ((A - 1) - (A + 1) * cw), A * ((A + 1) - (A - 1) * cw - t),
             (A + 1) + (A - 1) * cw + t, -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - t);
    }
    void highShelf (float fs, float f, float dB)
    {
        const float A = std::pow (10.f, dB / 40.f), w = 2.f * 3.14159265f * f / fs;
        const float cw = std::cos (w), alpha = std::sin (w) / 2.f * std::sqrt (2.f), t = 2.f * std::sqrt (A) * alpha;
        set (A * ((A + 1) + (A - 1) * cw + t), -2 * A * ((A - 1) + (A + 1) * cw), A * ((A + 1) + (A - 1) * cw - t),
             (A + 1) - (A - 1) * cw + t, 2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - t);
    }
    void peak (float fs, float f, float q, float dB)
    {
        const float A = std::pow (10.f, dB / 40.f), w = 2.f * 3.14159265f * f / fs;
        const float cw = std::cos (w), alpha = std::sin (w) / (2.f * q);
        set (1 + alpha * A, -2 * cw, 1 - alpha * A, 1 + alpha / A, -2 * cw, 1 - alpha / A);
    }
};

struct Params
{
    float bass = 0.5f, mid = 0.5f, treble = 0.5f;   // 0..1, 0.5 = flat
    float od1 = 0.5f, master1 = 0.5f;               // channel 1
    float od2 = 0.5f, master2 = 0.5f;               // channel 2
    float output = 0.5f;
    float trim = 1.f;                               // linear input trim
    bool  boost1 = false, boost2 = false, crunch2 = false;
    bool  ch2 = false;                              // false = channel 1
    bool  on = true;
};

class KTG1
{
public:
    void prepare (float oversampledRate)
    {
        fs = oversampledRate;
        for (auto* s : { &s11, &s12, &s21, &s22, &s23 }) s->prepare (fs);
        inHp.setCutoff (35.f, fs);
        chSmooth.setCutoff (60.f, fs);
        chTarget = 0.f;
        reset();
        cached = false;
    }
    void reset()
    {
        for (auto* s : { &s11, &s12, &s21, &s22, &s23 }) s->reset();
        inHp.reset(); chSmooth.reset(); env = 0.f;
        bassBq.reset(); midBq.reset(); trebBq.reset();
    }

    void setParams (const Params& np)
    {
        if (!cached || np.bass != p.bass) bassBq.lowShelf (fs, 110.f, (np.bass - 0.5f) * 24.f);
        if (!cached || np.mid != p.mid)   midBq.peak (fs, 700.f, 0.8f, (np.mid - 0.5f) * 20.f);
        if (!cached || np.treble != p.treble) trebBq.highShelf (fs, 3200.f, (np.treble - 0.5f) * 24.f);
        p = np;
        cached = true;
        chTarget = p.ch2 ? 1.f : 0.f;
    }

    float process (float in)
    {
        if (!p.on) return in;

        const float x = inHp.hp (in * p.trim);

        // --- Channel 1: cleaner, two stages, pull-boost adds gain ---
        const float b1 = p.boost1 ? 2.4f : 1.f;
        float a = s11.process (x, (1.f + 9.f * p.od1 * p.od1) * b1);
        a = s12.process (a * 0.6f, 1.5f + 3.f * p.od1 * (p.boost1 ? 1.6f : 1.f));
        a *= masterGain (p.master1);

        // --- Channel 2: hotter, three stages, pull-boost adds gain, pull-crunch adds power-stage squash ---
        const float b2 = p.boost2 ? 2.4f : 1.f;
        float b = s21.process (x, (1.f + 16.f * p.od2 * p.od2) * b2);
        b = s22.process (b * 0.6f, 2.f + 7.f * p.od2 * (p.boost2 ? 1.5f : 1.f));
        b = s23.process (b * 0.6f, 1.5f + 3.f * p.od2);
        b *= masterGain (p.master2);
        if (p.crunch2)
        {
            env = std::max (std::fabs (b), env * 0.9995f);          // slow "sag"
            b = std::tanh (b * 1.8f / (1.f + 0.5f * env)) * 0.6f;
        }

        const float w = chSmooth.lp (chTarget);
        float y = a * (1.f - w) + b * w;

        y = trebBq.process (midBq.process (bassBq.process (y)));
        return y * outputGain (p.output);
    }

    static float masterGain (float m) { return 3.f * m * m; }
    static float outputGain (float o) { return 4.f * o * o; }

private:
    float fs = 48000.f, env = 0.f, chTarget = 0.f;
    bool cached = false;
    Params p;
    TriodeStage s11, s12, s21, s22, s23;
    OnePole inHp, chSmooth;
    Biquad bassBq, midBq, trebBq;
};
} // namespace ktg1
