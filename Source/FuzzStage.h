#pragma once
#include "TubeStage.h"   // OnePole

// JUCE-free DSP for the FZ-3 style fuzz. Inspired by the Boss FZ-3 layout:
// JFET input buffer -> cascaded NPN gain stages with asymmetric clipping ->
// tone -> volume. Not a component-level circuit simulation.
namespace fz3
{
struct Params
{
    float fuzz   = 0.7f;  // 0..1
    float tone   = 0.5f;  // 0..1 (0 dark, 1 bright)
    float volume = 0.5f;  // 0..1
    float mix    = 1.f;
};

class FZ3
{
public:
    void prepare (float fs)
    {
        inHp.setCutoff (70.f, fs);
        jfetLp.setCutoff (16000.f, fs);
        int1Hp.setCutoff (90.f, fs);  int2Hp.setCutoff (120.f, fs);
        int1Lp.setCutoff (9000.f, fs); int2Lp.setCutoff (7000.f, fs);
        toneLo.setCutoff (650.f, fs); toneHi.setCutoff (2200.f, fs);
        dcHp.setCutoff (20.f, fs);
        reset();
    }
    void reset()
    {
        for (auto* f : { &inHp, &jfetLp, &int1Hp, &int2Hp, &int1Lp, &int2Lp, &toneLo, &toneHi, &dcHp })
            f->reset();
    }

    // NPN-ish clipper: exponential positive knee, harder negative knee.
    static float npn (float v)
    {
        return v >= 0.f ? 1.f - std::exp (-v) : -0.85f * (1.f - std::exp (1.6f * v));
    }
    static float jfet (float v) { return std::tanh (v * 0.9f + 0.05f) - std::tanh (0.05f); }

    float process (float in, const Params& p)
    {
        float x = jfetLp.lp (jfet (inHp.hp (in) * 1.2f));

        const float g1 = 3.f + 60.f * p.fuzz * p.fuzz;
        const float g2 = 2.f + 25.f * p.fuzz;
        float y = npn (x * g1);
        y = int1Lp.lp (int1Hp.hp (y));
        y = npn (y * g2);
        y = int2Lp.lp (int2Hp.hp (y));
        y = npn (y * 2.f);
        y = dcHp.hp (y);

        // Muff-style tone: crossfade between low-pass and high-pass paths.
        const float lo = toneLo.lp (y), hi = y - toneHi.lp (y);
        y = lo * (1.f - p.tone) * 1.3f + hi * p.tone * 1.6f + y * 0.15f;

        const float out = y * volGain (p.volume);
        return in * (1.f - p.mix) + out * p.mix;
    }

    static float volGain (float v) { return v <= 0.f ? 0.f : std::pow (10.f, (v - 0.5f) * 36.f / 20.f); }

private:
    ktg1::OnePole inHp, jfetLp, int1Hp, int2Hp, int1Lp, int2Lp, toneLo, toneHi, dcHp;
};
} // namespace fz3
