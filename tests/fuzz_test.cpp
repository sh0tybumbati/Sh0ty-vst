#include "FuzzStage.h"
#include <cstdio>
#include <vector>
static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf ("FAIL: %s\n", msg); ++fails; } else std::printf ("ok:   %s\n", msg); } while (0)

static double bin (const std::vector<float>& x, double f, double fs)
{
    double re = 0, im = 0;
    for (size_t n = 0; n < x.size(); ++n) { const double w = 2 * 3.14159265358979 * f * n / fs; re += x[n] * std::cos (w); im -= x[n] * std::sin (w); }
    return 2 * std::sqrt (re * re + im * im) / x.size();
}
static std::vector<float> run (float amp, fz3::Params p, double f0 = 1000, double fs = 192000)
{
    fz3::FZ3 k; k.prepare ((float) fs); std::vector<float> out;
    for (int n = 0; n < (int) fs; ++n)
    {
        const float y = k.process (amp * (float) std::sin (2 * 3.14159265358979 * f0 * n / fs), p);
        if (!std::isfinite (y)) { std::printf ("non-finite\n"); ++fails; return out; }
        if (n >= (int) fs / 2) out.push_back (y);
    }
    return out;
}
static float peak (const std::vector<float>& v) { float m = 0; for (float x : v) m = std::max (m, std::fabs (x)); return m; }

int main()
{
    const double fs = 192000;
    fz3::Params p; p.tone = 0.5f; p.volume = 0.5f;

    p.fuzz = 1.f;
    auto hot = run (0.3f, p);
    CHECK (bin (hot, 3000, fs) / bin (hot, 1000, fs) > 0.05, "fuzz=1 produces strong odd harmonics");
    CHECK (bin (hot, 2000, fs) / bin (hot, 1000, fs) > 0.02, "fuzz=1 produces even harmonics (asymmetric)");

    // Compression: 20 dB more input gives far less than 20 dB more output
    auto lo = run (0.03f, p), hi = run (0.3f, p);
    CHECK (peak (hi) / peak (lo) < 3.f, "heavy compression/sustain (10x input -> <3x output)");

    p.fuzz = 0.1f;
    auto mild = run (0.3f, p);
    p.fuzz = 1.f;
    CHECK (bin (hot, 3000, fs) / bin (hot, 1000, fs) > bin (mild, 3000, fs) / bin (mild, 1000, fs), "harmonics rise with fuzz");

    double dc = 0; for (float v : hot) dc += v; dc /= hot.size();
    CHECK (std::fabs (dc) < 0.02, "DC offset small");
    CHECK (peak (run (10.f, p)) < 10.f, "bounded with huge input");
    p.volume = 0.f; CHECK (peak (run (0.3f, p)) == 0.f, "volume=0 silent");
    p.volume = 0.5f; p.tone = 0.f; auto dark = run (0.1f, p, 4000);
    p.tone = 1.f; auto bright = run (0.1f, p, 4000);
    CHECK (bin (bright, 4000, fs) > bin (dark, 4000, fs), "tone up brightens");

    std::printf (fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
