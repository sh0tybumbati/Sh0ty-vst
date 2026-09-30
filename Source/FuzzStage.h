#pragma once
#include "TubeStage.h"   // OnePole

// JUCE-free DSP for the FZ-3 style fuzz.
//
// Modelled block-by-block from the "Boss FZ3" schematic sheet (Dirk Hendrik,
// 2007 redraw), signal path only. Voltages: 1.0 sample == 1 V, VCC == 9 V.
//
//   J1 -> R4A/C5A -> T2A (2SK184 JFET source follower)
//      -> C6 -> T4 (2SK118 JFET buffer) -> C7/R14 (~60 Hz coupling HP)
//      -> Q1 (CE, Rc 12k / Re 10k, gain ~-1.2; C9+R17 shunt = ~-15 dB
//              treble shelf above ~1.3 kHz)
//      -> Q2 (emitter follower) -> R19/C10, C11 (R19 6k8 + C11 10n = ~2.3 kHz LP)
//      -> Q3 (FUZZ stage: Rc 22k, emitter degeneration set by FUZZ pot VR3)
//      -> C12 (18n, ~70 Hz HP) -> Q4 (Rc 10k / Re 10k, gain ~-1, C13 8n2 =
//              ~1.9 kHz collector LP)
//      -> C14 -> tone network (R28/R29/R30 47k, C15/C16 10n, VR2 100k)
//      -> VR1 100kA volume -> T3 JFET buffer -> Q5 emitter follower -> J2
//
// The schematic draws the FUZZ pot's interaction with C21 as a placeholder
// ("xxk VR3"), so the Fuzz -> gain law below (20x .. 400x) is an estimate.
// The second "2 CH MIXER" branch (T2/Q6 clean path) is not modelled; use Mix.
namespace fz3
{
struct Params
{
    float fuzz   = 0.7f;  // 0..1
    float tone   = 0.5f;  // 0..1 (0 dark, 1 bright)
    float volume = 0.5f;  // 0..1
    float mix    = 1.f;
    float trim   = 1.f;   // linear input trim (guitar level -> volts)
};

class FZ3
{
public:
    void prepare (float fs)
    {
        inHp.setCutoff (60.f, fs);          // C7 / R14 / Q1 input
        jfetLp.setCutoff (18000.f, fs);
        q1Shelf.setCutoff (1300.f, fs);     // R16 12k vs C9 10n + R17 2k2
        q3Lp.setCutoff (2300.f, fs);        // R19 6k8 + C11 10n
        c12Hp.setCutoff (70.f, fs);         // C12 18n into R24A/R25A bias
        q4Lp.setCutoff (1900.f, fs);        // R26A 10k + C13 8n2 to VCC
        toneLo1.setCutoff (340.f, fs);      // R28/R29 47k + C15 10n
        toneLo2.setCutoff (340.f, fs);
        toneHi.setCutoff (340.f, fs);       // C16 10n + R30 47k
        dcHp.setCutoff (20.f, fs);
        reset();
    }
    void reset()
    {
        for (auto* f : { &inHp, &jfetLp, &q1Shelf, &q3Lp, &c12Hp, &q4Lp,
                         &toneLo1, &toneLo2, &toneHi, &dcHp })
            f->reset();
    }

    // Common-emitter stage limits: collector cuts off toward VCC on one half,
    // saturates near ground on the other, with different rails => asymmetric.
    static float rails (float v, float vp, float vn)
    {
        return v >= 0.f ? vp * std::tanh (v / vp) : vn * std::tanh (v / vn);
    }
    static float jfet (float v) { return std::tanh (v * 0.9f + 0.05f) - std::tanh (0.05f); }

    float process (float in, const Params& p)
    {
        const float vin = in * p.trim;

        float x = jfetLp.lp (jfet (inHp.hp (vin)));

        // Q1: gain ~1.2 at low freq, ~-15 dB shelf above 1.3 kHz.
        const float low = q1Shelf.lp (x);
        x = -1.2f * (low + 0.18f * (x - low));

        // Q3 input: 2.3 kHz LP, then base-emitter junction clamps the positive half (~0.6 V).
        x = q3Lp.lp (x);
        x = x >= 0.f ? 0.6f * std::tanh (x / 0.6f) : x;

        // Q3 collector bias sits off-centre => clip points shift (even harmonics).
        constexpr float kBias = 0.9f;
        const float gain = 20.f * std::pow (20.f, p.fuzz);        // 26..52 dB
        float y = -(rails (x * gain + kBias, 4.2f, 2.6f) - rails (kBias, 4.2f, 2.6f));

        // C12 coupling into Q4, gain ~1, collector rail limits and 1.9 kHz LP.
        y = c12Hp.hp (y);
        y = -rails (y * 1.f, 3.2f, 2.0f);
        y = q4Lp.lp (y);

        // Tone network: 2-pole LP path vs 1-pole HP path, crossfaded by VR2.
        const float lo = toneLo2.lp (toneLo1.lp (y));
        const float hi = y - toneHi.lp (y);
        y = 2.f * ((1.f - p.tone) * lo + p.tone * hi);

        // VR1 (audio taper) -> JFET buffer -> Q5 follower -> 1k out.
        y = dcHp.hp (y) * volGain (p.volume);

        const float out = y * 0.3f;                    // volts -> plugin level
        return in * (1.f - p.mix) + out * p.mix;
    }

    static float volGain (float v) { return v <= 0.f ? 0.f : 2.f * v * v; }

private:
    ktg1::OnePole inHp, jfetLp, q1Shelf, q3Lp, c12Hp, q4Lp, toneLo1, toneLo2, toneHi, dcHp;
};
} // namespace fz3
