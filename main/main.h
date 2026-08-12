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

constexpr int FFT_WINDOW_SIZE = 2048 , FFT_STEP_SIZE = 512,  // spek values
    MAX_TEXTURE_SIZE = 16384, 
    MARGIN_TOP = 30, MARGIN_BOTTOM = 30, MARGIN_LEFT = 50, MARGIN_RIGHT = 20,
    TEXT_SIZE = 10, TEXT_SPACING = 2;
constexpr char ICON_PATH[] = "resources/icon.png";
Font font;

struct Audio {
    int sampleRate;
    int channels;
    sf_count_t frames;
    // stereo: L0, R0, L1, R1, ...
    std::vector<float> samples;
};

enum Scene {
    SCENE_NONE,
    SCENE_SPECTROGRAM   
};

std::string freq_to_short_string(int frequency) {
    if (frequency >= 1000) {
        return std::to_string(frequency / 1000) + "kHz";
    } else {
        return std::to_string(frequency) + "Hz";
    }
}

void draw_frequency_line(int frequency, int max_frequency, int width, int height);
void draw_spectogram_texture(const std::vector<std::vector<double>>& magnitudes, const RenderTexture2D& texture);
void drawScene(const std::vector<std::vector<double>>& magnitudes, int channels, int sampleRate);

std::optional<Audio> loadAudio(const std::string&);

std::vector<std::vector<std::complex<double>>> perform_full_audio_fft(const std::vector<double>&, int);
std::vector<std::vector<double>> fft_to_normalized(const std::vector<std::vector<std::complex<double>>>&);