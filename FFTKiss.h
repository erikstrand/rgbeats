#ifndef FFTKISS
#define FFTKISS

#include "kiss_fft.h"
#include "Windows.h"
#include "sqrt_integer.h"

//==============================================================================
// Class FFTKiss
//==============================================================================
/*
 * Simple wrapper for Kiss FFT
 * N is the number of (complex) samples that will be transformed.
 # N must be supported by KISS FFT, and HanningWindow.
 * NORMALIZATION is the normalization factor
 * ifft(fft(data)) = data when NORMALIZATION = sqrt(N)
 *
 */

//------------------------------------------------------------------------------
template <unsigned N, unsigned NORMALIZATION>
class FFTKiss {
private:
  kiss_fft_cfg config;
  kiss_fft_cfg iconfig;
  int16_t buffer[N*2];
  HanningWindow<N> window;

public:
  FFTKiss () {
    config = kiss_fft_alloc(N, 0, NULL, NULL);
    iconfig = kiss_fft_alloc(N, 1, NULL, NULL);
  }
  ~FFTKiss () { free(config); free(iconfig); }

  void applyWindow (int16_t* data) {
    for (unsigned i=0; i<N; ++i) {
      int32_t temp = data[2*i] * window.value[i];
      data[2*i] = temp >> 15;
    }
  }

  void forward (int16_t* data) {
    kiss_fft(config, reinterpret_cast<kiss_fft_cpx*>(data), reinterpret_cast<kiss_fft_cpx*>(buffer));
    for (unsigned i=0; i<2*N; ++i) { buffer[i] *= NORMALIZATION; }
    memcpy(data, buffer, sizeof(int16_t)*N*2);
  }

  void inverse (int16_t* data) {
    kiss_fft(iconfig, reinterpret_cast<kiss_fft_cpx*>(data), reinterpret_cast<kiss_fft_cpx*>(buffer));
    for (unsigned i=0; i<2*N; ++i) { buffer[i] *= NORMALIZATION; }
    memcpy(data, buffer, sizeof(int16_t)*N*2);
  }

  uint32_t magnitude (int16_t* z) {
    uint32_t magsq = uint32_t(z[0]*z[0]) + uint32_t(z[1]*z[1]);
    return sqrt_uint32_approx(magsq);
  }
};

#endif
