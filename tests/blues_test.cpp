#include "BluesStage.h"
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
static std::vector<float> run (float amp, bd2::Params p, double f0 = 1000, double fs = 192000)
{
    bd2::BD2 k; k.prepare ((float) fs); std::vector<float> out;
    for (int n = 0; n < (int) fs; ++n)
    {
        const float y = k.process (amp * (float) std::sin (2 * 3.14159265358979 * f0 * n / fs), p);
        if (!std::isfinite (y)) { std::printf ("non-finite\n"); ++fails; return out; }
        if (n >= (int) fs / 2) out.push_back (y);
    }
    return out;
}
static float peak (const std::vector<float>& v) { float m = 0; for (float x : v) m = std::max (m, std::fabs (x)); return m; }
static double h (const std::vector<float>& v, int k) { return bin (v, 1000.0 * k, 192000) / bin (v, 1000, 192000); }

int main()
{
    bd2::Params p; p.tone = 0.5f; p.level = 0.5f;

    p.gain = 0.f;
    auto lowG = run (0.03f, p);
    CHECK (h (lowG, 3) < 0.03, "gain=0, soft playing: nearly clean (3rd < 3%)");

    p.gain = 1.f;
    auto hot = run (0.2f, p);
    CHECK (h (hot, 3) > 0.05, "gain=1: strong odd harmonics");

    p.gain = 0.5f;
    auto mid = run (0.05f, p);
    CHECK (h (mid, 2) > 0.005, "gain=0.5: even harmonics present (asymmetric PNP stages)");

    p.gain = 0.2f; auto g2 = run (0.2f, p);
    p.gain = 0.8f; auto g8 = run (0.2f, p);
    CHECK (h (g8, 3) > h (g2, 3), "harmonics rise with gain");

    // Dynamic response: quiet vs loud playing (touch sensitivity, but still compresses)
    p.gain = 0.6f;
    auto q = run (0.02f, p), l = run (0.2f, p);
    CHECK (peak (l) / peak (q) < 8.f && peak (l) / peak (q) > 1.5f, "10x input -> 1.5x..8x output (dynamic but compressed)");

    double dc = 0; for (float v : hot) dc += v; dc /= hot.size();
    CHECK (std::fabs (dc) < 0.02, "DC offset small");
    CHECK (peak (run (10.f, p)) < 10.f, "bounded with huge input");
    p.level = 0.f; CHECK (peak (run (0.3f, p)) == 0.f, "level=0 silent");

    p.level = 0.5f; p.gain = 0.f; p.tone = 0.f; auto dark = run (0.02f, p, 6000);
    p.tone = 1.f; auto bright = run (0.02f, p, 6000);
    CHECK (bin (bright, 6000, 192000) > 2.0 * bin (dark, 6000, 192000), "tone up brightens");
    p.tone = 0.f; auto bassy = run (0.02f, p, 150);
    p.tone = 1.f; auto thin = run (0.02f, p, 150);
    CHECK (bin (bassy, 150, 192000) > 1.5 * bin (thin, 150, 192000), "tone down adds bass");

    std::printf (fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
