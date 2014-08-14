/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef audio_analyze_hfc_onset
#define audio_analyze_hfc_onset

#include "AudioStream.h"
#include "RingBuffer.h"

#include "esProfiler.h"
extern Profiler profiler;

// windows.c
extern "C" {
  extern const int16_t AudioWindowHanning1024[];
  extern const int16_t AudioWindowBartlett1024[];
  extern const int16_t AudioWindowBlackman1024[];
  extern const int16_t AudioWindowFlattop1024[];
  extern const int16_t AudioWindowBlackmanHarris1024[];
  extern const int16_t AudioWindowNuttall1024[];
  extern const int16_t AudioWindowBlackmanNuttall1024[];
  extern const int16_t AudioWindowWelch1024[];
  extern const int16_t AudioWindowHamming1024[];
  extern const int16_t AudioWindowCosine1024[];
  extern const int16_t AudioWindowTukey1024[];
}

class AudioAnalyzeHfcOnset : public AudioStream {
private:
  int16_t const* window; // points to precomputed window function
  audio_block_t* blocklist[8]; // stores pointers to blocks provided by Audio library
  int16_t buffer[2048] __attribute__ ((aligned (4))); // working memory for fft calculation
  uint8_t state; // controls which blocklist pointer will be used next
  volatile bool outputflag;
  audio_block_t* inputQueueArray[1]; // only one audio input channel
  const unsigned refractorySamples = 15;

public:
  // Unaltered HFC samples. No need to store past state, so we only store the moving average window.
  RingBufferWithMedian<int32_t, 32> rawHFC;
  // HFC with trailing median subtracted. We store 2 times the Beat Extraction window.
  RingBuffer<int16_t, 1024> smoothedHFC;
  volatile unsigned sampleNumber;
  volatile uint16_t output[512] __attribute__ ((aligned (4)));
  unsigned hfcWindowEnd;
  unsigned hfcWindowEndSample;
  volatile unsigned samplesSinceLastOnset;
  volatile unsigned lastOnsetSignificance;
  volatile unsigned lastOnsetMaxSignificance;

public:
  AudioAnalyzeHfcOnset(uint8_t navg = 1, const int16_t *win = AudioWindowHanning1024):
    AudioStream(1, inputQueueArray),
    window(win),
    state(0),
    outputflag(false),
    sampleNumber(0),
    hfcWindowEnd(0),
    samplesSinceLastOnset(refractorySamples+1),
    lastOnsetSignificance(0),
    lastOnsetMaxSignificance(0)
  {
    init();
  }

  void init(void);
  inline bool available () {
    if (outputflag == true) {
      outputflag = false;
      return true;
    }
    return false;
  }
  virtual void update(void);

};

#endif
