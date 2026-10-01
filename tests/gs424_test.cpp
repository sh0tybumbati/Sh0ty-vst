#include "PortaStage.h"
#include <cstdio>
#include <vector>

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf ("FAIL: %s\n", msg); ++fails; } else std::printf ("ok:   %s\n", msg); } while (0)
static const double FS = 192000;

static double bin (const std::vector<float>& x, double f)
{
    double re = 0, im = 0;
    for (size_t n = 0; n < x.size(); ++n) { const double w = 2 * 3.14159265358979 * f * n / FS; re += x[n] * std::cos (w); im -= x[n] * std::sin (w); }
    return 2 * std::sqrt (re * re + im * im) / x.size();
}
static std::vector<float> run (float amp, gs424::Params p, double f0 = 1000)
{
    gs424::GS424 k; k.prepare ((float) FS); k.setParams (p); std::vector<float> out;
    for (int n = 0; n < (int) FS; ++n)
    {
        const float y = k.process (amp * (float) std::sin (2 * 3.14159265358979 * f0 * n / FS));
        if (! std::isfinite (y)) { std::printf ("non-finite output\n"); ++fails; return out; }
        if (n >= (int) FS / 2) out.push_back (y);
    }
    return out;
}
static float peak (const std::vector<float>& v) { float m = 0; for (float x : v) m = std::max (m, std::fabs (x)); return m; }
static double h (const std::vector<float>& v, int k) { return bin (v, 1000.0 * k) / bin (v, 1000); }

// small-signal gain of the transistor pair + difference amp, straight from the netlist
static double smallSignalGain (float g1, float g2)
{
    const float rl = gs424::linkResistance (g1), rf = gs424::feedbackResistance (g2);
    const float vd = 1e-4f;
    return 2.0 * rf * gs424::diffPairCurrent (vd, rl, 0.f) / vd;
}

int main()
{
    using namespace gs424;
    // --- the circuit against the schematic ---
    CHECK (std::fabs (smallSignalGain (0.f, 0.f) - 1.63) < 0.1, "GAIN 1 min: stage gain ~1.6x (4 dB) as in the 424");
    CHECK (std::fabs (smallSignalGain (1.f, 0.f) - 373.0) < 15.0, "GAIN 1 max: stage gain ~373x (51 dB) as in the 424");
    CHECK (std::fabs (smallSignalGain (1.f, 1.f) / smallSignalGain (1.f, 0.f) - 10.0) < 0.3, "GAIN 2 max: 10x more gain than stock R211");
    // the differential pair really is a limiter: output current saturates at the tail current
    CHECK (std::fabs (diffPairCurrent (50.f, 44.f, 0.f)) < kIe && std::fabs (diffPairCurrent (50.f, 44.f, 0.f)) > 0.99f * kIe, "diff pair current saturates at the tail current");

    Params p; p.gain2 = 0.f;
    // --- behaviour ---
    p.gain1 = 0.f; auto clean = run (0.05f, p);
    CHECK (h (clean, 3) < 0.03, "gain 1 min: clean");
    p.gain1 = 0.9f; auto hot = run (0.1f, p);
    CHECK (h (hot, 3) > 0.1, "gain 1 high: strong odd harmonics");
    p.gain1 = 0.3f; auto lo = run (0.1f, p); p.gain1 = 0.7f; auto hi = run (0.1f, p);
    CHECK (h (hi, 3) > h (lo, 3), "harmonics rise with gain 1");
    p.gain1 = 0.4f; p.gain2 = 0.f; auto g2lo = run (0.05f, p); p.gain2 = 1.f; auto g2hi = run (0.05f, p);
    CHECK (peak (g2hi) > peak (g2lo) && h (g2hi, 3) > h (g2lo, 3), "gain 2 adds level and distortion");
    { Params q; q.gain1 = 0.6f; q.gain2 = 0.5f; double a = peak (run (0.3f, q)) / peak (run (0.03f, q));
      CHECK (a < 6.0 && a > 1.0, "10x input -> compressed output (diff pair + op-amp rails)"); }
    { Params q; q.gain1 = 0.5f; auto o = run (0.05f, q); double dc = 0; for (float v : o) dc += v;
      CHECK (std::fabs (dc / o.size()) < 0.01, "DC offset small"); }
    { Params q; q.gain1 = 1.f; q.gain2 = 1.f; CHECK (peak (run (10.f, q)) < 2.5f, "bounded with +20 dB over-range input"); }

    // --- Baxandall EQ (clean settings so the EQ is measured, not the distortion) ---
    Params e; e.gain1 = 0.f; e.gain2 = 0.f; e.volume = 0.5f;
    auto eq = [&] (double f, float Params::*knob, float v) { Params q = e; q.*knob = v; return bin (run (0.002f, q, f), f); };
    CHECK (eq (60, &Params::bass, 1.f) > 3.0 * eq (60, &Params::bass, 0.f), "bass: boost vs cut at 60 Hz >> 3x");
    CHECK (eq (8000, &Params::treble, 1.f) > 3.0 * eq (8000, &Params::treble, 0.f), "treble: boost vs cut at 8 kHz >> 3x");
    CHECK (std::fabs (eq (1000, &Params::bass, 1.f) / eq (1000, &Params::bass, 0.f) - 1.0) < 0.5, "bass control leaves the mids alone");
    { const double flat = eq (1000, &Params::bass, 0.5f);
      CHECK (std::fabs (eq (60, &Params::bass, 0.5f) / flat - 1.0) < 0.12 && std::fabs (eq (8000, &Params::treble, 0.5f) / flat - 1.0) < 0.12, "EQ centred is flat"); }

    // --- housekeeping ---
    p = Params(); p.volume = 0.f; CHECK (peak (run (0.2f, p)) == 0.f, "volume=0 silent");
    p = Params(); p.on = false; { auto by = run (0.3f, p); CHECK (std::fabs (peak (by) - 0.3f) < 0.001f, "on=false passes the input through"); }
    p = Params(); { auto d = run (0.1f, p); std::printf ("      default peak level: %.3f\n", peak (d)); CHECK (peak (d) > 0.05f && peak (d) < 2.f, "default settings give a sensible output level"); }

    std::printf (fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
