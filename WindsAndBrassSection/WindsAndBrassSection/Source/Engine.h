#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <algorithm>
#include <array>
#include <cmath>

// Motor de sección de metales y maderas (síntesis propia, sin samples).
namespace wbs
{
constexpr int kNumPlayers = 5;
constexpr int kKeyswitchBase = 24; // MIDI 24..29 = cambio de articulación (C1..F1)

enum Articulation { Sustain = 0, Staccato, Marcato, Fall, Doit, Swell, NumArticulations };
enum Mode { Unison = 0, PolySection, AutoSection };

struct Spec
{
    const char* name;
    const char* id;
    int lo, hi;                 // rango tocable (MIDI)
    float saw, pulse, noise;    // mezcla de fuente
    float cutoff, range, q;     // filtro dependiente de dinámica
    float formantHz, formantMix;
    float pan, vibCents, trim;
};

// Orden = prioridad de voicing de arriba abajo
inline const std::array<Spec, kNumPlayers>& specs()
{
    static const std::array<Spec, kNumPlayers> s {{
        { "Trumpet",   "trumpet",  55, 82, 0.90f, 0.10f, 0.015f,  900.f, 5200.f, 1.1f, 1200.f, 0.55f,  0.25f, 14.f, 0.80f },
        { "Alto Sax",  "alto",     49, 81, 0.60f, 0.45f, 0.060f,  700.f, 3400.f, 0.9f, 1700.f, 0.65f, -0.55f, 18.f, 0.90f },
        { "Tenor Sax", "tenor",    44, 76, 0.60f, 0.45f, 0.070f,  600.f, 3000.f, 0.9f, 1100.f, 0.65f,  0.55f, 18.f, 0.95f },
        { "Trombone",  "trombone", 40, 70, 0.95f, 0.05f, 0.020f,  500.f, 3800.f, 1.0f,  800.f, 0.55f, -0.20f, 12.f, 1.00f },
        { "Bari Sax",  "bari",     36, 69, 0.55f, 0.50f, 0.080f,  450.f, 2500.f, 0.9f,  700.f, 0.70f,  0.00f, 16.f, 1.00f },
    }};
    return s;
}

inline juce::StringArray articulationNames() { return { "Sustain", "Staccato", "Marcato", "Fall", "Doit", "Swell" }; }
inline juce::StringArray chordNames() { return { "Major", "Minor", "Dom 7", "Maj 7", "Min 7", "Sus 4", "Dim 7", "Sixth" }; }
inline juce::StringArray rootNames() { return { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" }; }

inline int chordMask (int q)
{
    static const int iv[8][5] = { { 0, 4, 7, -1, -1 }, { 0, 3, 7, -1, -1 }, { 0, 4, 7, 10, -1 }, { 0, 4, 7, 11, -1 },
                                  { 0, 3, 7, 10, -1 }, { 0, 5, 7, -1, -1 }, { 0, 3, 6, 9, -1 },  { 0, 4, 7, 9, -1 } };
    int m = 0;
    for (int v : iv[juce::jlimit (0, 7, q)])
        if (v >= 0) m |= (1 << v);
    return m;
}

inline int foldToRange (int note, int p)
{
    const auto& s = specs()[(size_t) p];
    while (note > s.hi) note -= 12;
    while (note < s.lo) note += 12;
    return note;
}

inline float attackTime (int a)  { static const float t[] = { 0.05f, 0.012f, 0.014f, 0.04f, 0.04f, 0.55f }; return t[a]; }
inline float releaseTime (int a) { static const float t[] = { 0.12f, 0.06f, 0.10f, 0.45f, 0.35f, 0.20f };   return t[a]; }

inline float polyBlep (float t, float dt)
{
    if (t < dt)        { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt)  { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}

struct SVF
{
    float s1 = 0.f, s2 = 0.f;
    void reset() { s1 = s2 = 0.f; }
    float process (float x, float g, float k, float& band)
    {
        const float a1 = 1.f / (1.f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;
        const float v3 = x - s2;
        const float v1 = a1 * s1 + a2 * v3;
        const float v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.f * v1 - s1;
        s2 = 2.f * v2 - s2;
        band = v1;
        return v2; // paso bajo
    }
};

struct RenderParams { float vibrato = 0.5f, bright = 0.6f, expr = 1.f, mod = 0.f, level = 0.8f; };

//==============================================================================
class Player
{
public:
    void prepare (double sampleRate, int index)
    {
        sr = sampleRate;
        idx = index;
        rng.setSeed (1234 + index * 77);
        reset();
    }

    void reset()
    {
        stage = Idle; env = 0.f; currentNote = -1; delay = 0; age = 0;
        pitchOff = 0.f; accent = 0.f; phase = 0.f; lfoPhase = (float) idx * 0.2f;
        lp.reset(); fm.reset();
    }

    bool isActive() const noexcept { return stage != Idle; }
    float getEnv() const noexcept { return env; }

    void noteOn (int note, float velocity, int art, float human)
    {
        vel = velocity;
        targetSemi = (float) note;

        const bool sustained = (art == Sustain || art == Swell) && (artic == Sustain || artic == Swell);
        if (isActive() && stage != Release && sustained)
        {
            // legato: cambia de nota sin reatacar
            glide = 1.f - std::exp (-1.f / (0.03f * (float) sr));
            currentNote = note;
            return;
        }

        artic = art;
        currentNote = note;
        noteSemi = (float) note;
        glide = 1.f;
        detune = (rng.nextFloat() * 2.f - 1.f) * 6.f * human;
        delay = (int) (rng.nextFloat() * human * 0.035f * (float) sr);
        pitchOff = 0.f;
        pitchRate = 0.f;
        age = 0;
        accent = (art == Marcato) ? 1.f : 0.f;
        atkInc = 1.f / (attackTime (art) * (float) sr);
        stage = Attack;
    }

    void noteOff()
    {
        if (stage == Idle || stage == Release) return;
        startRelease();
    }

    void render (float* out, int n, const RenderParams& rp)
    {
        if (stage == Idle) return;

        const auto& sp = specs()[(size_t) idx];
        const float fs = (float) sr;
        const float pi = juce::MathConstants<float>::pi;
        const float twoPi = juce::MathConstants<float>::twoPi;
        const float velAmt = 0.25f + 0.75f * vel;
        const float k1 = 1.f / sp.q;
        const float gF = std::tan (pi * sp.formantHz / fs);
        const float kF = 0.5f;

        for (int i = 0; i < n; ++i)
        {
            if (delay > 0) { --delay; continue; }

            switch (stage)
            {
                case Attack:
                    env += atkInc;
                    if (env >= 1.f) { env = 1.f; stage = Sustained; }
                    break;
                case Sustained:
                    if (artic == Staccato && age > (int) (0.15f * fs)) startRelease();
                    break;
                case Release:
                    env -= relDec;
                    pitchOff += pitchRate;
                    if (env <= 0.f) { env = 0.f; stage = Idle; return; }
                    break;
                default: break;
            }

            if (age < (1 << 30)) ++age;
            noteSemi += (targetSemi - noteSemi) * glide;

            // vibrato con retardo de entrada
            lfoPhase += 5.3f / fs;
            if (lfoPhase >= 1.f) lfoPhase -= 1.f;
            const float vibEnv = juce::jlimit (0.f, 1.f, ((float) age / fs - 0.3f) / 0.6f);
            const float vibCents = std::sin (twoPi * lfoPhase) * sp.vibCents * rp.vibrato * vibEnv;

            const float semis = noteSemi + (detune + vibCents) / 100.f + pitchOff;
            const float freq = 440.f * std::exp2 ((semis - 69.f) / 12.f);
            const float dt = juce::jlimit (0.0001f, 0.45f, freq / fs);

            phase += dt;
            if (phase >= 1.f) phase -= 1.f;

            const float saw = 2.f * phase - 1.f - polyBlep (phase, dt);
            const float pw = 0.38f;
            float p2 = phase - pw + 1.f;
            p2 -= std::floor (p2);
            const float pulse = (phase < pw ? 1.f : -1.f) + polyBlep (phase, dt) - polyBlep (p2, dt);
            const float noise = rng.nextFloat() * 2.f - 1.f;

            const float src = saw * sp.saw + pulse * sp.pulse + noise * sp.noise * (0.3f + 0.7f * env);

            // el timbre se abre con dinámica, velocity, marcato y rueda de modulación
            const float open = juce::jlimit (0.f, 1.f, env * velAmt * (0.35f + 0.65f * rp.bright)
                                                         + accent * 0.35f + rp.mod * 0.4f);
            accent *= 0.9996f;
            const float fc = juce::jmin (sp.cutoff + sp.range * open * open, 0.45f * fs);
            const float g = std::tan (pi * fc / fs);

            float bandUnused = 0.f, bandF = 0.f;
            const float low = lp.process (src, g, k1, bandUnused);
            fm.process (src, gF, kF, bandF);
            const float y = low + sp.formantMix * bandF * kF;

            const float gain = env * (0.2f + 0.8f * velAmt) * sp.trim * rp.level * rp.expr * (1.f + accent * 0.4f);
            out[i] += y * gain;
        }
    }

private:
    enum Stage { Idle, Attack, Sustained, Release };

    void startRelease()
    {
        stage = Release;
        currentNote = -1;
        const float rt = releaseTime (artic);
        relDec = std::max (env, 0.001f) / (rt * (float) sr);
        pitchRate = 0.f;
        if (artic == Fall) pitchRate = -7.f / (rt * (float) sr);
        if (artic == Doit) pitchRate =  5.f / (rt * (float) sr);
    }

    double sr = 44100.0;
    int idx = 0;
    juce::Random rng;

    Stage stage = Idle;
    int artic = Sustain;
    int currentNote = -1;
    int delay = 0;
    int age = 0;

    float env = 0.f, atkInc = 0.01f, relDec = 0.001f;
    float vel = 0.8f, accent = 0.f;
    float noteSemi = 60.f, targetSemi = 60.f, glide = 1.f, detune = 0.f;
    float pitchOff = 0.f, pitchRate = 0.f;
    float phase = 0.f, lfoPhase = 0.f;

    SVF lp, fm;
};

//==============================================================================
class Section
{
public:
    struct Settings
    {
        int mode = PolySection, artic = Sustain, root = 0, quality = 0;
        std::array<bool, kNumPlayers> on { { true, true, true, true, true } };
        std::array<float, kNumPlayers> level { { 0.8f, 0.8f, 0.8f, 0.8f, 0.8f } };
        float vibrato = 0.5f, human = 0.4f, bright = 0.6f;
    };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (int p = 0; p < kNumPlayers; ++p)
        {
            players[(size_t) p].prepare (sr, p);
            const float a = (specs()[(size_t) p].pan + 1.f) * juce::MathConstants<float>::pi * 0.25f;
            panL[(size_t) p] = std::cos (a);
            panR[(size_t) p] = std::sin (a);
        }
        reset();
    }

    void reset()
    {
        held.fill (false);
        latched.fill (false);
        playing.fill (-1);
        pedal = false; keyswitch = -1; lastParamArtic = -1;
        expr = exprTarget = 1.f; mod = modTarget = 0.f;
        for (auto& p : players) p.reset();
    }

    void handleMidi (const juce::MidiMessage& m, const Settings& s)
    {
        if (m.isNoteOn())
        {
            const int n = m.getNoteNumber();
            if (n >= kKeyswitchBase && n < kKeyswitchBase + NumArticulations)
                keyswitch = n - kKeyswitchBase;
            else
            {
                held[(size_t) n] = true;
                latched[(size_t) n] = true;
                lastVel = m.getFloatVelocity();
            }
            refresh (s);
        }
        else if (m.isNoteOff())
        {
            const int n = m.getNoteNumber();
            if (n >= kKeyswitchBase && n < kKeyswitchBase + NumArticulations) return;
            held[(size_t) n] = false;
            if (! pedal) latched[(size_t) n] = false;
            refresh (s);
        }
        else if (m.isController())
        {
            const int cc = m.getControllerNumber();
            const float v = (float) m.getControllerValue() / 127.f;
            if (cc == 1)  modTarget = v;
            if (cc == 11) exprTarget = v;
            if (cc == 64)
            {
                pedal = v >= 0.5f;
                if (! pedal) latched = held;
                refresh (s);
            }
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            held.fill (false);
            latched.fill (false);
            refresh (s);
        }
    }

    // Reparte las notas pulsadas entre los intérpretes según el modo
    void refresh (const Settings& s)
    {
        if (s.artic != lastParamArtic) { keyswitch = -1; lastParamArtic = s.artic; }
        effArtic = keyswitch >= 0 ? keyswitch : s.artic;

        int notes[128];
        int n = 0;
        for (int i = 0; i < 128; ++i)
            if (held[(size_t) i] || latched[(size_t) i]) notes[n++] = i;

        std::array<int, kNumPlayers> target;
        target.fill (-1);

        if (n > 0)
        {
            const int lead = notes[n - 1];

            if (s.mode == Unison)
            {
                for (int p = 0; p < kNumPlayers; ++p)
                    if (s.on[(size_t) p]) target[(size_t) p] = foldToRange (lead, p);
            }
            else if (s.mode == PolySection)
            {
                int k = 0;
                for (int p = 0; p < kNumPlayers; ++p)
                    if (s.on[(size_t) p])
                    {
                        target[(size_t) p] = foldToRange (notes[n - 1 - (k % n)], p);
                        ++k;
                    }
            }
            else // AutoSection
            {
                const int root = (n >= 2) ? notes[0] % 12 : s.root;
                const int mask = chordMask (s.quality);
                int cur = lead;
                bool first = true;
                for (int p = 0; p < kNumPlayers; ++p)
                {
                    if (! s.on[(size_t) p]) continue;
                    int note = lead;
                    if (! first)
                    {
                        note = cur - 1;
                        while (! (mask & (1 << (((note - root) % 12 + 12) % 12)))) --note;
                    }
                    first = false;
                    cur = note;
                    target[(size_t) p] = foldToRange (note, p);
                }
            }
        }

        for (int p = 0; p < kNumPlayers; ++p)
        {
            const int t = target[(size_t) p];
            if (t == playing[(size_t) p]) continue;
            if (t < 0) players[(size_t) p].noteOff();
            else       players[(size_t) p].noteOn (t, lastVel, effArtic, s.human);
            playing[(size_t) p] = t;
        }
    }

    void render (float* L, float* R, int n, const Settings& s)
    {
        int done = 0;
        float tmp[32];
        while (done < n)
        {
            const int m = std::min (32, n - done);
            const float c = 1.f - std::exp (-(float) m / (0.02f * (float) sr));
            expr += (exprTarget - expr) * c;
            mod  += (modTarget - mod) * c;

            for (int p = 0; p < kNumPlayers; ++p)
            {
                auto& pl = players[(size_t) p];
                if (! pl.isActive()) continue;

                RenderParams rp;
                rp.vibrato = s.vibrato; rp.bright = s.bright; rp.expr = expr; rp.mod = mod;
                rp.level = s.level[(size_t) p];

                std::fill (tmp, tmp + m, 0.f);
                pl.render (tmp, m, rp);
                for (int i = 0; i < m; ++i)
                {
                    L[done + i] += tmp[i] * panL[(size_t) p];
                    R[done + i] += tmp[i] * panR[(size_t) p];
                }
            }
            done += m;
        }
    }

    int getEffectiveArtic() const noexcept { return effArtic; }
    float getPlayerLevel (int i) const noexcept { return players[(size_t) i].getEnv(); }

private:
    double sr = 44100.0;
    std::array<Player, kNumPlayers> players;
    std::array<float, kNumPlayers> panL {}, panR {};
    std::array<int, kNumPlayers> playing {};
    std::array<bool, 128> held {}, latched {};
    bool pedal = false;
    int keyswitch = -1, lastParamArtic = -1, effArtic = 0;
    float lastVel = 0.8f;
    float expr = 1.f, exprTarget = 1.f, mod = 0.f, modTarget = 0.f;
};

} // namespace wbs
