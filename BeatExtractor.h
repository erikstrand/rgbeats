//==============================================================================
// BeatExtractor.hpp
// Created 2014-06-15
//==============================================================================

#ifndef BEATSERAI_PULSEEXTRACTOR
#define BEATSERAI_PULSEEXTRACTOR

#include <cmath>
#include <arm_math.h>
#include "Complex.h"
#include "FFT.h"
#include "utils.h"
#include "RingBuffer.h"
//#include <fstream>

#include "esProfiler.h"
extern Profiler profiler;



//==============================================================================
// BeatExtractor
//==============================================================================

//------------------------------------------------------------------------------
/*
 * Takes HFC samples as input, and guesses the frequency and phase of the dominant pulse.
 *
 * T must be zeroable with memset and copyable with memcpy.
 * Smooths HFC samples with trailing median of M samples.
 * Calculates autocorrelation on windows of W samples, with a hop size of S samples.
 * 2*W must be an allowed value for an arm_cfft.
 * S is assumed to divide ~0+1 (one larger than the largest unsigned).
 * SPH is audio samples per HFC sample; used to make a BeatHypothesis in sample space.
 */
template <unsigned M, unsigned W, unsigned S, unsigned SPH>
class BeatExtractor {
public:
  RingBufferWithMedian<int16_t, M> rawHFC;     // Unaltered HFC samples
  RingBuffer<int16_t, W> smoothedHFC;   // HFC series with trailing median subtracted
  Complex<int16_t> workingMemory[2*W];  // Need twice the window size, so that we can pad with zeros
  Complex<int16_t> workingMemory2[2*W]; // Need a second bank for cross correlation calculation
  BeatHypothesis beat;                  // Most recent guess at the pulse.
  // should be static const...
  arm_cfft_radix4_instance_q15 fft_inst;
  arm_cfft_radix4_instance_q15 ifft_inst;

public:
  inline BeatExtractor () {
    arm_cfft_radix4_init_q15(&fft_inst, 2*W, 0, 1);
    arm_cfft_radix4_init_q15(&ifft_inst, 2*W, 1, 1);
  }

  // Adds an HFC sample to rawHFC and smoothedHFC.
  // If we have collected S new HFC samples, a new tempo hypothesis is generated and it returns 1.
  // Otherwise, it returns 0 without updating its BeatHypothesis.
  // Sets beat.measurementSample.
  int addSample (int16_t x);

  // Calculates the autocorrelation of the most recent W samples in smoothedHFC.
  // The FFT of the last W HFC samples is left in workingMemory,
  // and the autocorrelation is left in workingMemory2.
  void calculateAutoCorrelation ();
  // Uses the autocorrelation information in workingMemory2 to guess the tempo.
  // It sets beat.samplesPerBeat, and returns the number of HFC samples per beat.
  unsigned findFrequency ();
  // Calculates the cross correation of the most recent W samples in smoothedHFC
  // with an impulse train at the guessed tempo.
  // The FFT of the last W HFC samples is found and left in workingMemory,
  // and the cross correlation is left in workingMemory2.
  void calculateCrossCorrelation (unsigned hfcsPerBeat);
  // Uses the cross correlation information in workingMemory2 to guess the location of the most recent beat.
  // The argument is used to determine how wide a range to search for the best alignment.
  // Sets beat.anchorSample.
  void findPhase (unsigned hfcsPerBeat);

  // Writes a csv containing the real parts of the contents of workingMemory(2).
  //void saveWorkingMemory (char const* name, bool bank2=false) const;
};

//------------------------------------------------------------------------------
/*
template <unsigned M, unsigned W, unsigned S, unsigned SPH>
arm_cfft_radix4_instance_q15 BeatExtractor<M, W, S, SPH>::fft_inst;
template <unsigned M, unsigned W, unsigned S, unsigned SPH>
arm_cfft_radix4_instance_q15 BeatExtractor<M, W, S, SPH>::ifft_inst;
template <unsigned M, unsigned W, unsigned S, unsigned SPH>
arm_cfft_radix4_init_q15(&BeatExtractor<M, W, S, SPH>::fft_inst, 2*W, 0, 1);
template <unsigned M, unsigned W, unsigned S, unsigned SPH>
arm_cfft_radix4_init_q15(&BeatExtractor<M, W, S, SPH>::ifft_inst, 2*W, 1, 1);
*/


//------------------------------------------------------------------------------
template <unsigned M, unsigned W, unsigned S, unsigned SPH>
int BeatExtractor<M, W, S, SPH>::addSample (int16_t x) {
  // debug
  //std::cout << "BeatExtractor adding sample " << x << "\n";
  profiler.call(extractorAddSample);

  rawHFC.addSample(x);
  smoothedHFC.addSample(x - rawHFC.median());
  if (isMultipleOf<S>(smoothedHFC.counter)) {
    // TODO: would be more efficient if this function took sampleNumber as an argument
    beat.measurementSample = SPH*smoothedHFC.counter;

    profiler.call(extractorFrequency);
    calculateAutoCorrelation();
    unsigned hfcsPerBeat = findFrequency();
    profiler.finish(extractorFrequency);

    profiler.call(extractorPhase);
    calculateCrossCorrelation(hfcsPerBeat);
    findPhase(hfcsPerBeat);
    profiler.finish(extractorPhase);
    profiler.finish(extractorAddSample);
    return 1;
  }

  profiler.finish(extractorAddSample);
  return 0;
}

//------------------------------------------------------------------------------
template <unsigned M, unsigned W, unsigned S, unsigned SPH>
void BeatExtractor<M, W, S, SPH>::calculateAutoCorrelation () {
    smoothedHFC.copySamples(workingMemory, W);
    Complex<int16_t>::zero(&workingMemory[W], W);
    arm_cfft_radix4_q15(&fft_inst, reinterpret_cast<int16_t*>(workingMemory));
    //fft(workingMemory, 2*W, false);
    memcpy(workingMemory2, workingMemory, 2*W*sizeof(Complex<int16_t>));
    // Could apply low-pass filter here
    powerSpectrum(workingMemory2, 2*W);
    arm_cfft_radix4_q15(&ifft_inst, reinterpret_cast<int16_t*>(workingMemory));
    //fft(workingMemory2, 2*W, true);
    int16_t maxcorrelation = workingMemory2[0].re();
    // normalize
    for (int16_t i=0; i<static_cast<int16_t>(2*W); ++i) {
      //workingMemory2[i].re() /= maxcorrelation * 2*W/(2*W - i);
      workingMemory2[i].re() *= 2*W/(2*W - i);
    }
}

//------------------------------------------------------------------------------
template <unsigned M, unsigned W, unsigned S, unsigned SPH>
unsigned BeatExtractor<M, W, S, SPH>::findFrequency () {
  unsigned twoi;
  // Pass autocorrelation function through a comb filter.
  // overflow possibility - could fix by using in place 32 bit copy (correctly combine real and complex parts first)
  // No need to let i reach 1, as this represents a tempo so fast
  // that it's beyond the Nyquist frequency of our HFC samplerate.
  for (unsigned i=W/2-1; i>1; --i) {
    twoi = i<<1;
    workingMemory2[i].re() += workingMemory2[twoi].re();
    for (unsigned j=twoi+i; j<W; j+=twoi) {
      workingMemory2[i].re() += workingMemory2[j].re();
    }
  }

  // normalize and find global maximum
  // overflow possibility - would be mitigated by 32 bit idea mentioned above
  workingMemory2[2].re() *= 2;
  int16_t hfcsPerBeat = 2; // will end up equal to the largest normalized value in workingMemory2
  int16_t maxvalue = workingMemory2[2].re();
  for (int16_t i=3; i<W; ++i) {
    workingMemory2[i].re() *= i;
    if (workingMemory2[i].re() > maxvalue) {
      maxvalue = workingMemory2[i].re();
      hfcsPerBeat = i;
    }
  }

  // Find the largest multiple of this maximum that's still in our window.
  int16_t peakthreshold = workingMemory2[0].re() / 2; // half of perfect correlation
  unsigned searchWindow = W; // unsigned so that we can divide by shifting
  int16_t beatsPerWindow; // largest integer such that beatsPerWindow*hfcsPerBeat is in the window
  int16_t lastPeak;       // largest multiple in the window (== beatsPerWindow*hfcsPerBeat)
  int16_t newPeakIndex;
  float subsampleHfcsPerBeat = static_cast<float>(hfcsPerBeat);
  while (true) {
    beatsPerWindow = static_cast<int16_t>(searchWindow) / hfcsPerBeat;
    lastPeak = hfcsPerBeat * beatsPerWindow;
    newPeakIndex = lastPeak-5;
    int16_t newPeak = workingMemory2[newPeakIndex].re();
    for (int16_t i=newPeakIndex-4; i<lastPeak+5; ++i) {
      if (workingMemory2[i].re() > newPeak) {
        newPeak = workingMemory2[i].re();
        newPeakIndex = i;
      }
    }
    if (newPeak > peakthreshold) {
      //std::cout << "Located peak at " << newPeakIndex << " (" << beatsPerWindow << " beats out)" << '\n';
      subsampleHfcsPerBeat = static_cast<float>(newPeakIndex) / static_cast<float>(beatsPerWindow);
      break;
    } else {
      searchWindow >>= 1;
      if (searchWindow <= (W>>2)) {
        break;
      }
    }
  }

  // At 44100kHz, 64-32 HFC samples per beat corresponds to 86.1 to 172.2 bpm
  while (subsampleHfcsPerBeat > 50.0) { subsampleHfcsPerBeat *= 0.5; }
  while (subsampleHfcsPerBeat <= 25.0) { subsampleHfcsPerBeat *= 2.0; }
  hfcsPerBeat = static_cast<unsigned>(subsampleHfcsPerBeat + 0.5);
  // record the final value (in audio samples)
  beat.samplesPerBeat = static_cast<unsigned>(static_cast<float>(SPH)*subsampleHfcsPerBeat + 0.5);
  // return the final value (in HFC samples)
  return hfcsPerBeat;
}

//------------------------------------------------------------------------------
template <unsigned M, unsigned W, unsigned S, unsigned SPH>
void BeatExtractor<M, W, S, SPH>::calculateCrossCorrelation (unsigned hfcsPerBeat) {
  // Prepare impulse train in workingMemory2.
  // In order to lock phase as little back in time as possible,
  // our impulse train hypothesizes a beat at the last HFC sample.
  // We will use a reverse time cross correlation to find how far
  // back this train needs to be pushed to really match up.
  Complex<int16_t>::zero(workingMemory2, 2*W);
  for (unsigned i=W-1; i<W; i-=hfcsPerBeat) {
    workingMemory2[i].re() = 1;
    //workingMemory2[i+1].re() = T(0.5);
  }
  // calculate the fft of the impulse train
  arm_cfft_radix4_q15(&fft_inst, reinterpret_cast<int16_t*>(workingMemory));
  //fft(workingMemory2, 2*W, false);
  // multiply the conjugate of FFT(HFC) into FFT(impulse train)
  for (unsigned i=0; i<2*W; ++i) {
    workingMemory2[i] *= workingMemory[i].conjugate();
  }
  // calculate the inverse FFT to get the cross correlation
  arm_cfft_radix4_q15(&ifft_inst, reinterpret_cast<int16_t*>(workingMemory));
  //fft(workingMemory2, 2*W, true);
}

//------------------------------------------------------------------------------
template <unsigned M, unsigned W, unsigned S, unsigned SPH>
void BeatExtractor<M, W, S, SPH>::findPhase (unsigned hfcsPerBeat) {
  // Find the maximum cross correlation in one beat period
  unsigned maxindex = 0;
  int16_t maxvalue = workingMemory2[0].re();
  for (unsigned i=1; i<hfcsPerBeat; ++i) {
    if (workingMemory2[i].re() > maxvalue) {
      maxvalue = workingMemory2[i].re();
      maxindex = i;
    }
  }

  // store the location of the last beat (in HFC samples)
  beat.anchorSample = SPH*(smoothedHFC.counter - 1 - maxindex);
}

//------------------------------------------------------------------------------
/*
template <unsigned M, unsigned W, unsigned S, unsigned SPH>
void BeatExtractor<M, W, S, SPH>::saveWorkingMemory (char const* name, bool bank2) const {
  std::ofstream outfile(name);
  Complex<T> const* bank = bank2 ? workingMemory2 : workingMemory;
  for (unsigned i=0; i<2*W; ++i) {
    outfile << i << ',' << bank[i].re() << '\n';
  }
  outfile.close();
}
*/

#endif // BEATSERAI_PULSEEXTRACTOR

