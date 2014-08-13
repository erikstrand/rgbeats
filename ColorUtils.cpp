//==============================================================================
// ColorUtils.cpp
// Created 2014-08-10
//==============================================================================

#include "ColorUtils.h"


//------------------------------------------------------------------------------
void Color::hsvComponents (unsigned& h, unsigned& s, unsigned& v) const {
  // neutral color degenerate case
  if (x1 == x2 and x2 == x3) {
    h = 0; // h
    s = 0; // s
    v = x3;
    return;
  }

  // red == green (yellow / blue) degenerate case
  if (x1 == x2) {
    if (x1 > x3) {
      h = 256;
      s = 255 * (x1 - x3) / x1;
      v = x1;
    } else {
      h = 1024;
      s = 255 * (x3 - x1) / x3;
      v = x3;
    }
    return;
  }

  // green == blue (cyan / red) degenerate case
  if (x2 == x3) {
    if (x2 > x1) {
      h = 768;
      s = 255 * (x2 - x1) / x2;
      v = x2;
    } else {
      h = 0;
      s = 255 * (x1 - x2) / x1;
      v = x1;
    }
    return;
  }

  // blue == red (magenta / green) degenerate case
  if (x3 == x1) {
    if (x1 > x2) {
      h = 1280;
      s = 255 * (x1 - x2) / x1;
      v = x1;
    } else {
      h = 512;
      s = 255 * (x2 - x1) / x2;
      v = x2;
    }
    return;
  }

  // now we can assume that no two components are equal
  v = (x1 > x2) ? x1 : x2;
  if (x3 > v) { v = x3; }
  unsigned min = (x1 < x2) ? x1 : x2;
  if (x3 < min) { min = x3; }
  unsigned chroma = v - min;

  if (v == x1) {
    if (x2 > x3) {
      // sextant = 0;
      h = 255 * (x2 - x3) / chroma;
    } else {
      // sextant = 5;
      h = 1536 - 255 * (x3 - x2) / chroma;
    }
  } else if (v == x2) {
    if (x3 > x1) {
      // sextant = 2;
      h = 512 + 255 * ((int)(x3) - x1) / chroma;
    } else {
      // sextant = 1;
      //h = 256 + 255 * (x1 - x3) / chroma;
      h = 512 - 255 * (x1 - x3) / chroma;
    }
  } else {
    if (x1 > x2) {
      // sextant = 4;
      h = 1024 + 255 * (x1 - x2) / chroma;
    } else {
      // sextant = 3;
      h = 1024 - 255 * (x2 - x1) / chroma;
    }
  }

  s = 255 * chroma / v;
}

//------------------------------------------------------------------------------
void Color::rgbComponents (unsigned& r, unsigned& g, unsigned& b) const {
  // neutral color degenerate case
  if (x2 == 0 or x3 == 0) {
    r = x3;
    g = x3;
    b = x3;
    return;
  }

  unsigned chroma = x2 * x3 / 255; // s * v

  // green == blue (red) degenerate case
  if (x1 == 0) {
    r = x3;
    g = b = x3 - chroma;
    return;
  }

  // red == green (yellow) degenerate case
  if (x1 == 256) {
    r = g = x3;
    b = x3 - chroma;
    return;
  }

  // red == blue (green) degenerate case
  if (x1 == 512) {
    g = x3;
    r = b = x3 - chroma;
    return;
  }

  // green == blue (cyan) degenerate case
  if (x1 == 768) {
    g = b = x3;
    r = x3 - chroma;
    return;
  }

  // red == green (blue) degenerate case
  if (x1 == 1024) {
    b = x3;
    r = g = x3 - chroma;
    return;
  }

  // red == blue (magenta) degenerate case
  if (x1 == 1280) {
    r = b = x3;
    g = x3 - chroma;
    return;
  }

  // general case
  unsigned v = x3;
  unsigned sextant = x1 / 256;
  unsigned remainder = x1 % 256;
  r = g = b = 0;
  switch (sextant) {
    case 0:
      r = chroma;
      g = chroma * remainder / 255;
      break;
    case 1:
      r = chroma * (255 - remainder) / 255;
      g = chroma;
      break;
    case 2:
      g = chroma;
      b = chroma * remainder / 255;
      break;
    case 3:
      g = chroma * (255 - remainder) / 255;
      b = chroma;
      break;
    case 4:
      r = chroma * remainder / 255;
      b = chroma;
      break;
    default:
      r = chroma;
      b = chroma * (255 - remainder) / 255;
      break;
  }

  unsigned m = v - chroma;
  r += m;
  g += m;
  b += m;
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

