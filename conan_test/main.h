#include <print>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
#include <optional>
#include <numbers>
#include <algorithm>
#include <ranges>

#include "raylib.h"
#include <sndfile.h>
#include "fftw.h"

constexpr int FFT_WINDOW_SIZE = 2048 , FFT_STEP_SIZE = 512; // spek values
constexpr char ICON_PATH[] = "resources/icon.png";

struct Audio {
    int sampleRate;
    int channels;
    sf_count_t frames;
    // stereo: L0, R0, L1, R1, ...
    std::vector<float> samples;
};


void draw_spectogram_texture(const std::vector<std::vector<double>>& magnitudes, const RenderTexture2D& texture);
void drawScene(const std::vector<std::vector<double>>& magnitudes, int channels, int sampleRate);

Audio loadAudio(const std::string&);

std::vector<std::vector<std::complex<double>>> perform_full_audio_fft(const std::vector<double>&, int);
std::vector<std::vector<double>> fft_to_normalized(const std::vector<std::vector<std::complex<double>>>&);