// Offline test: renders the piano engine to a WAV file so the sound can be checked
// without a DAW.
//
//   render_test out.wav seconds [events...]
//     on:NOTE:VEL:TIME     note on
//     off:NOTE:TIME        note off
//     ped:0|1:TIME         sustain pedal
//     set:NAME=VALUE       brightness, sustain, width, room, volume (dB)
//
// Example: render_test c4.wav 6 on:60:100:0 off:60:3
#include "../Source/PianoEngine.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

struct Event
{
    double t;
    int type; // 0 on, 1 off, 2 pedal
    int a, b;
};

static void writeWav(const char* path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen(path, "wb");
    if (!f) { std::perror("open"); std::exit(1); }
    const uint32_t n = (uint32_t) L.size();
    const uint32_t dataBytes = n * 4;
    auto w32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto w16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f); w32(36 + dataBytes);
    std::fwrite("WAVEfmt ", 1, 8, f); w32(16); w16(1); w16(2); w32((uint32_t) sr); w32((uint32_t) sr * 4); w16(4); w16(16);
    std::fwrite("data", 1, 4, f); w32(dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        const float l = std::max(-1.f, std::min(1.f, L[i]));
        const float r = std::max(-1.f, std::min(1.f, R[i]));
        w16((uint16_t) (int16_t) std::lround(l * 32767.f));
        w16((uint16_t) (int16_t) std::lround(r * 32767.f));
    }
    std::fclose(f);
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::fprintf(stderr, "usage: %s out.wav seconds [events...]\n", argv[0]);
        return 1;
    }
    const int sr = 44100;
    const double seconds = std::atof(argv[2]);
    ava::Settings s;
    std::vector<Event> ev;
    for (int i = 3; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a.rfind("on:", 0) == 0)
        {
            int note, vel; double t;
            std::sscanf(a.c_str(), "on:%d:%d:%lf", &note, &vel, &t);
            ev.push_back({ t, 0, note, vel });
        }
        else if (a.rfind("off:", 0) == 0)
        {
            int note; double t;
            std::sscanf(a.c_str(), "off:%d:%lf", &note, &t);
            ev.push_back({ t, 1, note, 0 });
        }
        else if (a.rfind("ped:", 0) == 0)
        {
            int d; double t;
            std::sscanf(a.c_str(), "ped:%d:%lf", &d, &t);
            ev.push_back({ t, 2, d, 0 });
        }
        else if (a.rfind("set:", 0) == 0)
        {
            const auto eq = a.find('=');
            const std::string name = a.substr(4, eq - 4);
            const double val = std::atof(a.c_str() + eq + 1);
            if (name == "brightness") s.brightness = val;
            else if (name == "sustain") s.sustain = val;
            else if (name == "width") s.width = val;
            else if (name == "room") s.room = val;
            else if (name == "volume") s.volumeDb = val;
        }
    }
    std::stable_sort(ev.begin(), ev.end(), [](const Event& x, const Event& y) { return x.t < y.t; });

    ava::PianoEngine eng;
    eng.setSettings(s);
    eng.prepare(sr);

    const int total = (int) (seconds * sr);
    std::vector<float> L((size_t) total), R((size_t) total);
    int pos = 0;
    size_t ei = 0;
    while (pos < total)
    {
        int next = total;
        if (ei < ev.size())
            next = std::min(total, (int) std::lround(ev[ei].t * sr));
        if (next > pos)
        {
            const int n = std::min(next - pos, 512);
            eng.render(L.data() + pos, R.data() + pos, n);
            pos += n;
            continue;
        }
        while (ei < ev.size() && (int) std::lround(ev[ei].t * sr) <= pos)
        {
            const Event& e = ev[ei++];
            if (e.type == 0) eng.noteOn(e.a, e.b);
            else if (e.type == 1) eng.noteOff(e.a);
            else eng.setSustainPedal(e.a != 0);
        }
    }

    double peak = 0;
    for (int i = 0; i < total; ++i)
        peak = std::max(peak, (double) std::max(std::fabs(L[i]), std::fabs(R[i])));
    std::printf("peak %.4f (%.1f dBFS)\n", peak, 20.0 * std::log10(std::max(peak, 1e-9)));
    for (double t = 0; t < seconds; t += 0.5)
    {
        const int a = (int) (t * sr), b = std::min(total, (int) ((t + 0.5) * sr));
        double sum = 0;
        for (int i = a; i < b; ++i) sum += 0.5 * ((double) L[i] * L[i] + (double) R[i] * R[i]);
        const double rms = std::sqrt(sum / std::max(1, b - a));
        std::printf("  t=%4.1fs rms %.5f (%.1f dB)\n", t, rms, 20.0 * std::log10(std::max(rms, 1e-9)));
    }
    writeWav(argv[1], L, R, sr);
    return 0;
}
