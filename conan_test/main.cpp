#include "main.h"

using std::println;

std::string current_file_path;
std::optional<Audio> audio;
std::vector<std::vector<std::complex<double>>> fftResult;
std::vector<std::vector<double>> magnitudes;

std::optional<RenderTexture2D> spectogram_texture;

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(800, 600, "Raylib Example");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();

            current_file_path = droppedFiles.paths[0]; // Store the first dropped file path

            UnloadDroppedFiles(droppedFiles); // Clear memory

            audio = loadAudio(current_file_path);

            fftResult = performFFT({audio->samples.begin(), audio->samples.end()}, audio->channels);

            magnitudes = fft_to_normalized(fftResult);
        }

        drawScene(magnitudes, audio ? audio->channels : 0, audio ? audio->sampleRate : 0);
    }

    CloseWindow();
    return 0;
}

void drawScene(const std::vector<std::vector<double>>& magnitudes, int channels, int sampleRate) {
    size_t width = GetScreenWidth();
    size_t height = GetScreenHeight();
    
    if (!spectogram_texture && !magnitudes.empty()) {
        spectogram_texture = LoadRenderTexture(magnitudes.size(), FFT_WINDOW_SIZE / 2);

        BeginTextureMode(*spectogram_texture);
            BeginDrawing();
                for (size_t i = 0; i < magnitudes.size(); ++i) {
                    for (size_t j = 0; j < magnitudes[i].size() / 2; ++j) {
                        double magnitude = magnitudes[i][j];
                        Color color = {static_cast<unsigned char>((1 - magnitude) * 255), 0, static_cast<unsigned char>(magnitude * 255), 255};
                        DrawPixel(i, j, color);
                    }
                }
            EndDrawing();
        EndTextureMode();
    }

    BeginDrawing();

        DrawTexturePro(spectogram_texture->texture, {0, 0, static_cast<float>(spectogram_texture->texture.width), static_cast<float>(spectogram_texture->texture.height)}, {0, 0, static_cast<float>(width), static_cast<float>(height)}, {0, 0}, 0.0f, WHITE);

    EndDrawing();
}

Audio loadAudio(const std::string& path)
{
    SF_INFO info{};
    SNDFILE* file = sf_open(path.c_str(), SFM_READ, &info);

    if (!file) {
        throw std::runtime_error(
            "Cannot open audio file: " +
            std::string(sf_strerror(nullptr))
        );
    }

    Audio audio{
        .sampleRate = info.samplerate,
        .channels = info.channels,
        .frames = info.frames,
        .samples = std::vector<float>(
            static_cast<std::size_t>(info.frames) * info.channels
        )
    };

    const sf_count_t readFrames =
        sf_readf_float(file, audio.samples.data(), info.frames);

    sf_close(file);

    if (readFrames != info.frames) {
        throw std::runtime_error("Could not read the complete audio file");
    }

    return audio;
}

std::vector<std::vector<std::complex<double>>> performFFT(const std::vector<double>& audio, int channels) {
    if (channels != 1) {
        throw std::runtime_error("Only mono audio is supported for FFT");
    }

    println("Performing FFT on audio data of size: {}", audio.size());

    size_t numWindows = (audio.size() + FFT_STEP_SIZE - 1) / FFT_STEP_SIZE; // ceil

    println("Number of FFT windows: {}", numWindows);

    std::vector<std::vector<std::complex<double>>> fftResults;

    for (size_t i = 0; i < numWindows; i++) {
        auto startIdx = audio.begin() + i * FFT_STEP_SIZE;
        auto endIdx = std::min(startIdx + FFT_WINDOW_SIZE, audio.end());
        std::vector<std::complex<double>> window(FFT_WINDOW_SIZE, 0.0);
        std::copy(startIdx, endIdx, window.begin());
        for (size_t j = 0; j < window.size(); j++) {
            double falloff = 0.5 * (1 - std::cos(2 * std::numbers::pi * j / (window.size() - 1))); // Hann window
            window[j] *= falloff;
        }
        fftResults.push_back(fftw_transform(window));
    }

    return fftResults;
}

std::vector<std::vector<double>> fft_to_normalized(const std::vector<std::vector<std::complex<double>>>& audio_data) {
    std::vector<std::vector<double>> normalized;
    
    for (size_t i = 0; i < audio_data.size(); ++i) {
        const auto& window = audio_data[i];
        std::vector<double> window_normalized;
        for (auto& sample : window) {
            double magnitude = std::abs(sample);
            // println("Magnitude: {}", magnitude);
            double db = 20 * std::log10(magnitude + 1e-10); // Avoid log(0)
            double normalized_db = (db + 100) / 100; // Normalize to [0, 1]
            window_normalized.push_back(normalized_db);
        }
        normalized.push_back(window_normalized);
    }

    return normalized;
}