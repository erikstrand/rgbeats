//==============================================================================
// LightProgram.h
// Created 2014-07-02
//==============================================================================

#ifndef BEATSERAI_LIGHTPROGRAM
#define BEATSERAI_LIGHTPROGRAM

#include <arm_math.h>
#include "ColorUtils.h"

extern const int ledsPerStrip;


//==============================================================================
// Helpers
//==============================================================================

//------------------------------------------------------------------------------
inline unsigned interpolateColor (unsigned c1, unsigned c2, unsigned diff, unsigned scale) {
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
class SpectrumInterpolator {
public:
  static const unsigned spectrumMax = 511;
public:
  SpectrumInterpolator () {}
  inline unsigned pixel (volatile uint16_t const* spectrum, unsigned n, unsigned outMax);
};

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


//==============================================================================
// Base Classes
//==============================================================================

//------------------------------------------------------------------------------
// virtual base class for light programs
class LightProgram {
public:
  virtual unsigned pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) = 0;
};

//------------------------------------------------------------------------------
class ColorScale {
public:
  static const unsigned max = (1 << 16);
  // returns a color given a number between 1 and 2^16
  virtual unsigned color (unsigned x) = 0;
};

//------------------------------------------------------------------------------
template <unsigned N>
class ColorScaleProgram : public LightProgram {
public:
  ColorScale* cs;
  ColorScaleProgram (ColorScale* c): cs(c) {}
  unsigned pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum);
};

template <unsigned N>
unsigned ColorScaleProgram<N>::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  return cs->color(x * ColorScale::max / N);
}

//------------------------------------------------------------------------------
/*
template <unsigned N>
class BufferProgram : public LightProgram {
};
*/


//==============================================================================
// Sources
//==============================================================================

//------------------------------------------------------------------------------
template <unsigned N>
class Solid : public LightProgram {
private:
  unsigned color;
public:
  Solid (unsigned c): color(c) {}
  inline unsigned pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
    return color;
  }
};

//------------------------------------------------------------------------------
template <unsigned N>
class Lanterns : public LightProgram {
private:
  unsigned offColor;
  unsigned onColor;
  unsigned nLanterns;
  unsigned width;
  unsigned separation;
public:
  inline Lanterns (unsigned c1, unsigned c2, unsigned l, unsigned w): offColor(c1), onColor(c2), nLanterns(l), width(w) {
    separation = N / nLanterns;
  }
  unsigned pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum);
};

template <unsigned N>
unsigned Lanterns<N>::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  if (x % separation < width) { return onColor; }
  else { return offColor; }
}

//------------------------------------------------------------------------------
template <unsigned N>
class SpectrumProgram : public LightProgram {
private:
  SpectrumInterpolator si;
public:
  inline SpectrumProgram () {}
  unsigned pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum);
};

//------------------------------------------------------------------------------
template <unsigned N>
class VUMeter : public LightProgram {
public:
  //static const int scale = 3000000;
  static const int scale = 30000;
  LightProgram* p1;
  VUMeter (LightProgram* p): p1(p) {}
  unsigned pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum);
};

template <unsigned N>
unsigned VUMeter<N>::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  hfc >>= 16;
  hfc = log2_fp(hfc);
  unsigned max = hfc * N / scale;
  if (max >= N) { max = N; }
  if (x <= max) {
    return p1->pixel(x, id, beat, beatpos, hfc, spectrum);
  } else {
    return 0;
  }
}

//==============================================================================
// Filters
//==============================================================================

//------------------------------------------------------------------------------
template <unsigned N>
class RotateProgram : public LightProgram {
public:
  LightProgram* p1;
  unsigned rotation;
  RotateProgram (LightProgram* p, unsigned r): p1(p), rotation(r) {}
  inline unsigned pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
    if (x < rotation) { x += N; }
    return p1->pixel(x - rotation, id, beat, beatpos, hfc, spectrum);
  }
};

//------------------------------------------------------------------------------
class LinearInterpolator : public LightProgram {
private:
  LightProgram* p1;
  unsigned n1; // number of pixels generated by p1
  unsigned n2; // number of pixels desired
  bool flip;   // if true, the program is flipped in addition to stretched
public:
  LinearInterpolator (LightProgram* p, unsigned n1, unsigned n2): p1(p), n1(n1), n2(n2), flip(false) {}
  inline void setProgram (LightProgram* p) { p1 = p; }
  inline void setBounds (unsigned new_n1, unsigned new_n2) { n1 = new_n1; n2 = new_n2; }
  inline void setFlip (bool f) { flip = f; }
  unsigned pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum);
};

unsigned LinearInterpolator::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  unsigned x2 = x * n1;  // scale output pixel to common coords
  if (flip) { x2 = n1 * n2 - x2; }
  unsigned s1 = x2 / n2; // index of first sample
  unsigned diff = x2 - s1*n2; // distance out from first sample
  if (diff == 0) {
    return p1->pixel(s1, id, beat, beatpos, hfc, spectrum);
  } else {
    unsigned c1 = p1->pixel(s1, id, beat, beatpos, hfc, spectrum);
    unsigned c2 = p1->pixel(s1 + 1, id, beat, beatpos, hfc, spectrum);
    return interpolateColor(c1, c2, diff, n2);
  }
}

//------------------------------------------------------------------------------
/*
template <unsigned N>
class ProgramMapper : public LightProgram {
public:
  static const unsigned maxPrograms = 16;
  unsigned splits[maxPrograms];
  bool flips[maxPrograms];
  LightProgram* programs[maxPrograms];
  unsigned activePrograms;
public:
   unsigned pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum);
};

template <unsigned N>
unsigned ProgramMapper<N>::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
}
*/

//------------------------------------------------------------------------------
template <unsigned N>
class ProgramRepeater : public LightProgram {
public:
  //LightProgram* p1;
  LinearInterpolator lin;
  unsigned copies;
  bool flipOdd;
public:
  ProgramRepeater (LightProgram* p, unsigned l, unsigned c): lin(p, l, N/c), copies(c), flipOdd(true) {}
  unsigned pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum);
};

//------------------------------------------------------------------------------
template <unsigned N>
unsigned ProgramRepeater<N>::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  unsigned ledsPerCopy = N / copies;
  unsigned program_number = x / ledsPerCopy;
  unsigned pixel_number = x % ledsPerCopy;
  if (flipOdd && (program_number & 0x1)) {
    lin.setFlip(true);
  } else {
    lin.setFlip(false);
  }
  return lin.pixel(pixel_number, id, beat, beatpos, hfc, spectrum);
}

//------------------------------------------------------------------------------
template <unsigned N>
class ColorShifter : public LightProgram {
private:
  LightProgram *p1;
public:
  ColorShifter (LightProgram* p): p1(p) {}
  unsigned pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum);
};

template <unsigned N>
unsigned ColorShifter<N>::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  Color c(p1->pixel(x, id, beat, beatpos, hfc, spectrum));
  
  // pulse with hfc
  /*
  c.hsvRepresentation();
  hfc >>= 16;
  hfc = log2_fp(hfc);
  c.x1 += hfc / 100;
  c.x1 = c.x1 % 1536;
  return c.rgbPack();
  */

  // pulse with beat
  /*
  c.hsvRepresentation();
  unsigned quarterCounter = beat % 4;
  c.x3 += (1024*quarterCounter + beatpos) / 400;
  return c.rgbPack();
  */

  // saturate with hfc
  /*
  c.hsvRepresentation();
  //hfc >>= 16;
  //hfc = log2_fp(hfc);
  c.x2 += hfc / 300000;
  c.x2 &= 0xFF;
  return c.rgbPack();
  */

  // rotate with beat
  c.hsvRepresentation();
  unsigned quarterCounter = beat % 8;
  c.x1 += (1024*quarterCounter + beatpos) * 1536 / 8192;
  c.x1 = c.x1 % 1536;
  return c.rgbPack();
}

//------------------------------------------------------------------------------
/*
class BufferProgram : public LightProgram {
public:
   unsigned buffer[ledsPerStrip];
   LightProgram* p;
   unsigned currentid;
   BufferProgram (LightProgram* p_): p(p_), currentid(~0) { memset(buffer, 0, ledsPerStrip*sizeof(unsigned)); }
   unsigned pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum);
};
*/


const unsigned cubehelix[] = {
0x000000, 0x000000, 0x000000, 0x010000, 0x010000, 0x020001, 0x030001, 0x030001, 0x030101, 0x030101, 0x030101, 0x040102, 0x040102, 0x040102, 0x050103, 0x060103, 0x060104, 0x060204, 0x070204, 0x070204, 0x070205, 0x070205, 0x070205, 0x080206, 0x080206, 0x090207, 0x090207, 0x090308, 0x090308, 0x0a0309, 0x0a0309, 0x0a0309,
0x0a030a, 0x0a030a, 0x0b040b, 0x0b040b, 0x0b040c, 0x0b040c, 0x0b040d, 0x0b040d, 0x0c050e, 0x0c050e, 0x0c050f, 0x0c0510, 0x0c0610, 0x0c0610, 0x0c0611, 0x0c0612, 0x0c0712, 0x0c0712, 0x0c0713, 0x0c0714, 0x0c0814, 0x0c0814, 0x0c0815, 0x0c0815, 0x0c0916, 0x0b0917, 0x0b0917, 0x0b0a17, 0x0b0a18, 0x0b0b18, 0x0b0b18, 0x0b0b19,
0x0a0c19, 0x0a0c1a, 0x0a0c1a, 0x0a0c1a, 0x0a0d1a, 0x0a0d1a, 0x0a0d1a, 0x090e1b, 0x090f1b, 0x090f1c, 0x090f1c, 0x08101c, 0x08101c, 0x08111c, 0x07111c, 0x07111c, 0x07121c, 0x07121c, 0x07131c, 0x06141d, 0x06141c, 0x06141c, 0x06141c, 0x06151c, 0x06151c, 0x05161c, 0x05161c, 0x05171c, 0x05181c, 0x04181c, 0x04181c, 0x04181c,
0x04191b, 0x04191b, 0x041a1b, 0x041a1b, 0x041b1a, 0x041c1a, 0x041c1a, 0x041c19, 0x031d19, 0x031d18, 0x031e18, 0x041e18, 0x041e17, 0x041f17, 0x041f16, 0x041f16, 0x042016, 0x042015, 0x042115, 0x042114, 0x052114, 0x052114, 0x052213, 0x052212, 0x062312, 0x062312, 0x062311, 0x062310, 0x072410, 0x07240f, 0x08240f, 0x08240f,
0x08240e, 0x08240e, 0x09250d, 0x09250d, 0x0a250c, 0x0b250c, 0x0c250b, 0x0c260b, 0x0c260b, 0x0d260a, 0x0e2609, 0x0f2609, 0x0f2608, 0x102608, 0x112608, 0x112608, 0x122607, 0x132607, 0x142606, 0x152606, 0x152606, 0x162606, 0x172605, 0x182605, 0x192605, 0x1a2605, 0x1b2605, 0x1c2604, 0x1d2504, 0x1e2504, 0x1f2504, 0x202504,
0x212504, 0x222504, 0x232504, 0x242404, 0x252404, 0x262404, 0x272405, 0x282405, 0x292305, 0x2a2305, 0x2b2305, 0x2c2306, 0x2d2206, 0x2e2206, 0x2f2207, 0x302207, 0x312108, 0x322108, 0x332109, 0x342109, 0x35200a, 0x36200a, 0x36200b, 0x37200c, 0x381f0c, 0x391f0d, 0x3a1f0e, 0x3b1f0e, 0x3c1e0f, 0x3d1e10, 0x3e1e11, 0x3e1e12,
0x3f1d13, 0x401d14, 0x411d15, 0x411d16, 0x421c17, 0x431c18, 0x441c19, 0x441c1a, 0x451c1b, 0x451c1c, 0x461b1d, 0x471b1e, 0x471b1f, 0x481b20, 0x481b22, 0x481b23, 0x491b24, 0x491b25, 0x4a1a27, 0x4a1a28, 0x4a1a29, 0x4a1a2a, 0x4b1a2c, 0x4b1a2d, 0x4b1a2e, 0x4b1a30, 0x4b1a31, 0x4b1a32, 0x4b1b33, 0x4b1b35, 0x4b1b36, 0x4b1b37,
0x4b1b39, 0x4b1b3a, 0x4b1b3b, 0x4b1c3d, 0x4a1c3e, 0x4a1c3f, 0x4a1c40, 0x4a1c42, 0x491d43, 0x491d44, 0x491d45, 0x481e47, 0x481e48, 0x471e49, 0x471f4a, 0x461f4b, 0x46204c, 0x45204d, 0x44214e, 0x44214f, 0x432150, 0x422251, 0x422252, 0x412353, 0x402454, 0x3f2455, 0x3f2556, 0x3e2557, 0x3e2557, 0x3e2557, 0x3e2557, 0x3e2557,
0x3e2557, 0x3e2557, 0x3e2557, 0x3d2658 
};

const unsigned cubehelix2[] = {
0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000001, 0x000001, 0x000001, 0x000001, 0x000001, 0x000002, 0x000102, 0x010102, 0x010102, 0x010102, 0x010103, 0x010103, 0x010103, 0x010103, 0x010103, 0x010204, 0x010204, 0x010204, 0x010204, 0x010204, 0x010204, 0x010205, 0x010305, 0x010305, 0x010305, 0x010305,
0x010305, 0x010305, 0x010306, 0x010406, 0x010406, 0x010406, 0x010406, 0x010406, 0x010406, 0x010406, 0x010506, 0x010507, 0x010507, 0x010507, 0x010507, 0x010507, 0x010607, 0x010607, 0x010607, 0x010607, 0x010607, 0x010707, 0x000707, 0x000707, 0x000707, 0x000707, 0x000707, 0x000807, 0x000807, 0x000807, 0x000807, 0x000807,
0x000807, 0x000907, 0x000907, 0x000907, 0x000907, 0x000907, 0x000907, 0x000a07, 0x000a07, 0x000a07, 0x010a07, 0x010a07, 0x010a07, 0x010b06, 0x010b06, 0x010b06, 0x010b06, 0x010b06, 0x010b06, 0x010c06, 0x010c06, 0x010c06, 0x010c06, 0x010c05, 0x010c05, 0x020c05, 0x020d05, 0x020d05, 0x020d05, 0x020d05, 0x020d05, 0x020d04,
0x020d04, 0x030d04, 0x030e04, 0x030e04, 0x030e04, 0x030e04, 0x030e04, 0x040e03, 0x040e03, 0x040e03, 0x040e03, 0x040e03, 0x050f03, 0x050f03, 0x050f02, 0x050f02, 0x060f02, 0x060f02, 0x060f02, 0x060f02, 0x070f02, 0x070f02, 0x070f02, 0x070f01, 0x080f01, 0x080f01, 0x080f01, 0x090f01, 0x090f01, 0x090f01, 0x0a0f01, 0x0a0f01,
0x0a0f01, 0x0b0f01, 0x0b0f01, 0x0b0f01, 0x0c0f01, 0x0c0f00, 0x0c0f00, 0x0d0f00, 0x0d0f00, 0x0e0f00, 0x0e0f00, 0x0e0f00, 0x0f0f00, 0x0f0f00, 0x0f0f00, 0x100f01, 0x100f01, 0x110f01, 0x110f01, 0x110f01, 0x120f01, 0x120f01, 0x130f01, 0x130f01, 0x140f01, 0x140f01, 0x140f02, 0x150e02, 0x150e02, 0x160e02, 0x160e02, 0x160e02,
0x170e03, 0x170e03, 0x180e03, 0x180e03, 0x180e03, 0x190e04, 0x190e04, 0x1a0e04, 0x1a0d04, 0x1a0d05, 0x1b0d05, 0x1b0d05, 0x1c0d06, 0x1c0d06, 0x1c0d06, 0x1d0d06, 0x1d0d07, 0x1d0d07, 0x1e0d08, 0x1e0d08, 0x1e0d08, 0x1f0c09, 0x1f0c09, 0x1f0c09, 0x200c0a, 0x200c0a, 0x200c0b, 0x210c0b, 0x210c0c, 0x210c0c, 0x220c0c, 0x220c0d,
0x220c0d, 0x220c0e, 0x230c0e, 0x230c0f, 0x230c0f, 0x230c10, 0x230c10, 0x240b11, 0x240b11, 0x240b12, 0x240b12, 0x240b13, 0x250b13, 0x250b14, 0x250b15, 0x250b15, 0x250b16, 0x250b16, 0x250b17, 0x250b17, 0x250b18, 0x250c18, 0x250c19, 0x260c1a, 0x260c1a, 0x260c1b, 0x260c1b, 0x260c1c, 0x260c1c, 0x260c1d, 0x260c1e, 0x250c1e,
0x250c1f, 0x250c1f, 0x250c20, 0x250d20, 0x250d21, 0x250d21, 0x250d22, 0x250d23, 0x250d23, 0x240d24, 0x240d24, 0x240e25, 0x240e25, 0x240e26, 0x240e26, 0x230e27, 0x230e27, 0x230f28, 0x230f28, 0x220f29, 0x220f29, 0x22102a, 0x22102a, 0x21102b, 0x21102b, 0x21102c, 0x20112c, 0x20112c, 0x20112d, 0x1f112d, 0x1f122e, 0x1f122e
};

//------------------------------------------------------------------------------
class CubeHelixScale : public ColorScale {
public:
  unsigned color (unsigned x);
};

//------------------------------------------------------------------------------
template <unsigned N>
unsigned SpectrumProgram<N>::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  unsigned power = si.pixel(spectrum, x, N-1);
  power &= 0xFF; // make sure we saturate at 255
  return cubehelix2[power];
}

/*
//------------------------------------------------------------------------------

unsigned BufferProgram::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
   if (id != currentid) {
      currentid = id;
      for (unsigned i=0; i<ledsPerStrip; ++i) {
         buffer[i] = p->pixel(x, id, beat, beatpos, hfc, spectrum);
      }
   }
   return buffer[x];
}


//------------------------------------------------------------------------------
unsigned CubeHelixScale::color (unsigned x) {
  return cubehelix2[x * 256 / ColorScale::max];
}
*/


#endif
