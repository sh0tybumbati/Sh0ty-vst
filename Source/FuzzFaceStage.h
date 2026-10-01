#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include "TubeStage.h"   // ktg1::OnePole

// JUCE-free DSP for the FF-1: a two-transistor PNP fuzz with the classic feedback loop, solved as a real circuit.
//
// Netlist (positive ground, 9 V battery, read from the "Fuzz Face PNP Ge" schematic):
//   Q1: emitter = ground, base = input (2.2u coupling), collector -> R 33k -> A (-9 V) and -> Q2 base
//   Q2: emitter = E2, collector -> R 8k2 -> B, B -> R 470 -> A (supply node shared by both stages)
//   R 100k feeds Q2's emitter (E2) back to Q1's base; FUZZ pot 1k from E2 to ground, wiper -> 20u -> ground
//   Output: B -> 0.01u -> VOLUME pot 500k (audio taper)
// Each sample: Newton-Raphson on the 6 node voltages with Ebers-Moll PNP transistors and trapezoidal capacitors.
// Germanium vs silicon is just a different transistor parameter set (saturation currents, leakage, gain).
//
// Simplifications: ideal battery (no sag), the source is a plain resistor (a real pickup is also inductive), the
// 500k Volume pot's loading of the output node is ignored, no temperature drift.
namespace ff1
{
struct Transistor { double is, isbc, bf, nvt, rfb; };   // rfb: the 100k feedback resistor (silicon sets use a bias trimmer)
inline Transistor germanium() { return { 5.0e-7, 6.0e-7, 65.0, 0.02585, 100e3 }; }   // NKT275 / AC128 class, a little leaky
inline Transistor silicon()   { return { 1.0e-14, 1.0e-13, 200.0, 0.02585, 68e3 }; }

struct Params
{
    float fuzz = 0.8f, volume = 0.7f;
    float trim = 1.f;          // linear input trim
    bool  silicon = false;     // false = germanium
    bool  on = true;
};

class FF1
{
public:
    using Vec4 = std::array<double, 4>;      // unknowns: Q1 base, Q1 collector / Q2 base, Q2 emitter, Q2 collector

    void prepare (float oversampledRate)
    {
        fs = oversampledRate; dt = 1.0 / (double) fs;
        outHp.setCutoff (31.8f, fs);        // 0.01u into the 500k volume pot
        setBandwidth (false);
        setTransistor (false, true);
        setFuzz (0.8f);
    }

    void reset() { solveDc(); outHp.reset(); outLp.reset(); fade = 1.f; }

    void setParams (const Params& p)
    {
        if (p.fuzz != fuzzKnob) setFuzz (p.fuzz);
        if (p.silicon != isSilicon) { setTransistor (p.silicon, false); fade = 0.f; }   // fade back in after re-biasing
        prm = p;
    }

    void setCustomTransistor (const Transistor& t) { q = t; solveDc(); }   // tests / tuning
    double nodeVoltage (int i) const                                      // for tests: 0..3 = the solved nodes, 4 = supply node B, 5 = fuzz-pot wiper
    {
        if (i < 4) return x[(size_t) i];
        if (i == 4) return -9.0 + (x[3] + 9.0) * 470.0 / (8.2e3 + 470.0);
        return v5prev;
    }
    int lastIterations() const { return lastIters; }

    float process (float in)
    {
        if (! prm.on) return in;
        step ((double) in * (double) prm.trim);
        const double vB = -9.0 + (x[3] + 9.0) * 470.0 / (8.2e3 + 470.0);   // the supply node B, where the output cap is tapped
        float y = outLp.lp (outHp.hp ((float) vB));
        fade = std::min (1.f, fade + fadeStep);
        return y * 2.4f * fade * volGain (prm.volume);
    }

    static float volGain (float v) { return v <= 0.f ? 0.f : v * v; }   // 500K audio taper, roughly

private:
    // Stand-in for the transistors' limited bandwidth (junction capacitance); not in the schematic.
    void setBandwidth (bool si) { outLp.setCutoff (si ? 18000.f : 11000.f, fs); }

    void setFuzz (float f)
    {
        fuzzKnob = std::clamp (f, 0.f, 1.f);
        rtw = std::max (0.5, (1.0 - (double) fuzzKnob) * 1000.0);   // pot top -> wiper (the AC emitter degeneration)
        rwg = std::max (0.5, (double) fuzzKnob * 1000.0);           // wiper -> ground
    }

    void setTransistor (bool si, bool silentInit)
    {
        isSilicon = si; q = si ? silicon() : germanium();
        solveDc();
        setBandwidth (si);
        fadeStep = (float) (1.0 / (0.02 * (double) fs));              // 20 ms
        if (silentInit) fade = 1.f;
    }

    // ---- Ebers-Moll PNP, returns currents and partial derivatives ----
    struct Q
    {
        double icOut, ibOut, ieIn;                          // current leaving the collector / base terminals, entering the emitter
        double dIc[3], dIb[3], dIe[3];                      // d/d(vE, vB, vC)
    };
    Q evalQ (double vE, double vB, double vC) const
    {
        const double u = (vE - vB) / q.nvt, w = (vC - vB) / q.nvt;
        const double eu = std::exp (std::clamp (u, -60.0, 40.0)), ew = std::exp (std::clamp (w, -60.0, 40.0));
        const double dEu = eu / q.nvt, dEw = ew / q.nvt;
        const double icc = q.is * (eu - ew), ibe = q.is / q.bf * (eu - 1.0), ibc = q.isbc * (ew - 1.0);
        const double gbe = q.is / q.bf * dEu, gbc = q.isbc * dEw, gcu = q.is * dEu, gcw = q.is * dEw;
        Q r;
        r.icOut = icc - ibc;   r.ibOut = ibe + ibc;   r.ieIn = icc + ibe;
        r.dIc[0] = gcu;                r.dIc[1] = -gcu + gcw + gbc;   r.dIc[2] = -gcw - gbc;
        r.dIb[0] = gbe;                r.dIb[1] = -gbe - gbc;         r.dIb[2] = gbc;
        r.dIe[0] = gcu + gbe;          r.dIe[1] = -gcu - gbe + gcw;   r.dIe[2] = -gcw;
        return r;
    }

    // ---- one Newton iteration on the 4 nonlinear unknowns (Q1 base, Q1 collector / Q2 base, Q2 emitter, Q2 collector).
    // The supply node B and the fuzz-pot wiper are linear, so they are eliminated analytically:
    //   B = A + (C2 - A) * 470 / (8k2 + 470)         wiper = (Gtw * E2 + ihc) / (Gtw + Gwg + gc)
    double newton (const Vec4& xin, Vec4& xout, bool dc, double gsrc, double geq, double gc, double ihc)
    {
        const double VA = -9.0, G33 = 1.0 / 33e3, Gc2 = 1.0 / (8.2e3 + 470.0), G100 = 1.0 / q.rfb, Gtw = 1.0 / rtw, Gwg = 1.0 / rwg;
        const double D = Gtw + Gwg + (dc ? 0.0 : gc);
        const double Geff = Gtw * (Gwg + (dc ? 0.0 : gc)) / D, Ieff = dc ? 0.0 : -Gtw * ihc / D;
        const Q q1 = evalQ (0.0, xin[0], xin[1]);          // Q1: E = ground, B = x0, C = x1
        const Q q2 = evalQ (xin[2], xin[1], xin[3]);       // Q2: E = x2, B = x1, C = x3
        const double gmin = 1e-12;
        double A[4][5] = {};
        // node 0: Q1 base
        const double F0 = (xin[0] - xin[2]) * G100 + (dc ? 0.0 : geq * xin[0] - gsrc) - q1.ibOut + gmin * xin[0];
        A[0][0] = G100 + (dc ? 0.0 : geq) - q1.dIb[1] + gmin;  A[0][1] = -q1.dIb[2];  A[0][2] = -G100;        A[0][4] = -F0;
        // node 1: Q1 collector = Q2 base
        const double F1 = (xin[1] - VA) * G33 - q1.icOut - q2.ibOut + gmin * xin[1];
        A[1][0] = -q1.dIc[1];  A[1][1] = G33 - q1.dIc[2] - q2.dIb[1] + gmin;  A[1][2] = -q2.dIb[0];  A[1][3] = -q2.dIb[2];  A[1][4] = -F1;
        // node 2: Q2 emitter
        const double F2 = (xin[2] - xin[0]) * G100 + Geff * xin[2] + Ieff + q2.ieIn + gmin * xin[2];
        A[2][0] = -G100;  A[2][1] = q2.dIe[1];  A[2][2] = G100 + Geff + q2.dIe[0] + gmin;  A[2][3] = q2.dIe[2];  A[2][4] = -F2;
        // node 3: Q2 collector, through 8k2 + 470 to the battery
        const double F3 = (xin[3] - VA) * Gc2 - q2.icOut + gmin * xin[3];
        A[3][1] = -q2.dIc[1];  A[3][2] = -q2.dIc[0];  A[3][3] = Gc2 - q2.dIc[2] + gmin;  A[3][4] = -F3;

        for (int c = 0; c < 4; ++c)         // Gaussian elimination, partial pivoting
        {
            int piv = c; for (int r = c + 1; r < 4; ++r) if (std::fabs (A[r][c]) > std::fabs (A[piv][c])) piv = r;
            if (std::fabs (A[piv][c]) < 1e-30) return 0.0;
            if (piv != c) for (int k = c; k <= 4; ++k) std::swap (A[c][k], A[piv][k]);
            for (int r = c + 1; r < 4; ++r) { const double f = A[r][c] / A[c][c]; for (int k = c; k <= 4; ++k) A[r][k] -= f * A[c][k]; }
        }
        double dx[4];
        for (int r = 3; r >= 0; --r) { double s = A[r][4]; for (int k = r + 1; k < 4; ++k) s -= A[r][k] * dx[k]; dx[r] = s / A[r][r]; }

        // limit the change of every junction voltage so the exponentials stay well behaved
        const double lim = dc ? 0.08 : 0.15;
        double scale = 1.0;
        auto lj = [&] (double dA, double dB) { const double d = std::fabs (dA - dB); if (d > lim) scale = std::min (scale, lim / d); };
        lj (0.0, dx[0]); lj (dx[1], dx[0]); lj (dx[2], dx[1]); lj (dx[3], dx[1]);
        double mx = 0.0;
        for (int i = 0; i < 4; ++i) { xout[(size_t) i] = xin[(size_t) i] + scale * dx[i]; mx = std::max (mx, std::fabs (scale * dx[i])); }
        return mx;
    }

    double wiperVoltage (double e2, bool dc, double gc, double ihc) const
    {
        const double Gtw = 1.0 / rtw, Gwg = 1.0 / rwg, D = Gtw + Gwg + (dc ? 0.0 : gc);
        return (Gtw * e2 + (dc ? 0.0 : ihc)) / D;
    }

    void solveDc()
    {
        Vec4 cur = isSilicon ? Vec4 { -0.62, -1.3, -0.65, -3.2 } : Vec4 { -0.16, -0.65, -0.47, -4.9 }, nxt = cur;
        for (int it = 0; it < 400; ++it)
        {
            const double d = newton (cur, nxt, true, 0, 0, 0, 0); cur = nxt;
            if (d < 1e-11) break;
        }
        x = cur;
        vc1 = -x[0]; i1 = 0.0;                                   // C1 holds -vB1 (source at 0)
        v5prev = wiperVoltage (x[2], true, 0, 0); i5 = 0.0;      // C20 holds its wiper voltage
    }

    void step (double vin)
    {
        const double rs = 6800.0, c1 = 2.2e-6, c20 = 20e-6;
        const double k = dt / (2.0 * c1), geq = 1.0 / (rs + k);
        const double gsrc = geq * (vin - vc1 - k * i1);
        const double gc = 2.0 * c20 / dt, ihc = gc * v5prev + i5;
        Vec4 nxt = x; int it = 0;
        for (; it < 12; ++it)
        {
            const double d = newton (x, nxt, false, gsrc, geq, gc, ihc); x = nxt;
            if (d < 1e-8) { ++it; break; }
        }
        lastIters = it;
        const double inew = geq * (vin - x[0] - vc1 - k * i1);
        vc1 += k * (i1 + inew); i1 = inew;
        const double v5 = wiperVoltage (x[2], false, gc, ihc);
        i5 = gc * (v5 - v5prev) - i5;
        v5prev = v5;
    }

    float fs = 48000.f, fade = 1.f, fadeStep = 0.001f, fuzzKnob = 0.8f;
    double dt = 1.0 / 48000.0, rtw = 200.0, rwg = 800.0;
    bool isSilicon = false;
    Transistor q = germanium();
    Params prm;
    Vec4 x { -0.16, -0.65, -0.47, -4.9 };
    double vc1 = 0.2, i1 = 0.0, v5prev = 0.0, i5 = 0.0;
    int lastIters = 0;
    ktg1::OnePole outHp, outLp;
};
} // namespace ff1
