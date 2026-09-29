#include "TubeStage.h"
#include <cstdio>
#include <vector>

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf ("FAIL: %s\n", msg); ++fails; } else std::printf ("ok:   %s\n", msg); } while (0)

// magnitude of a single DFT bin (freq f) over a window
static double bin (const std::vector<float>& x, double f, double fs)
{
    double re = 0, im = 0;
    for (size_t n = 0; n < x.size(); ++n)
    {
        const double w = 2 * 3.14159265358979 * f * n / fs;
        re += x[n] * std::cos (w); im -= x[n] * std::sin (w);
    }
    return 2 * std::sqrt (re * re + im * im) / x.size();
}

static std::vector<float> run (float amp, ktg1::Params p, double fs = 192000, double f0 = 1000)
{
    ktg1::KTG1 k; k.prepare ((float) fs);
    std::vector<float> out;
    const int N = (int) fs;                       // 1 s; analyse the last 0.5 s (integer cycles)
    for (int n = 0; n < N; ++n)
    {
        const float y = k.process (amp * (float) std::sin (2 * 3.14159265358979 * f0 * n / fs), p);
        if (n >= N / 2) out.push_back (y);
        if (!std::isfinite (y)) { std::printf ("non-finite output\n"); ++fails; return out; }
    }
    return out;
}

int main()
{
    const double fs = 192000;
    ktg1::Params p; p.mix = 1.f;

    // 1. Low drive is near-clean: little 2nd/3rd harmonic
    p.drive = 0.f; p.tone = 0.5f;
    auto clean = run (0.05f, p);
    CHECK (bin (clean, 2000, fs) / bin (clean, 1000, fs) < 0.02, "drive=0: 2nd harmonic < 2%");

    // 2. High drive produces both even and odd harmonics (asymmetric tube)
    p.drive = 0.9f;
    auto hot = run (0.3f, p);
    const double f1 = bin (hot, 1000, fs);
    CHECK (bin (hot, 2000, fs) / f1 > 0.02, "drive=0.9: even (2nd) harmonic present");
    CHECK (bin (hot, 3000, fs) / f1 > 0.02, "drive=0.9: odd (3rd) harmonic present");

    // 3. More drive => more distortion
    p.drive = 0.3f;
    auto mid = run (0.3f, p);
    CHECK (bin (hot, 3000, fs) / f1 > bin (mid, 3000, fs) / bin (mid, 1000, fs), "harmonics rise with drive");

    // 4. No DC at output
    double dc = 0; for (float v : hot) dc += v; dc /= hot.size();
    CHECK (std::fabs (dc) < 0.01, "DC offset < 0.01");

    // 5. Output bounded even with huge input
    auto huge = run (10.f, p);
    float pk = 0; for (float v : huge) pk = std::max (pk, std::fabs (v));
    CHECK (pk < 8.f, "output bounded with +20 dB over-range input");

    // 6. Level control: 0 = silent
    p.level = 0.f;
    auto silent = run (0.3f, p);
    float pk0 = 0; for (float v : silent) pk0 = std::max (pk0, std::fabs (v));
    CHECK (pk0 == 0.f, "level=0 is silent");

    // 7. Tone: high tone has more 8 kHz than low tone
    p.level = 0.6f; p.drive = 0.f;
    p.tone = 0.f;  auto dark   = run (0.1f, p, fs, 8000);
    p.tone = 1.f;  auto bright = run (0.1f, p, fs, 8000);
    CHECK (bin (bright, 8000, fs) > 1.5 * bin (dark, 8000, fs), "tone up brightens");

    std::printf (fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
