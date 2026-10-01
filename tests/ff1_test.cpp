#include "FuzzFaceStage.h"
#include <cstdio>
#include <vector>

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf ("FAIL: %s\n", msg); ++fails; } else std::printf ("ok:   %s\n", msg); } while (0)
static const double FS = 96000;     // the plugin runs this stage at 2x oversampling

static double bin (const std::vector<float>& x, double f)
{
    double re = 0, im = 0;
    for (size_t n = 0; n < x.size(); ++n) { const double w = 2 * 3.14159265358979 * f * n / FS; re += x[n] * std::cos (w); im -= x[n] * std::sin (w); }
    return 2 * std::sqrt (re * re + im * im) / x.size();
}
static std::vector<float> run (float amp, ff1::Params p, double f0 = 220, int* maxIters = nullptr)
{
    ff1::FF1 d; d.prepare ((float) FS); d.setParams (p); d.reset();
    std::vector<float> out; int mx = 0;
    for (int n = 0; n < (int) FS; ++n)
    {
        const float y = d.process (amp * (float) std::sin (2 * 3.14159265358979 * f0 * n / FS));
        if (! std::isfinite (y)) { std::printf ("non-finite output\n"); ++fails; return out; }
        mx = std::max (mx, d.lastIterations());
        if (n >= (int) FS / 2) out.push_back (y);
    }
    if (maxIters) *maxIters = mx;
    return out;
}
static float peak (const std::vector<float>& v) { float m = 0; for (float x : v) m = std::max (m, std::fabs (x)); return m; }
static double h (const std::vector<float>& v, int k) { return bin (v, 220.0 * k) / bin (v, 220); }

int main()
{
    ff1::Params p; p.volume = 1.f;

    // --- DC bias against the textbook values for this circuit (positive ground, 9 V) ---
    for (bool si : { false, true })
    {
        ff1::FF1 d; d.prepare ((float) FS); p.silicon = si; p.fuzz = 0.5f; d.setParams (p); d.reset();
        std::printf ("      %s bias: Q1 coll %.2f V, Q2 emit %.2f V, Q2 coll %.2f V\n", si ? "Si" : "Ge", d.nodeVoltage (1), d.nodeVoltage (2), d.nodeVoltage (3));
        if (! si)
        {
            CHECK (d.nodeVoltage (1) < -0.4 && d.nodeVoltage (1) > -0.9, "Ge: Q1 collector sits around -0.5 .. -0.9 V");
            CHECK (d.nodeVoltage (3) < -4.0 && d.nodeVoltage (3) > -5.6, "Ge: Q2 collector sits around -4.5 V (half the battery)");
            CHECK (d.nodeVoltage (0) > -0.3 && d.nodeVoltage (0) < -0.05, "Ge: Q1 base is about one germanium Vbe (~0.16 V) from its emitter");
        }
        else
        {
            CHECK (d.nodeVoltage (0) < -0.5 && d.nodeVoltage (0) > -0.7, "Si: Q1 base is about one silicon Vbe (~0.6 V) from its emitter");
            CHECK (d.nodeVoltage (3) < -2.0 && d.nodeVoltage (3) > -4.5, "Si: Q2 collector biased mid-supply-ish with the trimmer");
        }
    }
    {   // the fuzz pot only moves the AC path, not the DC bias
        ff1::FF1 a, b; a.prepare ((float) FS); b.prepare ((float) FS); p.silicon = false;
        p.fuzz = 0.1f; a.setParams (p); a.reset(); p.fuzz = 0.9f; b.setParams (p); b.reset();
        CHECK (std::fabs (a.nodeVoltage (3) - b.nodeVoltage (3)) < 0.02, "fuzz knob does not change the DC bias");
    }

    // --- behaviour ---
    p.silicon = false;
    p.fuzz = 0.2f; auto quiet = run (0.03f, p);
    CHECK (h (quiet, 3) < 0.03 && h (quiet, 2) < 0.03, "low fuzz + quiet playing: nearly clean");
    p.fuzz = 1.f;  auto full = run (0.03f, p);
    CHECK (h (full, 3) > 0.15, "full fuzz: strong odd harmonics even from a quiet signal");
    CHECK (h (full, 3) > h (quiet, 3), "more fuzz => more distortion");

    // compression / sustain: 13x more input gives far less than 13x more output
    p.fuzz = 0.8f;
    { const float a = peak (run (0.03f, p)), b = peak (run (0.4f, p)); CHECK (b / a < 3.0f, "sustain: 13x more input -> under 3x more output"); }

    // germanium vs silicon really are different circuits
    p.fuzz = 0.6f; p.silicon = false; auto ge = run (0.03f, p); p.silicon = true; auto si = run (0.03f, p);
    std::printf ("      at fuzz 0.6, input 0.03:  Ge H2 %.3f H3 %.3f   |   Si H2 %.3f H3 %.3f\n", h (ge, 2), h (ge, 3), h (si, 2), h (si, 3));
    CHECK (h (si, 2) > 3.0 * h (ge, 2), "silicon is much richer in even harmonics than germanium at the same settings");

    // --- safety ---
    for (bool s : { false, true })
    {
        p.silicon = s; p.fuzz = 1.f;
        int mx = 0; auto huge = run (5.f, p, 220, &mx);
        CHECK (peak (huge) < 3.f, s ? "Si: bounded with a +34 dB over-range input" : "Ge: bounded with a +34 dB over-range input");
        CHECK (mx <= 12, s ? "Si: the solver converges within its iteration limit" : "Ge: the solver converges within its iteration limit");
        double dc = 0; for (float v : huge) dc += v; CHECK (std::fabs (dc / huge.size()) < 0.02, s ? "Si: no DC at the output" : "Ge: no DC at the output");
    }
    p.silicon = false; p.fuzz = 0.5f; p.volume = 0.f; CHECK (peak (run (0.2f, p)) == 0.f, "volume = 0 is silent");
    ff1::Params d0; { auto o = run (0.1f, d0); std::printf ("      default peak level: %.3f\n", peak (o)); CHECK (peak (o) > 0.05f && peak (o) < 1.5f, "default settings give a sensible output level"); }
    p = ff1::Params(); p.on = false; { auto by = run (0.3f, p); CHECK (std::fabs (peak (by) - 0.3f) < 0.001f, "on = false passes the input through"); }

    // --- switching transistors while playing ---
    {
        ff1::FF1 d; d.prepare ((float) FS); p = ff1::Params(); p.volume = 1.f; d.setParams (p); d.reset();
        bool finite = true; float last = 0.f;
        for (int n = 0; n < (int) FS; ++n)
        {
            if (n == (int) FS / 4) { p.silicon = true; d.setParams (p); }
            if (n == (int) FS * 5 / 8) { p.silicon = false; d.setParams (p); }
            const float y = d.process (0.1f * (float) std::sin (2 * 3.14159265358979 * 220 * n / FS));
            if (! std::isfinite (y)) finite = false;
            if (n > (int) FS * 7 / 8) last = std::max (last, std::fabs (y));
        }
        CHECK (finite, "switching Ge <-> Si while playing stays finite");
        CHECK (last > 0.05f, "...and the sound comes back after the switch");
    }

    std::printf (fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
