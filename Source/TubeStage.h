#pragma once
#include <algorithm>
#include <cmath>

// JUCE-free DSP for the KTG-1 style tube preamp. Kept header-only so it can be
// unit-tested without the plugin framework.
//
// NOTE: this is an "inspired by" model built from generic triode-stage
// behaviour (asymmetric soft clipping, coupling caps, grid/Miller low-pass),
// not a component-level model of the real KTG-1 circuit.
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

struct Params
{
    float drive  = 0.5f;  // 0..1
    float tone   = 0.5f;  // 0..1  (0 = dark, 1 = bright)
    float level  = 0.5f;  // 0..1
    float bright = 0.f;   // 0..1  pre-clip presence boost
    float mix    = 1.f;   // 0..1  dry/wet
};

class KTG1
{
public:
    // Runs at the oversampled rate.
    void prepare (float oversampledRate)
    {
        fs = oversampledRate;
        s1.prepare (fs); s2.prepare (fs);
        brightLp.setCutoff (2200.f, fs);
        toneLp.setCutoff (900.f, fs);
        inHp.setCutoff (35.f, fs);
        reset();
    }
    void reset() { s1.reset(); s2.reset(); brightLp.reset(); toneLp.reset(); inHp.reset(); }

    float process (float in, const Params& p)
    {
        float x = inHp.hp (in);

        const float hi = x - brightLp.lp (x);
        x += hi * p.bright * 1.5f;

        const float g1 = 1.f + 14.f * p.drive * p.drive;
        const float g2 = 1.f + 5.f * p.drive;
        float y = s1.process (x, g1);
        y = s2.process (y * 0.7f, g2);

        // tilt tone: lows vs highs around ~900 Hz
        const float lo = toneLp.lp (y), hiT = y - lo;
        y = lo * (1.4f - 0.8f * p.tone) + hiT * (0.6f + 0.8f * p.tone);

        const float out = y * levelGain (p.level);
        return in * (1.f - p.mix) + out * p.mix;
    }

    static float levelGain (float lvl)   // -inf..+12 dB, unity ~0.6
    {
        return lvl <= 0.f ? 0.f : std::pow (10.f, (lvl - 0.6f) * 30.f / 20.f);
    }

private:
    float fs = 48000.f;
    TriodeStage s1, s2;
    OnePole brightLp, toneLp, inHp;
};
} // namespace ktg1
