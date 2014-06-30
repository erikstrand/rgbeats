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

#include "AudioAnalyzeHfcOnset.h"
#include "utility/sqrt_integer.h"
#include "utility/dspinst.h"
#include "arm_math.h"

// TODO: this should be a class member, so more than one FFT can be used
static arm_cfft_radix4_instance_q15 fft_inst;

void AudioAnalyzeHfcOnset::init(void)
{
  // TODO: replace this with static const version
  arm_cfft_radix4_init_q15(&fft_inst, 1024, 0, 1);
  //state = 0;
  //outputflag = false;
}

// 140312 - PAH - slightly faster copy
static void copy_to_fft_buffer(void *destination, const void *source)
{
  const uint16_t *src = (const uint16_t *)source;
  uint32_t *dst = (uint32_t *)destination;

  for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
    *dst++ = *src++;  // real sample plus a zero for imaginary
  }
}

static void apply_window_to_fft_buffer(void *buffer, const void *window)
{
  int16_t *buf = (int16_t *)buffer;
  const int16_t *win = (int16_t *)window;;

  for (int i=0; i < 1024; i++) {
    int32_t val = *buf * *win++;
    //*buf = signed_saturate_rshift(val, 16, 15);
    *buf = val >> 15;
    buf += 2;
  }

}

void AudioAnalyzeHfcOnset::update(void)
{
  profiler.call(analyzeother);

  audio_block_t *block;

  block = receiveReadOnly();
  if (!block) return;

  // We may not finish the FFT and HFC calculation before the next buffer is added,
  // so we store the correct value of sampleNumber for this iteration in a separate variable.
  unsigned newSampleNumber = (sampleNumber += 128);

  switch (state) {
    case 0:
      blocklist[0] = block;
      state = 1;
      break;
    case 1:
      blocklist[1] = block;
      state = 2;
      break;
    case 2:
      blocklist[2] = block;
      state = 3;
      break;
    case 3:
      blocklist[3] = block;
      state = 4;
      break;
    case 4:
      blocklist[4] = block;
      state = 5;
      break;
    case 5:
      blocklist[5] = block;
      state = 6;
      break;
    case 6:
      blocklist[6] = block;
      state = 7;
      break;
    case 7:
      profiler.call(analyzefft);
      blocklist[7] = block;
      // TODO: perhaps distribute the work over multiple update() ??
      //       github pull requsts welcome......
      copy_to_fft_buffer(buffer+0x000, blocklist[0]->data);
      copy_to_fft_buffer(buffer+0x100, blocklist[1]->data);
      copy_to_fft_buffer(buffer+0x200, blocklist[2]->data);
      copy_to_fft_buffer(buffer+0x300, blocklist[3]->data);
      copy_to_fft_buffer(buffer+0x400, blocklist[4]->data);
      copy_to_fft_buffer(buffer+0x500, blocklist[5]->data);
      copy_to_fft_buffer(buffer+0x600, blocklist[6]->data);
      copy_to_fft_buffer(buffer+0x700, blocklist[7]->data);
      if (window) apply_window_to_fft_buffer(buffer, window);
      arm_cfft_radix4_q15(&fft_inst, buffer);
      profiler.finish(analyzefft);

      // calculate power spectrum and HFC
      profiler.call(hfccalculation);
      int32_t hfc = 0;
      for (int i=0; i < 512; i++) {
        uint32_t tmp = *((uint32_t *)buffer + i); // real & imag
        uint32_t magsq = multiply_16tx16t_add_16bx16b(tmp, tmp);
        uint32_t mag = sqrt_uint32_approx(magsq);
        output[i] = static_cast<uint16_t>(mag);
        hfc += (512+i)*static_cast<int32_t>(mag);
      }
      profiler.finish(hfccalculation);

      // Add HFC samples to buffers
      profiler.call(bufferAddSample);
      rawHFC.addSample(hfc);
      profiler.finish(bufferAddSample);

      // Determine if this HFC sample is an onset
      profiler.call(onsetDetection);
      int32_t median = rawHFC.median();
      int32_t sdev = rawHFC.stddeviation();
      int32_t hfc2 = hfc - median;
      if (samplesSinceLastOnset > refractorySamples and hfc2 - 2*sdev > 0) {
        samplesSinceLastOnset = 0;
      } else {
        samplesSinceLastOnset++;
      }
      profiler.finish(onsetDetection);

      // Prepare smoothed HFC sample
      // Need to downscale to fit HFC samples in int16's.
      // From some dubstep tests, this scale leads to rare saturation.
      //hfc2 /= 128;
      hfc2 /= 2048;
      int16_t smallhfc2;
      if (hfc2 >= 0x8000) {
        Serial.println("Smoothed HFC saturation");
        smallhfc2 = INT16_MAX;
      } else {
        smallhfc2 = static_cast<int16_t>(hfc2);
      }
      unsigned hfcNumber = smoothedHFC.addSample(static_cast<int16_t>(smallhfc2));

      /*
      Serial.print(newSampleNumber);
      Serial.print(" / ");
      Serial.print(hfcNumber);
      Serial.print(": ");
      Serial.print(hfc);
      Serial.print(" - ");
      Serial.print(median);
      Serial.print(" => ");
      Serial.print(smallhfc2);
      Serial.print(", stddev: ");
      Serial.print(sdev);
      Serial.print(", since onset: ");
      Serial.print(samplesSinceLastOnset);
      Serial.println();
      */
      /*
      Serial.print("Audio sample: ");
      Serial.print(newSampleNumber);
      Serial.print(", hfc sample: ");
      Serial.print(hfcNumber);
      Serial.println();
      */

      // Determine if it's time for another beat estimation
      if ((hfcNumber & 0x7F) == 0) {
        /*
        Serial.print("Audio sample: ");
        Serial.print(newSampleNumber);
        Serial.print(", hfc sample: ");
        Serial.print(hfcNumber);
        Serial.println();
        */
        hfcWindowEnd = hfcNumber;
        hfcWindowEndSample = newSampleNumber;
        outputflag = true;
      }

      release(blocklist[0]);
      release(blocklist[1]);
      release(blocklist[2]);
      release(blocklist[3]);
      blocklist[0] = blocklist[4];
      blocklist[1] = blocklist[5];
      blocklist[2] = blocklist[6];
      blocklist[3] = blocklist[7];
      state = 4;

      profiler.finish(analyzeother);
      break;
  }
}


