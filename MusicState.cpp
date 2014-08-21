//==============================================================================
// MusicState.cpp
// Created 2014-08-12
//==============================================================================

#include "MusicState.h"


//------------------------------------------------------------------------------
unsigned MusicState::random () const {
  unsigned a = 0;
  // Fill up a word using the low bits of some spectral bins.
  // My guess is that integral multiple bins are more likely to exhibit correlation,
  // so I tried to choose bins that aren't related by simple ratios.
  // However no tests have been performed (this isn't a crypto grade RNG after all)
  // so my guesses are likely wrong.
  a |= spectrum[10] & 0xF;
  a |= ((spectrum[15] & 0xF) << 4);
  a |= ((spectrum[19] & 0xF) << 8);
  a |= ((spectrum[24] & 0xF) << 12);
  a |= ((spectrum[31] & 0xF) << 16);
  a |= ((spectrum[32] & 0xF) << 20);
  a |= ((spectrum[46] & 0xF) << 24);
  a |= ((spectrum[54] & 0xF) << 28);
  // Use Murmur hash to mix all the bits together
  a ^= a >> 13;
  a *= 0x5bd1e995; // magic constant (see Murmur Hash paper to demystify)
  a ^= a >> 15;
  return a;
}

//------------------------------------------------------------------------------
unsigned MusicState::hfcLin (unsigned scale) const {
  // HFC maxes out around 3 million, which we need to fit in a 16 bit word.
  // Dividing by 64 means we're very unlikely to clip bits off the top.
  unsigned x = (hfc >> 6);
  return x * scale / 60000;
}

//------------------------------------------------------------------------------
unsigned MusicState::hfcLog (unsigned scale, unsigned noise_floor) const {
  // HFC maxes out around 3 million, which we need to fit in a 16 bit word.
  // Dividing by 64 means we're very unlikely to clip bits off the top.
  unsigned x = (hfc >> 6);
  // remove noise floor
  if (x < noise_floor) { x = 0; }
  else { x -= noise_floor; }
  return log2_fp(x) * scale / 64000;
}

//------------------------------------------------------------------------------
unsigned MusicState::onsetSaw (unsigned scale, unsigned decay, bool useMaxSignificance) const {
  if (samplesSinceOnset >= decay) {
    return 0;
  }
  // We want to divide by 6, as a reasonable upper bound for the number of sdevs we expect to see regularly.
  // We divide by 3 now to retain an extra digit of precision, and divide out the remaining
  // factor of two at the end.
  unsigned rise = (useMaxSignificance ? maxSignificance : onsetSignificance) * scale / 3;
  rise = rise - rise * samplesSinceOnset / decay;
  return (rise >> 1) + ((rise & 0x1) ? 1 : 0);
}

//------------------------------------------------------------------------------
void SawDecay::update (unsigned sample, unsigned height) {
  if (currentHeight > 0) {
    unsigned decay = (sample - lastSample) * decayFall / decayTime;
    if (decay < lastHeight) {
      currentHeight = lastHeight - decay;
    } else {
      currentHeight = 0;
    }
  }
 
  if (height > currentHeight) {
    lastHeight = currentHeight = height;
    lastSample = sample;
  }
}

//------------------------------------------------------------------------------
uint16_t log2_fp (uint16_t x_16t) {
  if (x_16t == 0) { return 0; }
  unsigned y;
  unsigned b = 1 << 27; // 1/2 in Q28
  unsigned c = 0;

  // Compute the characteristic (integral part of log2(x)).
  // This also determines the fixed point format we use to store x.
  uint32_t x = x_16t;
  while (x_16t >= 2) {
    x_16t >>= 1;
    ++c;
  }
  y = c << 28;
  
  // Calculate 8 binary fractional digits of log2(x).
  unsigned threshold = 2 << c; // 2 in Q[c]
  for (unsigned i=0; i<16; ++i) {
    x *= x;
    x >>= c; // return x to Q[c].
    if (x >= threshold) {
      x >>= 1;
      y |= b;
    }
    b >>= 1;
  }

  return y >> 16;
}

