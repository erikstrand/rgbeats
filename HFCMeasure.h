#ifndef HFCMEASURE
#define HFCMEASURE

#include "RingBuffer.h"


//==============================================================================
// Class HFCMeasure
//==============================================================================
/*
 * Simply used to calculate HFC measure.
 * It takes a W long array of complex numbers and returns an int HFC value.
 * Complex numbers are expected to be one int16_t each for real and imaginary parts.
 *
 * This class needs to know:
 * W: number of audio samples per FFT window (and thus per HFC sample)
 * FFT: some type that has an applyWindow method and a forward method
 */

//------------------------------------------------------------------------------
template <unsigned W, typename FFT>
class HFCMeasure {
private:
  FFT fftmachine;

public:
  // returns the hfc sample, and stores the spectrum in spectrum
  inline int32_t calculateSpectrumAndHFC (int16_t* buffer, uint16_t* spectrum);
};

//------------------------------------------------------------------------------
template <unsigned W, typename FFT>
int32_t HFCMeasure<W, FFT>::calculateSpectrumAndHFC (int16_t* buffer, uint16_t* spectrum) {
  fftmachine.applyWindow(buffer);
  fftmachine.forward(buffer);

  int32_t hfc = 0;
  for (int i=0; i < W; i++) {
    uint32_t magnitude = fftmachine.magnitude(buffer + 2*i);
    spectrum[i] = static_cast<uint16_t>(magnitude); // willl overflow magnitudes >= 2^16
    hfc += i*magnitude;
  }
  return hfc;
}


//==============================================================================
// Class HFCRecord
//==============================================================================
/*
 * Stores two circular buffers of HFC values.
 * One is used to calculate a trailing median, and the other stores values
 * with the trailing median removed. The first also determines the window
 * for the detection of outliers aka onsets.
 */

//------------------------------------------------------------------------------
struct OnsetState {
public:
  unsigned samplesSinceLastOnset;
  unsigned lastOnsetInitialSignificance;
  unsigned lastOnsetMaxSignificance;

public:
  OnsetState ():
    // hack: large enough to mean no onset, small enough to not overflow
    samplesSinceLastOnset(1<<15),
    lastOnsetInitialSignificance(0),
    lastOnsetMaxSignificance(0)
    {}
};

//------------------------------------------------------------------------------
template <unsigned RAW_N, unsigned SMOOTHED_N>
class HFCRecord {
public:
  // Unaltered HFC samples. This buffer is used to calculate the trailing median.
  RingBufferWithMedian<RAW_N> rawHFC;
  // HFC with trailing median subtracted.
  // SMOOTHED_N is generally 2 times the beat extraction window to allow room for 0 padding.
  RingBuffer<int16_t, SMOOTHED_N> smoothedHFC;

  // Onset detection paramters and state
  unsigned onsetThreshold;
  unsigned refractorySamples;
  OnsetState onsetState;

public:
  inline HFCRecord (unsigned oT = 2, unsigned rS = 15): onsetThreshold(oT), refractorySamples(rS) {}
  unsigned addSample (int32_t sample);
};

//------------------------------------------------------------------------------
template <unsigned RAW_N, unsigned SMOOTHED_N>
unsigned HFCRecord<RAW_N, SMOOTHED_N>::addSample (int32_t sample) {
  rawHFC.addSample(sample);

  // perform onset calculations
  int32_t median = rawHFC.median();
  int32_t sdev = rawHFC.stddeviation();
  sample -= median;
  int32_t stddevs = sample / sdev;
  if (onsetState.samplesSinceLastOnset > refractorySamples) {
    if (stddevs >= onsetThreshold) {
      onsetState.samplesSinceLastOnset = 0;
      onsetState.lastOnsetInitialSignificance = stddevs;
      onsetState.lastOnsetMaxSignificance = stddevs;
    }
  } else {
    if (stddevs > onsetState.lastOnsetMaxSignificance) {
      onsetState.lastOnsetMaxSignificance = stddevs;
    }
    ++onsetState.samplesSinceLastOnset;
  }

  return rawHFC.samples();
}

#endif

