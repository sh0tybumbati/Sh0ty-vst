#pragma once
#include <algorithm>
#include <cmath>
#include "TubeStage.h"   // ktg1::OnePole
#include "PortaEq.h"     // generated Baxandall shelf tables

// JUCE-free DSP for the GS-424 pedal, modelled from the Tascam 424 MKIII channel input stage
// (MIX PCB 2/4 in the service manual). Voltages: 1.0 sample == 1 V.
//
//   Q201 / Q202 (2SC732)  differential pair, 2.2k collector loads, 4.7k tails (~1.78 mA each),
//                         emitters linked AC-wise by R207 15R + C207 470u + R22 10k rheostat (the TRIM / gain pot)
//   U101B (NJM4565)       difference amplifier, R211 = R212 = 8.2k, C210 470p across R211 (~41 kHz)
//   U202B (NJM4565)       Baxandall HIGH / LOW EQ (100k pots; see tools/gen_baxandall.py)
//
// Small-signal gain from the netlist:  Vo / Vd = 2 * R211 / (2*re + R207 + R22),  re = Vt / I.
// That gives ~4 dB (R22 = 10k) up to ~51 dB (R22 = 0) -- the 424's trim range.
//
// Pedal-isation (not in the 424): GAIN 2 makes R211 / R212 variable (8.2k stock .. 82k), Volume sits after the EQ,
// and the mid EQ band is omitted. Op-amp rails are +/-7.5 V (the 424 runs +/-9 V). The 100R / 1k8 mic-input
// loading is not modelled (assume a buffered guitar input).
namespace gs424
{
constexpr float kVt   = 0.02585f;   // thermal voltage
constexpr float kIe   = 1.78e-3f;   // tail current per transistor
constexpr float kR207 = 15.f;       // fixed part of the emitter link
constexpr float kRf0  = 8.2e3f;     // stock R211 / R212
constexpr float kRail = 7.5f;       // op-amp output swing

struct Params
{
    float gain1 = 0.4f, gain2 = 0.3f, bass = 0.5f, treble = 0.5f, volume = 0.5f;   // 0..1
    float mix = 1.f;
    float trim = 1.f;     // linear input trim
    bool  on = true;
};

// --- circuit helpers (static so tests can check them against the schematic) ---
// Linear-in-dB mapping of the GAIN 1 knob onto the R22 rheostat: gain 4.3 dB .. 51 dB.
inline float stageGainForKnob (float g1) { return 1.63f * std::pow (373.f / 1.63f, std::clamp (g1, 0.f, 1.f)); }
inline float linkResistance (float g1)
{
    const float re2 = 2.f * kVt / kIe;
    const float r22 = 2.f * kRf0 / stageGainForKnob (g1) - re2 - kR207;
    return kR207 + std::max (0.f, r22);
}
inline float feedbackResistance (float g2) { return kRf0 * std::pow (10.f, std::clamp (g2, 0.f, 1.f)); }

// Differential pair with a linking resistor: solve  Vd = Vt ln((I+i)/(I-i)) + i * Rlink  for the link current i.
inline float diffPairCurrent (float vd, float rlink, float guess)
{
    float lo = -kIe * 0.99999f, hi = kIe * 0.99999f;
    float i = std::clamp (guess, lo, hi);
    for (int n = 0; n < 12; ++n)
    {
        const float f = kVt * std::log ((kIe + i) / (kIe - i)) + i * rlink - vd;
        if (f > 0.f) hi = i; else lo = i;
        const float df = kVt * (1.f / (kIe + i) + 1.f / (kIe - i)) + rlink;
        float next = i - f / df;
        if (! (next > lo && next < hi)) next = 0.5f * (lo + hi);   // bracket the Newton step
        if (std::fabs (next - i) < 1e-9f) { i = next; break; }
        i = next;
    }
    return i;
}

// NJM4565 output: linear up to the rail, then a fairly hard knee.
inline float opAmpClip (float x) { const float a = std::fabs (x) / kRail; return x / std::pow (1.f + std::pow (a, 4.f), 0.25f); }

// First-order shelf from an analog prototype, bilinear transform.
struct Shelf1
{
    float b0 = 1.f, b1 = 0.f, a1 = 0.f, z = 0.f;
    void reset() { z = 0.f; }
    float process (float x) { const float y = b0 * x + z; z = b1 * x - a1 * y; return y; }
    // H(s) = hf * (s + wz) / (s + wp)
    void setAnalog (float fs, float wz, float wp, float hf)
    {
        const float k = 2.f * fs, d = k + wp;
        b0 = hf * (k + wz) / d; b1 = hf * (wz - k) / d; a1 = (wp - k) / d;
    }
    void setLow (float fs, float gdb, float fmid)    // DC gain gdb, unity at HF
    {
        const float g = std::pow (10.f, gdb / 20.f), wm = 6.2831853f * fmid;
        setAnalog (fs, wm * std::sqrt (g), wm / std::sqrt (g), 1.f);
    }
    void setHigh (float fs, float gdb, float fmid)   // unity at DC, gain gdb at HF
    {
        const float g = std::pow (10.f, gdb / 20.f), wm = 6.2831853f * fmid;
        setAnalog (fs, wm / std::sqrt (g), wm * std::sqrt (g), g);
    }
};

inline float lutLerp (const float* t, float k)
{
    const float x = std::clamp (k, 0.f, 1.f) * 20.f; const int i = std::min (19, (int) x);
    return t[i] + (t[i + 1] - t[i]) * (x - (float) i);
}

class GS424
{
public:
    void prepare (float oversampledRate)
    {
        fs = oversampledRate;
        inHp.setCutoff (20.f, fs);
        opLp.setCutoff (41300.f, fs);      // R211 8.2k * C210 470p
        outHp.setCutoff (20.f, fs);        // C219 10u coupling
        reset();
        cached = false;
    }
    void reset() { inHp.reset(); opLp.reset(); outHp.reset(); bassSh.reset(); trebSh.reset(); iPrev = 0.f; }

    void setParams (const Params& np)
    {
        if (! cached || np.gain1 != p.gain1) rlink = linkResistance (np.gain1);
        if (! cached || np.gain2 != p.gain2) rf = feedbackResistance (np.gain2);
        if (! cached || np.bass != p.bass)
            bassSh.setLow (fs, lutLerp (portastudio::BaxandallTable::bassGain, np.bass), lutLerp (portastudio::BaxandallTable::bassFreq, np.bass));
        if (! cached || np.treble != p.treble)
            trebSh.setHigh (fs, lutLerp (portastudio::BaxandallTable::trebleGain, np.treble), lutLerp (portastudio::BaxandallTable::trebleFreq, np.treble));
        p = np; cached = true;
    }

    float process (float in)
    {
        if (! p.on) return in;
        const float v = inHp.hp (in * p.trim);

        iPrev = diffPairCurrent (v, rlink, iPrev);                // transistor pair
        float y = opLp.lp (2.f * rf * iPrev);                     // difference amp: Vo = 2 Rf i
        y = opAmpClip (y);                                        // op-amp rails
        y = trebSh.process (bassSh.process (y));                  // Baxandall bass / treble
        return outHp.hp (y) * (1.f / kRail) * volGain (p.volume);
    }

    static float volGain (float v) { return v <= 0.f ? 0.f : 2.f * v * v; }

private:
    float fs = 48000.f, rlink = 100.f, rf = kRf0, iPrev = 0.f;
    bool cached = false;
    Params p;
    ktg1::OnePole inHp, opLp, outHp;
    Shelf1 bassSh, trebSh;
};
} // namespace gs424
