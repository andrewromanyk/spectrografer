#include "main.h"

using std::println;

std::string current_file_path;
std::optional<Audio> audio;
std::vector<std::vector<std::complex<double>>> fftResult;
std::vector<std::vector<double>> magnitudes;

std::optional<RenderTexture2D> spectogram_texture;

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(800, 600, "Spectrografer");
    SetTargetFPS(60);

    SetWindowMinSize(500, 300);

    Image icon = LoadImage(ICON_PATH);
        SetWindowIcon(icon);
    UnloadImage(icon);

    while (!WindowShouldClose()) {
        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();

            current_file_path = droppedFiles.paths[0]; // Store the first dropped file path

            UnloadDroppedFiles(droppedFiles); // Clear memory

            audio = loadAudio(current_file_path);

            fftResult = perform_full_audio_fft({audio->samples.begin(), audio->samples.end()}, audio->channels);

            magnitudes = fft_to_normalized(fftResult);
            
            UnloadRenderTexture(*spectogram_texture);
            spectogram_texture.reset();
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
        draw_spectogram_texture(magnitudes, *spectogram_texture);
    }

    BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(
            spectogram_texture->texture, 
            {0, 0, static_cast<float>(spectogram_texture->texture.width), static_cast<float>(spectogram_texture->texture.height)}, 
            {10, 10, static_cast<float>(width - 20), static_cast<float>(height - 20)}, 
            {0, 0}, 
            0.0f, 
            WHITE
        );

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

std::vector<std::vector<std::complex<double>>> perform_full_audio_fft(const std::vector<double>& audio, int channels) {
    std::vector<double> mono_audio;
    if (channels != 1) {
        // take first channel only for now
        mono_audio.reserve(audio.size() / channels);
        for (size_t i = 0; i < audio.size(); i += channels) {
            mono_audio.push_back(audio[i]);
        }
    } else {
        mono_audio = audio;
    }

    size_t numWindows = (mono_audio.size() + FFT_STEP_SIZE - 1) / FFT_STEP_SIZE; // ceil

    std::vector<std::vector<std::complex<double>>> fftResults;

    for (size_t i = 0; i < numWindows; i++) {
        auto startIdx = mono_audio.begin() + i * FFT_STEP_SIZE;
        auto endIdx = std::min(startIdx + FFT_WINDOW_SIZE, mono_audio.end());
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
    return audio_data | std::views::transform([](const auto& window) {
        return window | std::views::take(FFT_WINDOW_SIZE / 2)
                | std::views::transform([](const auto& sample) {
                    return std::abs(sample) * 4 / FFT_WINDOW_SIZE;
                })
                | std::views::transform([](double scaled) {
                    return 20 * std::log10(scaled + 1e-6);
                })
                | std::views::transform([](double db) {
                    return (std::clamp(db, -100.0, 0.0) + 100.0) / 100.0;
                }) | std::ranges::to<std::vector>();
    }) | std::ranges::to<std::vector>();
}

void draw_spectogram_texture(const std::vector<std::vector<double>>& magnitudes, const RenderTexture2D& texture) {
    double ratio = 1;
    int width = static_cast<int>(magnitudes.size());
    if (magnitudes.size() > MAX_TEXTURE_SIZE) {
        width = MAX_TEXTURE_SIZE;
        ratio = magnitudes.size() / static_cast<double>(MAX_TEXTURE_SIZE);
        println("Spectrogram width exceeds maximum texture size. ratio: {}", ratio);
    }
    
    spectogram_texture = LoadRenderTexture(width, FFT_WINDOW_SIZE / 2);

    BeginTextureMode(texture);
        BeginDrawing();
            ClearBackground(BLACK);

            for (auto [column, time, time_d] = std::tuple<size_t, size_t, double>{0, 0, 0.0}; column < width; column++, time++, time_d += ratio) {
                int time_overflow = std::floor(time_d);
                for (size_t j = 0; j < FFT_WINDOW_SIZE / 2; ++j) {
                    double magnitude = magnitudes[time][j];
                    if (time_overflow > time) {
                        for (int k = time + 1; k < time_overflow; ++k) {
                            magnitude += magnitudes[k][j];
                        }
                        magnitude /= (time_overflow - time);
                    }

                    Color color = 
                    (magnitude <= 0.0)  ? BLACK :
                    (magnitude <= 0.17) ? ColorLerp(BLACK, {0, 0, 100, 255}, magnitude / 0.17) :
                    (magnitude <= 0.33) ? ColorLerp({0, 0, 100, 255}, {120, 0, 150, 255}, (magnitude - 0.17) / 0.16) :
                    (magnitude <= 0.50) ? ColorLerp({120, 0, 150, 255}, {220, 0, 100, 255}, (magnitude - 0.33) / 0.17) :
                    (magnitude <= 0.67) ? ColorLerp({220, 0, 100, 255}, {255, 0, 0, 255}, (magnitude - 0.50) / 0.17) :
                    (magnitude <= 0.83) ? ColorLerp({255, 0, 0, 255}, {255, 180, 0, 255}, (magnitude - 0.67) / 0.16) :
                    ColorLerp({255, 180, 0, 255}, WHITE, (magnitude - 0.83) / 0.17);

                    DrawPixel(column, j, color); // because stupid opengl framebuffer origin is bottom-left, not top-left like raylib
                }
                time = time_overflow;
            }
        EndDrawing();
    EndTextureMode();
}