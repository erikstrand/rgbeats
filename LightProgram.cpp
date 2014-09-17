//==============================================================================
// LightProgram.cpp
// Created 2014-08-21
//==============================================================================

#include "LightProgram.h"


//------------------------------------------------------------------------------
unsigned interpolateColor (unsigned c1, unsigned c2, unsigned diff, unsigned scale) {
  unsigned x1, x2;
  unsigned c3 = 0;

  // interpolate red
  x1 = c1 >> 16;
  x2 = c2 >> 16;
  if (x2 >= x1) {
    c3 |= (x1 + (diff * (x2 - x1) / scale)) << 16;
  } else {
    c3 |= (x2 + ((scale - diff) * (x1 - x2) / scale)) << 16;
  }

  // interpolate green
  x1 = (c1 >> 8) & 0xFF;
  x2 = (c2 >> 8) & 0xFF;
  if (x2 >= x1) {
    c3 |= (x1 + (diff * (x2 - x1) / scale)) << 8;
  } else {
    c3 |= (x2 + ((scale - diff) * (x1 - x2) / scale)) << 8;
  }

  // interpolate blue
  x1 = c1 & 0xFF;
  x2 = c2 & 0xFF;
  if (x2 >= x1) {
    c3 |= x1 + (diff * (x2 - x1) / scale);
  } else {
    c3 |= x2 + ((scale - diff) * (x1 - x2) / scale);
  }

  return c3;
};

//------------------------------------------------------------------------------
unsigned SpectrumInterpolator::pixel (volatile uint16_t const* spectrum, unsigned n, unsigned outMax) {
  unsigned n2 = n * spectrumMax;  // scale output pixel to common coords
  unsigned s1 = n2 / outMax;      // index of first sample
  unsigned diff = n2 - s1*outMax; // distance out from first sample
  if (diff == 0) {
    return 64*spectrum[s1];
  } else {
    unsigned v1 = spectrum[s1];
    unsigned v2 = spectrum[s1+1];
    if (v2 >= v1) {
      return 64 * v1 + 64 * diff * (v2 - v1) / outMax;
    } else {
      return 64 * v2 + 64 * (outMax - diff) * (v1 - v2) / outMax;
    }
  }
}

//------------------------------------------------------------------------------
void LinearInterpolator::pixel (unsigned x, MusicState const& state, Color& color) {
  unsigned x2 = x * n1;  // scale output pixel to common coords
  if (flip) { x2 = n1 * n2 - x2; }
  unsigned s1 = x2 / n2; // index of first sample
  unsigned diff = x2 - s1*n2; // distance out from first sample
  if (diff == 0) {
    p1->pixel(s1, state, color);
  } else {
    Color c1, c2;
    p1->pixel(s1, state, c1);
    p1->pixel(s1 + 1, state, c2);
    color = Color(interpolateColor(c1.rgbPack(), c2.rgbPack(), diff, n2));
  }
}

//------------------------------------------------------------------------------
void FadeTransition::pixel (unsigned x, MusicState const& state, Color& color) {
  unsigned completedSamples;
  switch (transitionState) {
  // transition is cued
  case 1:
    startSample = state.sample;
    transitionState = 2;
    // no break since we still need to render

  // transition is in progress
  case 2:
    completedSamples = state.sample - startSample;
    if (completedSamples < transitionSamples) {
      Color c1, c2;
      p1->pixel(x, state, c1);
      p2->pixel(x, state, c2);
      color = Color(interpolateColor(c1.rgbPack(), c2.rgbPack(), completedSamples, transitionSamples));
      break;
    } else {
      p1 = p2;
      transitionState = 0;
      // no break since we still need to render
    }

  // running a progam like normal
  default:
    p1->pixel(x, state, color);
  }
}


