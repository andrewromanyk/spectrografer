add_library (fft fftw.cpp fftw.h)

target_include_directories (fft PUBLIC .)

target_link_libraries (fft PUBLIC fftw::fftw)