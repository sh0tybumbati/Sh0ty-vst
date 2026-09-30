#pragma once
#include "TubeStage.h"   // OnePole

// JUCE-free DSP for the BD-2 style overdrive.
//
// Modelled block-by-block from the Boss BD-2 "MT board" schematic. Voltages:
// 1.0 sample == 1 V.
//
//   JK2 -> R18 10k / C14 0.047 -> Q3 (2SK184 JFET source follower, R15 1M bias)
//      -> Q10/Q11 JFET buffers -> C22 0.15 / R29 22k (~48 Hz coupling HP)
//      -> Q9 (2SA1049 PNP gain stage, GAIN pot VR1A 250kA dual-gang;
//              C26 220p / R37 330k => ~2.2 kHz feedback roll-off)
//      -> D7..D10 (1SS133 diode clipper)
//      -> Q12 (2SA1049 PNP gain stage, GAIN pot VR1B; C17 5n6 / R26 5k6 => ~5 kHz)
//      -> IC1B (M5218 op-amp) tone stage, TONE pot VR2 10kB (C100/C101 0.018),
//              D1/D3 1SS133 limiting the feedback, R9 6k8 / C8 2n2 => ~10.6 kHz
//      -> LEVEL pot VR3 100kA -> Q5/Q1 output buffers, R1 1k -> JK3
//
// Simplifications: the transistor stages are modelled as gain + asymmetric
// rail clipping (PNP => different cutoff / saturation limits), the diode clippers
// as a smooth-knee symmetric limiter (~0.6 V), and the op-amp tone stage as a
// +/-10 dB tilt around 1 kHz. The GAIN pot's exact split across the two stages
// is an estimate. The FET bypass switching is not modelled.
namespace bd2
{
struct Params
{
    float gain  = 0.5f;  // 0..1
    float tone  = 0.5f;  // 0..1
    float level = 0.5f;  // 0..1
    float mix   = 1.f;
    float trim  = 1.f;   // linear input trim (guitar level -> volts)
};

class BD2
{
public:
    void prepare (float fs)
    {
        jfetLp.setCutoff (20000.f, fs);
        c22Hp.setCutoff (48.f, fs);        // C22 0.15u / R29 22k
        st1Lp.setCutoff (2200.f, fs);      // C26 220p / R37 330k
        st2Hp.setCutoff (30.f, fs);
        st2Lp.setCutoff (5100.f, fs);      // C17 5n6 / R26 5k6
        toneSplit.setCutoff (1000.f, fs);
        opLp.setCutoff (10600.f, fs);      // R9 6k8 / C8 2n2
        dcHp.setCutoff (20.f, fs);
        reset();
    }
    void reset()
    {
        for (auto* f : { &jfetLp, &c22Hp, &st1Lp, &st2Hp, &st2Lp, &toneSplit, &opLp, &dcHp })
            f->reset();
    }

    static float jfet (float v) { return std::tanh (v * 0.9f); }

    // Transistor stage rails: collector cuts off toward the supply on one half
    // and saturates on the other.
    static float rails (float v, float vp, float vn)
    {
        return v >= 0.f ? vp * std::tanh (v / vp) : vn * std::tanh (v / vn);
    }
    // Collector bias sits off-centre, so even small swings see an asymmetric curve.
    static float stage (float v, float vp, float vn, float bias)
    {
        return rails (v + bias, vp, vn) - rails (bias, vp, vn);
    }

    // Antiparallel-diode style limiter: linear well below vc, soft knee, hard-ish above.
    static float diodeClip (float x, float vp, float vn)
    {
        const float a = std::fabs (x) / (x >= 0.f ? vp : vn);
        return x / std::pow (1.f + std::pow (a, 2.5f), 1.f / 2.5f);
    }

    float process (float in, const Params& p)
    {
        const float vin = in * p.trim;
        float x = jfetLp.lp (jfet (vin));

        // dual-gang GAIN pot: 1.5x .. 19x in Q9, 1.2x .. 4.8x in Q12
        const float g1 = 1.5f * std::pow (10.f, p.gain * 1.1f);
        const float g2 = 1.2f * std::pow (10.f, p.gain * 0.6f);

        // Q9 stage (inverting) -> diode clipper
        x = c22Hp.hp (x);
        x = st1Lp.lp (x * g1);
        x = -stage (x, 4.2f, 3.0f, 1.0f);
        x = diodeClip (x, 0.62f, 0.62f);

        // Q12 stage (inverting) -> second diode pair
        x = st2Lp.lp (st2Hp.hp (x) * g2);
        x = -stage (x, 4.0f, 2.8f, 0.9f);
        x = diodeClip (x, 0.75f, 0.5f);

        // Tone stage: +/-10 dB tilt around 1 kHz, feedback diodes limit the output.
        const float lo = toneSplit.lp (x), hi = x - lo;
        const float tiltDb = (p.tone - 0.5f) * 20.f;
        const float gHi = std::pow (10.f, tiltDb / 20.f);
        x = opLp.lp (lo / gHi + hi * gHi);
        x = diodeClip (x, 0.9f, 0.9f);

        const float out = dcHp.hp (x) * volGain (p.level);
        return in * (1.f - p.mix) + out * p.mix;
    }

    static float volGain (float v) { return v <= 0.f ? 0.f : 3.f * v * v; }

private:
    ktg1::OnePole jfetLp, c22Hp, st1Lp, st2Hp, st2Lp, toneSplit, opLp, dcHp;
};
} // namespace bd2
