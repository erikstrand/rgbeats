//==============================================================================
// MusicState.cpp
// Created 2014-08-12
//==============================================================================

#include "MusicState.h"


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
