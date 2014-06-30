//==============================================================================
// BeatExtractor.hpp
// Created 2014-06-15
//==============================================================================

#ifndef BEATSERAI_PULSEEXTRACTOR
#define BEATSERAI_PULSEEXTRACTOR

//#include <cmath>
#include <arm_math.h>
#include "utility/dspinst.h"
#include "utility/sqrt_integer.h"
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
 * 2*W must be an allowed value for an arm_cfft.
 * SPH is audio samples per HFC sample; used to make a BeatHypothesis in sample space.
 */
template <unsigned W, unsigned SPH>
class BeatExtractor {
public:
  Complex<int16_t> workingMemory[2*W] __attribute__ ((aligned (4)));  // Need twice the window size, so that we can pad with zeros
  Complex<int16_t> workingMemory2[2*W] __attribute__ ((aligned (4))); // Need a second bank for cross correlation calculation
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
  int extractBeat (unsigned endSample);

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
template <unsigned W, unsigned SPH>
arm_cfft_radix4_instance_q15 BeatExtractor<W, SPH>::fft_inst;
template <unsigned W, unsigned SPH>
arm_cfft_radix4_instance_q15 BeatExtractor<W, SPH>::ifft_inst;
template <unsigned W, unsigned SPH>
arm_cfft_radix4_init_q15(&BeatExtractor<W, SPH>::fft_inst, 2*W, 0, 1);
template <unsigned W, unsigned SPH>
arm_cfft_radix4_init_q15(&BeatExtractor<W, SPH>::ifft_inst, 2*W, 1, 1);
*/


//------------------------------------------------------------------------------
template <unsigned W, unsigned SPH>
int BeatExtractor<W, SPH>::extractBeat (unsigned endSample) {
  // TODO: would be more efficient if this function took sampleNumber as an argument
  //beat.measurementSample = SPH*smoothedHFC.counter;
  beat.measurementSample = endSample;

  profiler.call(extractorFrequency);
  calculateAutoCorrelation();
  unsigned hfcsPerBeat = findFrequency();
  profiler.finish(extractorFrequency);

  profiler.call(extractorPhase);
  calculateCrossCorrelation(hfcsPerBeat);
  findPhase(hfcsPerBeat);
  profiler.finish(extractorPhase);
  return 1;
}

//------------------------------------------------------------------------------
template <unsigned W, unsigned SPH>
void BeatExtractor<W, SPH>::calculateAutoCorrelation () {
    // Compute forward FFT
    Complex<int16_t>::zero(&workingMemory[W], W);
    arm_cfft_radix4_q15(&fft_inst, reinterpret_cast<int16_t*>(workingMemory));
    // copy data to our second buffer, so HFC transform can be used later
    memcpy(workingMemory2, workingMemory, 2*W*sizeof(Complex<int16_t>));

    // Compute power spectrum
    // Could apply low-pass filter here
    powerSpectrum(workingMemory2, 2*W);
    //Serial.print("power spectrum: ");
    for (int i=0; i < 2*W; ++i) {
      uint32_t tmp = *reinterpret_cast<uint32_t *>(workingMemory2 + i); // real & imag
      uint32_t magsq = multiply_16tx16t_add_16bx16b(tmp, tmp);
      uint32_t mag = sqrt_uint32_approx(magsq);
      workingMemory2[i].re() = static_cast<uint16_t>(mag);
      //Serial.print(workingMemory2[i].re());
      //Serial.print(", ");
      workingMemory2[i].im() = 0;
    }
    //Serial.println();

    // Compute inverse FFT
    arm_cfft_radix4_q15(&ifft_inst, reinterpret_cast<int16_t*>(workingMemory2));

    // Normalize
    //Serial.print("Autocorrelation: ");
    //Serial.println();
    for (int32_t i=0; i<static_cast<int32_t>(W/2); ++i) {
      int32_t scaled = (int32_t)W/(W - i) * (int32_t)workingMemory2[i].re();
      *reinterpret_cast<int32_t*>(workingMemory2 + i) = scaled;
      //Serial.println(scaled);
    }
    for (int32_t i=W/2; i<static_cast<int32_t>(3*W/2); ++i) {
      int32_t scaled = (int32_t)workingMemory2[i].re();
      *reinterpret_cast<int32_t*>(workingMemory2 + i) = scaled;
      //Serial.println(scaled);
    }
    for (int32_t i=3*W/2; i<static_cast<int32_t>(2*W); ++i) {
      int32_t scaled = (int32_t)W/(i - W) * (int32_t)workingMemory2[i].re();
      *reinterpret_cast<int32_t*>(workingMemory2 + i) = scaled;
      //Serial.println(scaled);
    }
    //Serial.println();
}

//------------------------------------------------------------------------------
template <unsigned W, unsigned SPH>
unsigned BeatExtractor<W, SPH>::findFrequency () {
  int32_t* workingMemory32 = reinterpret_cast<int32_t*>(workingMemory2);
  unsigned twoi;

  // Pass autocorrelation function through a comb filter.
  // overflow possibility - could fix by using in place 32 bit copy (correctly combine real and complex parts first)
  // No need to let i reach 1, as this represents a tempo so fast
  // that it's beyond the Nyquist frequency of our HFC samplerate.
  for (unsigned i=W/2-1; i>1; --i) {
    twoi = i<<1;
    workingMemory32[i] += workingMemory32[twoi];
    for (unsigned j=twoi+i; j<W; j+=twoi) {
      workingMemory32[i] += workingMemory32[j];
    }
  }

  // normalize and find global maximum
  // overflow possibility - would be mitigated by 32 bit idea mentioned above
  //Serial.println("=========================================================================");
  workingMemory32[2] *= 2;
  //Serial.println(workingMemory32[2]);
  int32_t hfcsPerBeat = 2; // will end up equal to the largest normalized value in workingMemory2
  int32_t maxvalue = workingMemory32[2];
  for (int32_t i=3; i<W; ++i) {
    workingMemory32[i] *= W/((W + 1) / i);
    //Serial.println(workingMemory32[i]);
    if (workingMemory32[i] > maxvalue) {
      maxvalue = workingMemory32[i];
      hfcsPerBeat = i;
    }
  }
  //Serial.println();

  // Find the largest multiple of this maximum that's still in our window.
  int32_t peakthreshold = maxvalue / 2; // half of what we've already found
  int32_t searchWindow = W;
  int32_t beatsPerWindow; // largest integer such that beatsPerWindow*hfcsPerBeat is in the window
  int32_t lastPeak;       // largest multiple in the window (== beatsPerWindow*hfcsPerBeat)
  int32_t newPeakIndex;
  float subsampleHfcsPerBeat = static_cast<float>(hfcsPerBeat);
  while (true) {
    beatsPerWindow = static_cast<int32_t>(searchWindow) / hfcsPerBeat;
    if (beatsPerWindow <= 1) {
      break;
    }
    lastPeak = hfcsPerBeat * beatsPerWindow;
    newPeakIndex = lastPeak-5;
    int32_t newPeak = workingMemory32[newPeakIndex];
    for (int32_t i=newPeakIndex-4; i<lastPeak+5; ++i) {
      if (workingMemory32[i] > newPeak) {
        newPeak = workingMemory32[i];
        newPeakIndex = i;
      }
    }
    if (newPeak > peakthreshold) {
      //std::cout << "Located peak at " << newPeakIndex << " (" << beatsPerWindow << " beats out)" << '\n';
      subsampleHfcsPerBeat = static_cast<float>(newPeakIndex) / static_cast<float>(beatsPerWindow);
      break;
    } else {
      searchWindow /= 2;
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
template <unsigned W, unsigned SPH>
void BeatExtractor<W, SPH>::calculateCrossCorrelation (unsigned hfcsPerBeat) {
  // Prepare impulse train in workingMemory2.
  // In order to lock phase as little back in time as possible,
  // our impulse train hypothesizes a beat at the last HFC sample.
  // We will use a reverse time cross correlation to find how far
  // back this train needs to be pushed to really match up.
  Complex<int16_t>::zero(workingMemory2, 2*W);
  for (unsigned i=W-1; i<W; i-=hfcsPerBeat) {
    workingMemory2[i].re() = 8192;
    //workingMemory2[i+1].re() = T(0.5);
  }
  // calculate the fft of the impulse train
  arm_cfft_radix4_q15(&fft_inst, reinterpret_cast<int16_t*>(workingMemory2));
  //fft(workingMemory2, 2*W, false);
  // multiply the conjugate of FFT(HFC) into FFT(impulse train)
  //Serial.println("FFT zone");
  for (unsigned i=0; i<2*W; ++i) {
    workingMemory2[i] *= workingMemory[i].conjugate();
    //Serial.println(workingMemory2[i].re());
  }
  //Serial.println();
  // calculate the inverse FFT to get the cross correlation
  arm_cfft_radix4_q15(&ifft_inst, reinterpret_cast<int16_t*>(workingMemory2));
  //fft(workingMemory2, 2*W, true);
}

//------------------------------------------------------------------------------
template <unsigned W, unsigned SPH>
void BeatExtractor<W, SPH>::findPhase (unsigned hfcsPerBeat) {
  // Find the maximum cross correlation in one beat period
  unsigned maxindex = 0;
  int16_t maxvalue = workingMemory2[0].re();
  //Serial.println("=========================================================================");
  //Serial.println("cross correlation");
  for (unsigned i=1; i<hfcsPerBeat; ++i) {
    //Serial.println(workingMemory2[i].re());
    if (workingMemory2[i].re() > maxvalue) {
      maxvalue = workingMemory2[i].re();
      maxindex = i;
    }
  }
  //Serial.println();

  // store the location of the last beat (in HFC samples)
  beat.anchorSample = SPH*(beat.measurementSample - 1 - maxindex);
}

//------------------------------------------------------------------------------
/*
template <unsigned W, unsigned SPH>
void BeatExtractor<W, SPH>::saveWorkingMemory (char const* name, bool bank2) const {
  std::ofstream outfile(name);
  Complex<T> const* bank = bank2 ? workingMemory2 : workingMemory;
  for (unsigned i=0; i<2*W; ++i) {
    outfile << i << ',' << bank[i].re() << '\n';
  }
  outfile.close();
}
*/

#endif // BEATSERAI_PULSEEXTRACTOR

