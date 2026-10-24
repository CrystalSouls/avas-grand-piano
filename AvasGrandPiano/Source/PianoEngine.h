// Ava's Grand Piano - modeled piano engine.
//
// Pure C++17, no plugin framework needed. Each note is built from physical
// ingredients of a real grand piano:
//   * inharmonic partials (stiff strings ring slightly sharp as you go up)
//   * hammer strike point + hammer softness (velocity changes the tone, not just volume)
//   * two strings per note, slightly out of tune, which gives the lovely slow "beating"
//   * two-stage decay (bright quick "prompt" sound, then a long quiet "aftersound")
//   * higher partials die faster than lower ones
//   * felt dampers (the top octave has none, like a real piano) and a sustain pedal
//   * hammer thump + damper thud noises, stereo spread across the keyboard, and a room
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace ava
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kLn1000 = 6.907755278982137; // 60 dB of decay
constexpr int kMaxVoices = 40;
constexpr int kMaxPartials = 32;
constexpr int kChunk = 128;

// Overall loudness calibration (set from offline level tests).
constexpr double kOutputScale = 0.16;
constexpr double kRoomWetGain = 1.6;

struct Rng
{
    uint32_t state = 0x1234567u;
    uint32_t next() { state = state * 1664525u + 1013904223u; return state; }
    double uni() { return (next() >> 8) * (1.0 / 16777216.0); }  // 0..1
    double bi() { return uni() * 2.0 - 1.0; }                    // -1..1
};

struct Settings
{
    double brightness = 0.5; // 0..1  hammer hardness / sparkle
    double sustain = 1.0;    // 0.4..1.8 multiplier on ring time
    double width = 0.6;      // 0..1 stereo spread
    double room = 0.25;      // 0..1 room amount
    double volumeDb = 0.0;   // master volume
};

//==============================================================================
class Voice
{
public:
    bool active = false;
    bool keyDown = false;
    bool damping = false;
    int note = 0;
    uint64_t order = 0;
    double level = 0.0; // recent output level (used when stealing voices)

    void start(int midiNote, int velocity, double sampleRate, const Settings& s, Rng& rng, uint64_t startOrder)
    {
        sr = sampleRate;
        active = true;
        keyDown = true;
        damping = false;
        note = midiNote;
        order = startOrder;
        dampG = 1.0;
        dampMul = 1.0;
        atkG = 0.0;
        atkInc = 1.0 / (0.0025 * sr);
        level = 0.0;
        lp = lp2 = 0.0;
        thudEnv = 0.0;

        const double v = std::clamp(velocity / 127.0, 0.0, 1.0);
        const double m = (double) midiNote;

        // Piano tuning is "stretched": bass slightly flat, treble slightly sharp.
        const double stretchCents = 30.0 * std::pow((m - 64.5) / 43.5, 3.0);
        const double f0 = 440.0 * std::pow(2.0, (m - 69.0) / 12.0 + stretchCents / 1200.0);

        // String stiffness (inharmonicity) grows toward the treble.
        const double B = std::clamp(1.0e-4 * std::exp(0.058 * (m - 45.0)), 6.0e-5, 8.0e-3);

        // Harder hit and brighter setting = more high partials.
        const double hammerHz = (900.0 + 3600.0 * std::pow(v, 1.2)) * (0.45 + 1.1 * s.brightness)
                                * std::pow(f0 / 261.6, 0.25);
        const double strikePoint = 0.125; // hammer hits 1/8 along the string

        const double T60base = 32.0 * std::exp(-0.026 * (m - 21.0)) * s.sustain; // seconds
        const double slowFrac = std::clamp(0.25 + (60.0 - m) * 0.006, 0.22, 0.50);
        const double fastRatio = std::clamp(0.20 + (60.0 - m) * 0.004, 0.16, 0.32);
        const double beatScale = std::clamp((m - 28.0) / 20.0, 0.25, 1.0);

        double amps[kMaxPartials];
        double freqs[kMaxPartials];
        int n = 0;
        double sumsq = 0.0;
        for (int k = 1; k <= kMaxPartials; ++k)
        {
            const double fk = k * f0 * std::sqrt(1.0 + B * k * k);
            if (fk > 0.46 * sr || fk > 18000.0)
                break;
            double a = std::fabs(std::sin(kPi * k * strikePoint)) / (double) k;
            const double r = fk / hammerHz;
            a /= std::sqrt(1.0 + r * r * r * r);
            if (a < 1.0e-5)
                continue;
            amps[n] = a;
            freqs[n] = fk;
            sumsq += a * a;
            ++n;
        }
        if (n == 0) // extremely high note at low sample rate: keep at least a sine
        {
            amps[0] = 1.0;
            freqs[0] = std::min(f0, 0.45 * sr);
            sumsq = 1.0;
            n = 1;
        }

        const double norm = 1.0 / std::sqrt(sumsq);
        const double gain = std::pow(v, 1.4) * kOutputScale * levelCompensation(m);

        partialCount = n;
        for (int i = 0; i < n; ++i)
        {
            const double fk = freqs[i];
            const double A = amps[i] * norm * gain;

            // higher partials die faster than the fundamental (relative to this note's own fundamental)
            const double damp0 = 1.0 + 0.5 * std::pow(f0 / 700.0, 1.2);
            const double dampK = 1.0 + 0.5 * std::pow(fk / 700.0, 1.2);
            const double T60 = std::max(T60base * damp0 / dampK, 0.05);
            const double T60fast = std::max(T60 * fastRatio, 0.03);

            Partial& p = parts[i];
            p.dS = std::exp(-kLn1000 / (T60 * sr));
            p.dF = std::exp(-kLn1000 / (T60fast * sr));
            p.envS = A * slowFrac;
            p.envF = A * (1.0 - slowFrac);

            // string 1 sits on the note, string 2 is a hair out of tune -> beating
            const double sign = rng.uni() < 0.5 ? -1.0 : 1.0;
            const double cents = (0.3 + 1.3 * rng.uni()) * beatScale * sign;
            const double fk2 = fk * std::pow(2.0, cents / 1200.0);

            const double w1 = 2.0 * kPi * fk / sr;
            const double w2 = 2.0 * kPi * fk2 / sr;
            p.c1r = std::cos(w1);
            p.c1i = std::sin(w1);
            p.c2r = std::cos(w2);
            p.c2i = std::sin(w2);
            p.s1r = 1.0;
            p.s1i = 0.0;
            const double ph2 = 0.4 * rng.bi();
            p.s2r = 0.75 * std::cos(ph2);
            p.s2i = 0.75 * std::sin(ph2);
        }

        // Hammer thump: a short burst of low-passed noise.
        const double cutN = std::clamp(f0 * 4.0, 250.0, 3000.0) * (0.6 + 0.8 * s.brightness);
        noiseCoef = 1.0 - std::exp(-2.0 * kPi * cutN / sr);
        noiseEnv = gain * 0.30 * (0.3 + 0.7 * v);
        noiseDecay = std::exp(-1.0 / (0.007 * sr));
        thudAmp = gain * 0.08 * (0.4 + 0.6 * v);
        this->rng = &rng;

        panPos = std::clamp((m - 64.0) / 44.0, -1.0, 1.0);
    }

    // Felt damper comes down on the strings.
    void beginDamping(bool fast)
    {
        keyDown = false;
        if (damping && !fast)
            return;
        damping = true;

        if (fast)
        {
            dampMul = std::exp(-kLn1000 / (0.07 * sr));
            return;
        }
        if (note > 96) // the top octave of a piano has no dampers
        {
            dampMul = 1.0;
            return;
        }
        const double T60d = std::clamp(0.40 + (60.0 - note) * 0.010, 0.12, 1.1);
        dampMul = std::exp(-kLn1000 / (T60d * sr));
        thudEnv = thudAmp;
    }

    void process(float* L, float* R, int n, double width)
    {
        while (n > 0 && active)
        {
            const int c = std::min(n, kChunk);
            renderChunk(L, R, c, width);
            L += c;
            R += c;
            n -= c;
        }
    }

private:
    struct Partial
    {
        double s1r, s1i, c1r, c1i;
        double s2r, s2i, c2r, c2i;
        double envF, envS, dF, dS;
    };

    // Per-pitch loudness trim so bass, middle and treble feel evenly loud.
    static double levelCompensation(double m)
    {
        static const double keys[8] = { 21.0, 36.0, 48.0, 60.0, 72.0, 84.0, 96.0, 108.0 };
        static const double dB[8] = { -2.5, -2.0, -1.0, 0.0, 1.5, 2.0, 3.0, 4.0 };
        if (m <= keys[0]) return std::pow(10.0, dB[0] / 20.0);
        for (int i = 1; i < 8; ++i)
        {
            if (m <= keys[i])
            {
                const double t = (m - keys[i - 1]) / (keys[i] - keys[i - 1]);
                return std::pow(10.0, (dB[i - 1] + t * (dB[i] - dB[i - 1])) / 20.0);
            }
        }
        return std::pow(10.0, dB[7] / 20.0);
    }

    void renderChunk(float* L, float* R, int c, double width)
    {
        double b1[kChunk];
        double b2[kChunk];
        for (int i = 0; i < c; ++i)
            b1[i] = b2[i] = 0.0;

        for (int p = 0; p < partialCount; ++p)
        {
            Partial& q = parts[p];
            double s1r = q.s1r, s1i = q.s1i, s2r = q.s2r, s2i = q.s2i;
            double eF = q.envF, eS = q.envS;
            const double c1r = q.c1r, c1i = q.c1i, c2r = q.c2r, c2i = q.c2i;
            const double dF = q.dF, dS = q.dS;
            for (int i = 0; i < c; ++i)
            {
                const double amp = eF + eS;
                b1[i] += amp * s1i;
                b2[i] += amp * s2i;
                const double n1r = s1r * c1r - s1i * c1i;
                const double n1i = s1r * c1i + s1i * c1r;
                s1r = n1r;
                s1i = n1i;
                const double n2r = s2r * c2r - s2i * c2i;
                const double n2i = s2r * c2i + s2i * c2r;
                s2r = n2r;
                s2i = n2i;
                eF *= dF;
                eS *= dS;
            }
            q.s1r = s1r; q.s1i = s1i; q.s2r = s2r; q.s2i = s2i;
            q.envF = eF; q.envS = eS;
        }

        // stereo placement: the two strings sit a touch apart, bass left / treble right
        const double pos = panPos * width * 0.7;
        const double p1 = std::clamp(pos - 0.15 * width, -1.0, 1.0);
        const double p2 = std::clamp(pos + 0.15 * width, -1.0, 1.0);
        const double a1 = (p1 + 1.0) * kPi / 4.0;
        const double a2 = (p2 + 1.0) * kPi / 4.0;
        const double g1L = std::cos(a1), g1R = std::sin(a1);
        const double g2L = std::cos(a2), g2R = std::sin(a2);
        const double cL = std::cos((pos + 1.0) * kPi / 4.0), cR = std::sin((pos + 1.0) * kPi / 4.0);

        double peak = 0.0;
        for (int i = 0; i < c; ++i)
        {
            if (atkG < 1.0)
                atkG = std::min(1.0, atkG + atkInc);
            if (damping)
                dampG *= dampMul;
            const double gv = atkG * dampG;

            double l = (b1[i] * g1L + b2[i] * g2L) * gv;
            double r = (b1[i] * g1R + b2[i] * g2R) * gv;

            if (noiseEnv > 1.0e-6)
            {
                lp += noiseCoef * (rng->bi() - lp);
                const double nz = lp * noiseEnv;
                l += nz * cL;
                r += nz * cR;
                noiseEnv *= noiseDecay;
            }
            if (thudEnv > 1.0e-6)
            {
                lp2 += 0.08 * (rng->bi() - lp2);
                const double nz = lp2 * thudEnv;
                l += nz * cL;
                r += nz * cR;
                thudEnv *= 0.9985;
            }

            L[i] += (float) l;
            R[i] += (float) r;
            peak = std::max(peak, std::fabs(l) + std::fabs(r));
        }
        level = peak;

        for (int p = 0; p < partialCount; ++p)
        {
            if (parts[p].envF + parts[p].envS < 1.0e-6)
            {
                parts[p] = parts[partialCount - 1];
                --partialCount;
                --p;
            }
        }
        if (partialCount == 0 && noiseEnv < 1.0e-6 && thudEnv < 1.0e-6)
            active = false;
        if (damping && dampG < 1.0e-4)
            active = false;
    }

    double sr = 44100.0;
    Partial parts[kMaxPartials];
    int partialCount = 0;
    double dampG = 1.0, dampMul = 1.0;
    double atkG = 0.0, atkInc = 0.01;
    double panPos = 0.0;
    double noiseEnv = 0.0, noiseDecay = 0.99, noiseCoef = 0.2, lp = 0.0;
    double thudEnv = 0.0, thudAmp = 0.0, lp2 = 0.0;
    Rng* rng = nullptr;
};

//==============================================================================
// A small stereo room (Freeverb style): gives the piano a sense of space.
class Room
{
public:
    void prepare(double sr)
    {
        static const int combTune[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int apTune[4] = { 556, 441, 341, 225 };
        const double scale = sr / 44100.0;
        for (int i = 0; i < 8; ++i)
        {
            combL[i].init((int) std::lround(combTune[i] * scale));
            combR[i].init((int) std::lround((combTune[i] + 23) * scale));
        }
        for (int i = 0; i < 4; ++i)
        {
            apL[i].init((int) std::lround(apTune[i] * scale));
            apR[i].init((int) std::lround((apTune[i] + 23) * scale));
        }
    }

    void reset()
    {
        for (auto& c : combL) c.clear();
        for (auto& c : combR) c.clear();
        for (auto& a : apL) a.clear();
        for (auto& a : apR) a.clear();
    }

    void process(float* L, float* R, int n, double amount)
    {
        if (amount < 1.0e-4)
            return;
        const float fb = 0.86f;
        const float damp = 0.35f;
        const float dry = (float) (1.0 - 0.3 * amount);
        const float wet = (float) (amount * kRoomWetGain);
        for (int i = 0; i < n; ++i)
        {
            const float in = (L[i] + R[i]) * 0.015f;
            float oL = 0.f, oR = 0.f;
            for (int k = 0; k < 8; ++k)
            {
                oL += combL[k].process(in, fb, damp);
                oR += combR[k].process(in, fb, damp);
            }
            for (int k = 0; k < 4; ++k)
            {
                oL = apL[k].process(oL);
                oR = apR[k].process(oR);
            }
            L[i] = L[i] * dry + oL * wet;
            R[i] = R[i] * dry + oR * wet;
        }
    }

private:
    struct Comb
    {
        std::vector<float> buf;
        int idx = 0;
        float store = 0.f;
        void init(int size) { buf.assign((size_t) std::max(1, size), 0.f); idx = 0; store = 0.f; }
        void clear() { std::fill(buf.begin(), buf.end(), 0.f); store = 0.f; idx = 0; }
        float process(float in, float fb, float damp)
        {
            const float out = buf[(size_t) idx];
            store = out * (1.f - damp) + store * damp;
            if (std::fabs(store) < 1.0e-20f) store = 0.f;
            buf[(size_t) idx] = in + store * fb;
            if (++idx >= (int) buf.size()) idx = 0;
            return out;
        }
    };
    struct Allpass
    {
        std::vector<float> buf;
        int idx = 0;
        void init(int size) { buf.assign((size_t) std::max(1, size), 0.f); idx = 0; }
        void clear() { std::fill(buf.begin(), buf.end(), 0.f); idx = 0; }
        float process(float in)
        {
            const float b = buf[(size_t) idx];
            const float out = -in + b;
            buf[(size_t) idx] = in + b * 0.5f;
            if (++idx >= (int) buf.size()) idx = 0;
            return out;
        }
    };
    Comb combL[8], combR[8];
    Allpass apL[4], apR[4];
};

//==============================================================================
class PianoEngine
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        room.prepare(sr);
        reset();
    }

    void setSettings(const Settings& s)
    {
        settings = s;
    }

    void reset()
    {
        for (auto& v : voices)
            v.active = false;
        pedalDown = false;
        room.reset();
        currentGain = std::pow(10.0, settings.volumeDb / 20.0);
    }

    void noteOn(int note, int velocity)
    {
        if (velocity <= 0)
        {
            noteOff(note);
            return;
        }
        // a re-struck key quickly mutes its previous ring
        for (auto& v : voices)
            if (v.active && v.note == note)
                v.beginDamping(true);

        Voice* v = allocate();
        v->start(note, velocity, sr, settings, rng, ++orderCounter);
    }

    void noteOff(int note)
    {
        for (auto& v : voices)
        {
            if (v.active && v.note == note && v.keyDown)
            {
                v.keyDown = false;
                if (!pedalDown)
                    v.beginDamping(false);
            }
        }
    }

    void setSustainPedal(bool down)
    {
        if (down == pedalDown)
            return;
        pedalDown = down;
        if (!down)
            for (auto& v : voices)
                if (v.active && !v.keyDown && !v.damping)
                    v.beginDamping(false);
    }

    void allNotesOff()
    {
        pedalDown = false;
        for (auto& v : voices)
            if (v.active)
                v.beginDamping(false);
    }

    void render(float* L, float* R, int n)
    {
        std::fill(L, L + n, 0.f);
        std::fill(R, R + n, 0.f);

        for (auto& v : voices)
            if (v.active)
                v.process(L, R, n, settings.width);

        room.process(L, R, n, settings.room);

        const double target = std::pow(10.0, settings.volumeDb / 20.0);
        const double step = (target - currentGain) / (double) std::max(1, n);
        for (int i = 0; i < n; ++i)
        {
            currentGain += step;
            L[i] = softClip((float) (L[i] * currentGain));
            R[i] = softClip((float) (R[i] * currentGain));
        }
        currentGain = target;
    }

    int activeVoices() const
    {
        int c = 0;
        for (auto& v : voices)
            if (v.active)
                ++c;
        return c;
    }

private:
    static float softClip(float x)
    {
        const float t = 0.8f;
        const float a = std::fabs(x);
        if (a <= t)
            return x;
        const float y = t + (1.f - t) * std::tanh((a - t) / (1.f - t));
        return x < 0.f ? -y : y;
    }

    Voice* allocate()
    {
        for (auto& v : voices)
            if (!v.active)
                return &v;
        Voice* best = &voices[0];
        double bestScore = 1.0e300;
        for (auto& v : voices)
        {
            const double score = v.damping ? v.level : 1.0e6 + (double) v.order;
            if (score < bestScore)
            {
                bestScore = score;
                best = &v;
            }
        }
        return best;
    }

    double sr = 44100.0;
    Settings settings;
    Voice voices[kMaxVoices];
    Room room;
    Rng rng;
    uint64_t orderCounter = 0;
    bool pedalDown = false;
    double currentGain = 1.0;
};

} // namespace ava
