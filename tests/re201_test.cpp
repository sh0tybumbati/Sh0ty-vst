#include "SpaceEchoStage.h"
#include <chrono>
#include <cstdio>
#include <vector>

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf ("FAIL: %s\n", msg); ++fails; } else std::printf ("ok:   %s\n", msg); } while (0)
static const double FS = 48000;

static re201::Params base()
{
    re201::Params p; p.wow = 0.f; p.repeatRate = 0.5f; p.intensity = 0.f; p.echoVol = 1.f; p.reverbVol = 1.f;
    p.bass = p.treble = 0.5f; return p;
}
static std::vector<float> impulse (re201::Params p, double seconds, float amp = 0.3f)
{
    re201::SpaceEcho e; e.prepare (FS); e.setParams (p); e.reset();
    std::vector<float> y; const int N = (int) (FS * seconds);
    for (int n = 0; n < N; ++n) { y.push_back (e.process (n == 0 ? amp : 0.f)); if (! std::isfinite (y.back())) { std::printf ("non-finite\n"); ++fails; return y; } }
    return y;
}
static std::vector<int> peaks (const std::vector<float>& y, float thresh, int guard)   // sample index of each local maximum above thresh
{
    std::vector<int> r; int last = -guard * 2;
    for (size_t n = 1; n + 1 < y.size(); ++n)
    {
        const float a = std::fabs (y[n]);
        if (a > thresh && a >= std::fabs (y[n - 1]) && a >= std::fabs (y[n + 1]))
        {
            if ((int) n - last < guard) { if (a > std::fabs (y[(size_t) r.back()])) r.back() = (int) n; }
            else r.push_back ((int) n);
            last = (int) n;
        }
    }
    return r;
}
static double rms (const std::vector<float>& y, double t0, double t1)
{
    double s = 0; size_t a = (size_t) (t0 * FS), b = std::min (y.size(), (size_t) (t1 * FS));
    for (size_t i = a; i < b; ++i) s += y[i] * y[i];
    return std::sqrt (s / std::max<size_t> (1, b - a));
}
static double sineGain (re201::Params p, double f, int mode)
{
    p.mode = mode; re201::SpaceEcho e; e.prepare (FS); e.setParams (p); e.reset();
    double acc = 0; int cnt = 0; const int N = (int) (FS * 2.5);
    for (int n = 0; n < N; ++n)
    {
        const float y = e.process (0.05f * (float) std::sin (6.28318530718 * f * n / FS));
        if (n > (int) (FS * 1.5)) { acc += y * y; ++cnt; }
    }
    return std::sqrt (acc / cnt) / (0.05 / std::sqrt (2.0));
}
static double db (double x) { return 20 * std::log10 (std::max (x, 1e-12)); }

int main()
{
    // --- head timing: heads sit at 1x / 2x / 3x the head-1 delay ---
    {
        auto p = base(); const double t1 = re201::headOneDelay (0.5f);
        for (int m : { 1, 2, 3 })
        {
            p.mode = m; auto y = impulse (p, 1.4);
            auto pk = peaks (y, 0.02f, 400);
            const double want = t1 * re201::kHeadRatio[m - 1];
            std::printf ("      mode %d: %zu echo(es), first at %.1f ms (want %.1f)\n", m, pk.size(), pk.empty() ? 0.0 : pk[0] * 1000 / FS, want * 1000);
            CHECK (pk.size() == 1 && std::fabs (pk[0] / FS - want) < 0.003, "single-head modes: one echo at the right head position");
        }
        p.mode = 11; p.reverbVol = 0.f; auto y = impulse (p, 1.4); auto pk = peaks (y, 0.02f, 400);
        CHECK (pk.size() == 3, "mode 11 (all heads, reverb off) gives three echoes");
        if (pk.size() == 3) CHECK (std::fabs ((pk[1] - pk[0]) / FS - t1) < 0.003 && std::fabs ((pk[2] - pk[1]) / FS - t1) < 0.003, "the three echoes are evenly spaced (1x, 2x, 3x)");
        p.mode = 4; pk = peaks (impulse (p, 1.4), 0.02f, 400); CHECK (pk.size() == 2, "mode 4 (heads 1 + 2) gives two echoes");
    }
    // --- REPEAT RATE changes the spacing ---
    {
        auto p = base(); p.mode = 3; p.repeatRate = 0.1f; auto slow = peaks (impulse (p, 1.4), 0.02f, 400);
        p.repeatRate = 0.9f; auto fast = peaks (impulse (p, 1.4), 0.02f, 400);
        CHECK (! slow.empty() && ! fast.empty() && slow[0] > 3 * fast[0] / 2, "higher REPEAT RATE = shorter delay");
    }
    // --- INTENSITY: repeats ---
    {
        auto p = base(); p.mode = 1;
        p.intensity = 0.f;  auto y0 = impulse (p, 2.0);
        p.intensity = 0.5f; auto y1 = impulse (p, 2.0);
        p.intensity = 0.9f; auto y2 = impulse (p, 2.0);
        CHECK (peaks (y0, 0.01f, 400).size() == 1, "INTENSITY 0: a single echo, no repeats");
        const auto p1 = peaks (y1, 0.004f, 400), p2 = peaks (y2, 0.004f, 400);
        CHECK (p1.size() >= 3, "INTENSITY 0.5: repeats");
        CHECK (p2.size() > p1.size(), "more INTENSITY = more repeats");
        if (p1.size() >= 3) CHECK (std::fabs (y1[(size_t) p1[1]]) < std::fabs (y1[(size_t) p1[0]]), "repeats decay at moderate INTENSITY");
        // the tape loop rolls the highs off a little more on each pass
        std::vector<float> n1, n2;
        auto bright = [&] (const std::vector<float>& y, int centre) { double hf = 0, tot = 0; for (int i = centre - 200; i < centre + 200; ++i) { const double d = y[(size_t) i] - y[(size_t) i - 1]; hf += d * d; tot += y[(size_t) i] * y[(size_t) i]; } return hf / std::max (tot, 1e-12); };
        if (p2.size() >= 3) CHECK (bright (y2, p2[2]) < bright (y2, p2[0]), "each repeat is duller than the last");
    }
    // --- runaway is bounded, no NaN, no DC ---
    {
        auto p = base(); p.mode = 11; p.intensity = 1.f; p.echoVol = 1.f; p.reverbVol = 1.f; p.wow = 1.f;
        re201::SpaceEcho e; e.prepare (FS); e.setParams (p); e.reset();
        float mx = 0; double dc = 0; unsigned s = 7;
        for (int n = 0; n < (int) (FS * 20); ++n)
        {
            s = s * 1664525u + 1013904223u;
            const float x = n < (int) FS * 2 ? 0.5f * ((float) (s >> 8) / 8388608.f - 1.f) : 0.f;
            const float y = e.process (x);
            if (! std::isfinite (y)) { std::printf ("non-finite at %d\n", n); ++fails; break; }
            mx = std::max (mx, std::fabs (y));
            if (n > (int) (FS * 18)) dc += y;
        }
        std::printf ("      INTENSITY 10 for 20 s: peak %.2f, mean %.5f\n", mx, dc / (FS * 2));
        CHECK (mx < 6.f, "self-oscillation stays bounded (tape saturation)");
        CHECK (std::fabs (dc / (FS * 2)) < 0.01, "no DC build-up");
    }
    // --- spring reverb ---
    {
        auto p = base(); p.mode = 12; auto y = impulse (p, 3.0);
        const double a = rms (y, 0.0, 0.25), b = rms (y, 0.5, 0.75), c = rms (y, 1.5, 1.75), d = rms (y, 2.6, 2.85);
        std::printf ("      reverb rms: %.5f %.5f %.5f %.5f\n", a, b, c, d);
        CHECK (a > 1e-4 && b > 1e-5, "reverb only: a tail exists");
        CHECK (a > b && b > c && c > d, "the tail decays");
        CHECK (db (c) - db (a) < -6, "tail drops at least 6 dB by 1.5 s");
        auto q = base(); q.mode = 12; q.reverbVol = 0.f; auto z = impulse (q, 1.0);
        CHECK (rms (z, 0, 1.0) < 1e-6, "REVERB VOLUME 0 = silent");
        auto m = base(); m.mode = 3; m.reverbVol = 1.f; auto e1 = impulse (m, 1.0);   // echo-only mode must have no reverb
        CHECK (rms (e1, 0.0, 0.08) < 1e-5, "echo-only mode: no reverb before the first echo");
        auto r = base(); r.mode = 12; r.echoVol = 1.f; r.intensity = 0.8f; auto rr = impulse (r, 2.0);
        CHECK (peaks (rr, 0.05f, 400).empty(), "reverb only: no tape echoes");
    }
    // --- foot switch and power ---
    {
        auto p = base(); p.mode = 3; p.intensity = 0.8f;
        re201::SpaceEcho e; e.prepare (FS); e.setParams (p); e.reset();
        for (int n = 0; n < (int) FS; ++n) e.process (0.3f * (float) std::sin (6.28318 * 220 * n / FS));
        p.echoCancel = true; e.setParams (p);
        for (int n = 0; n < (int) (FS * 0.3); ++n) e.process (0.f);
        double s = 0; for (int n = 0; n < (int) (FS * 0.5); ++n) { const float y = e.process (0.f); s += y * y; }
        CHECK (std::sqrt (s / (FS * 0.5)) < 1e-4, "ECHO CANCEL silences the echo and its repeats");
        p.echoCancel = false; p.on = false; e.setParams (p);
        for (int n = 0; n < (int) (FS * 0.3); ++n) e.process (0.f);
        s = 0; for (int n = 0; n < (int) (FS * 0.3); ++n) { const float y = e.process (0.3f); s += y * y; }
        CHECK (std::sqrt (s / (FS * 0.3)) < 1e-3, "power off: no echo");
    }
    // --- tone controls and tape bandwidth ---
    {
        auto p = base(); p.mode = 3; p.reverbVol = 0.f;
        const double flat = sineGain (p, 90, 3);
        p.bass = 1.f;   const double bassUp = sineGain (p, 90, 3);   p.bass = 0.f;   const double bassDn = sineGain (p, 90, 3);
        p.bass = 0.5f;
        const double trf = sineGain (p, 5000, 3);
        p.treble = 1.f; const double trUp = sineGain (p, 5000, 3);   p.treble = 0.f; const double trDn = sineGain (p, 5000, 3);
        std::printf ("      bass %+.1f / %+.1f dB, treble %+.1f / %+.1f dB\n", db (bassUp / flat), db (bassDn / flat), db (trUp / trf), db (trDn / trf));
        CHECK (db (bassUp / flat) > 6 && db (bassDn / flat) < -6, "BASS boosts and cuts the low end");
        CHECK (db (trUp / trf) > 6 && db (trDn / trf) < -6, "TREBLE boosts and cuts the high end");
        p.treble = 0.5f;
        const double lo = sineGain (p, 1000, 3), hi = sineGain (p, 12000, 3);
        std::printf ("      tape bandwidth: 12 kHz is %.1f dB vs 1 kHz\n", db (hi / lo));
        CHECK (db (hi / lo) < -12, "the tape loop rolls off the top end");
    }
    // --- lamps and meter ---
    {
        auto p = base(); re201::SpaceEcho e; e.prepare (FS); e.setParams (p); e.reset();
        for (int n = 0; n < 4800; ++n) e.process (0.05f * (float) std::sin (6.28318 * 440 * n / FS));
        const bool quiet = e.peakLamp() < 0.5f; const float v1 = e.vuLevel();
        for (int n = 0; n < 4800; ++n) e.process (1.5f * (float) std::sin (6.28318 * 440 * n / FS));
        CHECK (quiet && e.peakLamp() > 0.5f && e.vuLevel() > v1 * 4, "VU follows the level and the PEAK lamp lights on a hot signal");
    }
    // --- CPU ---
    {
        auto p = base(); p.mode = 11; p.intensity = 0.5f; p.wow = 1.f;
        re201::SpaceEcho e; e.prepare (FS); e.setParams (p); e.reset();
        const auto t0 = std::chrono::steady_clock::now(); float sink = 0;
        for (int n = 0; n < (int) FS * 10; ++n) sink += e.process (0.1f * (float) std::sin (0.05 * n));
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
        std::printf ("      10 s of audio in %.0f ms (%.1f ms per second)  [%g]\n", ms, ms / 10, sink * 0.f);
        CHECK (ms < 3000, "runs comfortably faster than real time");
    }
    std::printf (fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
