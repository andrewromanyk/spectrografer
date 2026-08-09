#include "fftw.h"

std::vector<std::complex<double>> fftw_transform(const std::vector<std::complex<double>>& input) {
    int N = input.size();
    std::vector<std::complex<double>> output(N);

    // Create FFTW plan
    fftw_plan plan = fftw_plan_dft_1d(N,
                                      reinterpret_cast<fftw_complex*>(const_cast<std::complex<double>*>(input.data())),
                                      reinterpret_cast<fftw_complex*>(output.data()),
                                      FFTW_FORWARD, FFTW_ESTIMATE);

    // Execute the plan
    fftw_execute(plan);

    // Clean up
    fftw_destroy_plan(plan);
 
    return output;
}