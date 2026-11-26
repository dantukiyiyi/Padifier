#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

constexpr float kPi = 3.14159265f;

inline float clamp01 (float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

// Circular delay line. read(0) returns the most recently pushed sample.
struct Delay
{
    std::vector<float> buf;
    int wp = 0;

    void reset (int size)
    {
        buf.assign ((size_t) std::max (4, size), 0.0f);
        wp = 0;
    }

    void push (float x)
    {
        buf[(size_t) wp] = x;
        wp = (wp + 1) % (int) buf.size();
    }

    float read (double d) const
    {
        const double n = (double) buf.size();
        double pos = (double) wp - 1.0 - d;
        while (pos < 0.0) pos += n;
        while (pos >= n) pos -= n;
        const int i0 = (int) pos;
        const int i1 = (i0 + 1) % (int) n;
        const float f = (float) (pos - i0);
        return buf[(size_t) i0] + f * (buf[(size_t) i1] - buf[(size_t) i0]);
    }
};

struct Allpass
{
    Delay d;
    int len = 2;
    float g = 0.6f;

    void reset (int l, float gain)
    {
        len = std::max (2, l);
        g = gain;
        d.reset (len + 2);
    }

    float process (float x)
    {
        const float v = d.read (len - 1);
        const float y = -g * x + v;
        d.push (x + g * v);
        return y;
    }
};

struct Comb
{
    Delay d;
    int len = 2;

    void reset (int l)
    {
        len = std::max (2, l);
        d.reset (len + 2);
    }

    float process (float x, float fb)
    {
        const float v = d.read (len - 1);
        d.push (x + fb * v);
        return v;
    }
};

struct OnePole
{
    float a = 1.0f;
    float z = 0.0f;

    void setCutoff (double fc, double fs)
    {
        a = (float) (1.0 - std::exp (-6.283185307179586 * fc / fs));
    }

    float process (float x)
    {
        z += a * (x - z);
        return z;
    }
};

// Wet-path band limiter: High End lowpass and Low End highpass, 0 = bypassed.
struct BandFilter
{
    OnePole hiLp, loLp;
    bool hiOn = false, loOn = false;

    void setup (float hiPct, float loPct, double fs)
    {
        const double hiFc = 22000.0 * std::pow (500.0 / 22000.0, hiPct / 100.0);
        const double loFc = 20.0 * std::pow (2000.0 / 20.0, loPct / 100.0);
        hiOn = hiPct > 0.0f;
        loOn = loPct > 0.0f;
        hiLp.setCutoff (std::min (hiFc, fs * 0.45), fs);
        loLp.setCutoff (loFc, fs);
    }

    float process (float x)
    {
        if (hiOn) x = hiLp.process (x);
        if (loOn) x = x - loLp.process (x);
        return x;
    }
};

// Pitch shifter: two read heads over a delay line, phase-offset by half a window.
struct PitchShifter
{
    static constexpr int kWin = 2048;
    static constexpr int kMinDelay = 8;

    Delay d;
    double phase = 0.0;

    void reset()
    {
        d.reset (kWin + kMinDelay + 64);
        phase = 0.0;
    }

    float process (float x, float ratio)
    {
        d.push (x);
        phase += (1.0 - (double) ratio) / (double) kWin;
        while (phase < 0.0) phase += 1.0;
        while (phase >= 1.0) phase -= 1.0;

        double p2 = phase + 0.5;
        if (p2 >= 1.0) p2 -= 1.0;

        const float w1 = std::sin (kPi * (float) phase);
        const float w2 = std::sin (kPi * (float) p2);

        return w1 * w1 * d.read (kMinDelay + phase * kWin)
             + w2 * w2 * d.read (kMinDelay + p2 * kWin);
    }
};

struct Params
{
    float arm = 0.0f;
    float armSens = 0.5f;
    float elast = 0.0f;
    float rev = 0.0f;
    float dist = 0.0f;
    float resp = 0.0f;
    float sat = 0.0f;
    float satAgg = 0.0f;
    float flg = 0.0f;
    float flgDepth = 0.5f;
    float flgFb = 0.3f;
    float combFb[2] = { 0.0f, 0.0f };
};

struct ChannelState
{
    PitchShifter fifth, octave;
    Allpass ap[4];
    Comb comb[2];
    Delay early, flanger;
    OnePole distLp, respLp4k, respLp1k;
    BandFilter fx[7];
    BandFilter master;
    std::uint32_t seed = 22222u;

    void prepare (double fs)
    {
        fifth.reset();
        octave.reset();

        const double apMs[4] = { 7.3, 10.1, 13.7, 17.9 };
        for (int i = 0; i < 4; ++i)
            ap[i].reset ((int) (apMs[i] * fs / 1000.0), 0.6f);

        comb[0].reset ((int) (29.7 * fs / 1000.0));
        comb[1].reset ((int) (37.1 * fs / 1000.0));
        early.reset ((int) (0.05 * fs) + 8);
        flanger.reset ((int) (0.02 * fs) + 8);
    }
};

// Full effect chain for one channel. Returns the processed sample.
inline float processChain (ChannelState& s, float x, float env, float lfo,
                           const Params& P, double fs)
{
    // 1. Armonias: fifth + octave layer, gated by envelope and sensitivity.
    const float gateA = clamp01 ((env - (1.0f - P.armSens) * 0.2f) * 10.0f);
    const float shifted = 0.5f * s.fifth.process (x, 1.5f)
                        + 0.5f * s.octave.process (x, 2.0f);
    float y = x + P.arm * s.fx[0].process (shifted * gateA);

    // 2. Elasticidad: diffusion that only runs while input is present.
    float diff = y;
    for (auto& ap : s.ap)
        diff = ap.process (diff);
    diff *= clamp01 (env * 8.0f);
    y += P.elast * s.fx[1].process (diff);

    // 3. Reverb: decaying tail from the diffused signal, not gated.
    const float tail = 0.5f * (s.comb[0].process (diff, P.combFb[0])
                             + s.comb[1].process (diff, P.combFb[1]));
    y += P.rev * 0.6f * s.fx[2].process (tail);

    // 4. Distancia: air absorption, early reflections, dry/wet blend.
    const float a = P.dist;
    s.early.push (y);
    const float early = s.early.read ((5.0 + 20.0 * a) * fs / 1000.0);
    const float dWet = s.fx[3].process (s.distLp.process (y) * (1.0f - 0.3f * a)
                                        + 0.4f * a * early);
    y = (1.0f - 0.5f * a) * y + a * dWet;

    // 5. Respiraciones: filtered noise shaped by the input envelope.
    s.seed = s.seed * 1664525u + 1013904223u;
    const float noise = (float) (s.seed >> 8) * (1.0f / 8388608.0f) - 1.0f;
    const float band = s.respLp4k.process (noise);
    const float air = band - s.respLp1k.process (band);
    y += P.resp * 0.3f * s.fx[4].process (air * clamp01 (env * 6.0f));

    // 6. Saturacion: new harmonics from a reference; the original is never shaped.
    const float g = 1.0f + 9.0f * P.satAgg;
    const float h = 0.5f * (y * std::abs (y) + y * y * y);
    const float gen = std::tanh (g * h) / std::tanh (g);
    y += P.sat * 0.5f * s.fx[5].process (gen);

    // 7. Flanger: modulated short delay with feedback.
    const double depthMs = P.flgDepth * 5.0;
    const double delaySamples = (1.0 + depthMs * (0.5 + 0.5 * lfo)) * fs / 1000.0;
    const float delayed = s.flanger.read (delaySamples);
    s.flanger.push (y + P.flgFb * delayed);
    y += P.flg * s.fx[6].process (delayed);

    return y;
}
