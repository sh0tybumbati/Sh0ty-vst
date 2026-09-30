#include "TubeStage.h"
#include <cstdio>
#include <vector>

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf ("FAIL: %s\n", msg); ++fails; } else std::printf ("ok:   %s\n", msg); } while (0)
static const double FS = 192000;

static double bin (const std::vector<float>& x, double f)
{
    double re = 0, im = 0;
    for (size_t n = 0; n < x.size(); ++n)
    {
        const double w = 2 * 3.14159265358979 * f * n / FS;
        re += x[n] * std::cos (w); im -= x[n] * std::sin (w);
    }
    return 2 * std::sqrt (re * re + im * im) / x.size();
}

static std::vector<float> run (float amp, ktg1::Params p, double f0 = 1000)
{
    ktg1::KTG1 k; k.prepare ((float) FS); k.setParams (p);
    std::vector<float> out;
    const int N = (int) FS;
    for (int n = 0; n < N; ++n)
    {
        const float y = k.process (amp * (float) std::sin (2 * 3.14159265358979 * f0 * n / FS));
        if (!std::isfinite (y)) { std::printf ("non-finite output\n"); ++fails; return out; }
        if (n >= N / 2) out.push_back (y);
    }
    return out;
}
static float peak (const std::vector<float>& v) { float m = 0; for (float x : v) m = std::max (m, std::fabs (x)); return m; }
static double h (const std::vector<float>& v, int k) { return bin (v, 1000.0 * k) / bin (v, 1000); }

int main()
{
    ktg1::Params p;

    // 1. Channel 1, overdrive down, soft playing: near-clean
    p.od1 = 0.f; p.master1 = 0.5f;
    auto clean = run (0.02f, p);
    CHECK (h (clean, 2) < 0.02 && h (clean, 3) < 0.02, "ch1 od=0: near-clean");

    // 2. Distortion rises with overdrive on both channels
    for (int ch = 0; ch < 2; ++ch)
    {
        p.ch2 = ch == 1;
        (ch ? p.od2 : p.od1) = 0.2f; auto lo = run (0.1f, p);
        (ch ? p.od2 : p.od1) = 0.9f; auto hi = run (0.1f, p);
        CHECK (h (hi, 3) > h (lo, 3), ch ? "ch2: harmonics rise with overdrive" : "ch1: harmonics rise with overdrive");
        CHECK (h (hi, 3) > 0.04, ch ? "ch2 od=0.9: strong odd harmonics" : "ch1 od=0.9: strong odd harmonics");
    }

    // 3. Even harmonics present at moderate drive (asymmetric triodes)
    p.ch2 = false; p.od1 = 0.5f;
    CHECK (h (run (0.1f, p), 2) > 0.01, "ch1 moderate drive: even harmonics present");

    // 4. Channel 2 is hotter than channel 1 at the same knob position
    p.od1 = p.od2 = 0.5f; p.ch2 = false; auto c1 = run (0.05f, p);
    p.ch2 = true; auto c2 = run (0.05f, p);
    CHECK (h (c2, 3) > h (c1, 3), "ch2 dirtier than ch1 at same overdrive");

    // 5. Pull boost adds gain on both channels
    for (int ch = 0; ch < 2; ++ch)
    {
        p.ch2 = ch == 1; p.od1 = p.od2 = 0.3f;
        auto off = run (0.02f, p);
        (ch ? p.boost2 : p.boost1) = true;
        auto on = run (0.02f, p);
        (ch ? p.boost2 : p.boost1) = false;
        CHECK (peak (on) > 1.5f * peak (off), ch ? "ch2 pull boost adds gain" : "ch1 pull boost adds gain");
    }

    // 5b. The boost is a cathode-bypass SHELF: it lifts highs far more than lows
    for (int ch = 0; ch < 2; ++ch)
    {
        p.ch2 = ch == 1; p.od1 = p.od2 = 0.f;
        const double lowOff = bin (run (0.01f, p, 60), 60), highOff = bin (run (0.01f, p, 2000), 2000);
        (ch ? p.boost2 : p.boost1) = true;
        const double lowOn = bin (run (0.01f, p, 60), 60), highOn = bin (run (0.01f, p, 2000), 2000);
        (ch ? p.boost2 : p.boost1) = false;
        CHECK ((highOn / highOff) > 2.0 * (lowOn / lowOff), ch ? "ch2 boost lifts highs more than lows (shelf)" : "ch1 boost lifts highs more than lows (shelf)");
    }

    // 6. Pull crunch squashes dynamics on channel 2
    p.ch2 = true; p.od2 = 0.6f; p.master2 = 0.7f; p.crunch2 = false;
    const float r0 = peak (run (0.3f, p)) / peak (run (0.03f, p));
    p.crunch2 = true;
    const float r1 = peak (run (0.3f, p)) / peak (run (0.03f, p));
    CHECK (r1 < r0, "pull crunch compresses dynamics");
    p.crunch2 = false;

    // 7. Shared tone stack: each band moves its frequency region
    p.ch2 = false; p.od1 = 0.f;
    auto tone = [&] (double f, float ktg1::Params::*knob, float v)
    {
        ktg1::Params q = p; q.*knob = v; return bin (run (0.02f, q, f), f);
    };
    CHECK (tone (80, &ktg1::Params::bass, 1.f) > 2.5 * tone (80, &ktg1::Params::bass, 0.f), "bass knob moves low end");
    CHECK (tone (700, &ktg1::Params::mid, 1.f) > 2.0 * tone (700, &ktg1::Params::mid, 0.f), "mid knob moves mids");
    CHECK (tone (8000, &ktg1::Params::treble, 1.f) > 2.5 * tone (8000, &ktg1::Params::treble, 0.f), "treble knob moves highs");

    // 8. Channel select switches the sound
    p.od1 = 0.2f; p.od2 = 0.9f; p.ch2 = false; auto s1 = run (0.1f, p);
    p.ch2 = true; auto s2 = run (0.1f, p);
    CHECK (std::fabs (h (s1, 3) - h (s2, 3)) > 0.05, "channel selector changes the sound");

    // 9. Master / output / on-off
    p.ch2 = false; p.master1 = 0.f; CHECK (peak (run (0.3f, p)) == 0.f, "master=0 silent");
    p.master1 = 0.5f; p.output = 0.f; CHECK (peak (run (0.3f, p)) == 0.f, "output=0 silent");
    p.output = 0.5f; p.on = false;
    { auto by = run (0.3f, p); CHECK (std::fabs (peak (by) - 0.3f) < 0.001f, "on=false passes the input through"); }
    p.on = true;

    // 10. Safety: no DC, bounded on huge input
    p.od1 = 0.9f; p.ch2 = false;
    auto hot = run (0.3f, p);
    double dc = 0; for (float v : hot) dc += v; dc /= hot.size();
    CHECK (std::fabs (dc) < 0.02, "DC offset small");
    CHECK (peak (run (10.f, p)) < 12.f, "bounded with +30 dB over-range input");

    // 11. Sensible default level
    ktg1::Params d; auto dd = run (0.1f, d);
    std::printf ("      default peak level: %.3f\n", peak (dd));
    CHECK (peak (dd) > 0.05f && peak (dd) < 2.f, "default settings give a sensible output level");

    std::printf (fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
