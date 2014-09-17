//==============================================================================
// LightProgramTemplate.h
// Created 2014-08-21
//==============================================================================

#ifndef RGBEATS_LIGHTPROGRAM_TEMPLATE
#define RGBEATS_LIGHTPROGRAM_TEMPLATE

#include "LightProgram.h"
//#include <arm_math.h>
#include "MusicState.h"
#include "ColorUtils.h"


//==============================================================================
// Base LightPrograms
//==============================================================================

//------------------------------------------------------------------------------
template <unsigned N>
class Solid : public LightProgram {
private:
  Color c;
public:
  Solid (): c(100, 0, 0) {}
  void init (XorShift32& rand) {
    c.hsvRepresentation();
    c.x1 = rand.uint32() % 1536;
    c.x2 = 150 + rand.uint32() % 106;
    c.x3 = 10 + rand.uint32() % 30;
  };
  inline void pixel (unsigned x, MusicState const& state, Color& color) {
    color = c;
  }
  inline void setColor (Color const& color) { c = color; }
};

//------------------------------------------------------------------------------
// ToDo: abstract use of color scale
template <unsigned N>
class ColorScaleProgram : public LightProgram {
public:
  ColorScale* cs;
  ColorScaleProgram (ColorScale* c): cs(c) {}
public:
  //void init (XorShift32& rand) {}
  void pixel (unsigned x, MusicState const& state, Color& color);
};

template <unsigned N>
void ColorScaleProgram<N>::pixel (unsigned x, MusicState const& state, Color& color) {
  color = Color(cs->color(x * ColorScale::max / N));
}

//------------------------------------------------------------------------------
// ToDo: abstract use of color scale
template <unsigned N>
class SpectrumProgram : public LightProgram {
private:
  SpectrumInterpolator si;
public:
  inline SpectrumProgram () {}
  //void init (XorShift32& rand) {}
  inline void pixel (unsigned x, MusicState const& state, Color& color);
};

//------------------------------------------------------------------------------
// class NoiseSource


//==============================================================================
// Effect Filters
//==============================================================================

//------------------------------------------------------------------------------
template <unsigned N>
class ShiftColorOnset : public LightFilter {
private:
  SawDecay sd1;
  unsigned sample;
public:
  ShiftColorOnset (): sd1(5, 2), sample(0) {}
  void pixel (unsigned x, MusicState const& state, Color& color);
};

template <unsigned N>
void ShiftColorOnset<N>::pixel (unsigned x, MusicState const& state, Color& color) {
  if (state.sample != sample) {
    sample = state.sample;
    sd1.update(state.sample, state.onsetSaw(1024, 10));
  }
  p1->pixel(x, state, color);
  color.hsvRepresentation();
  color.x1 += sd1.height();
  color.x1 %= 1536;
}

//------------------------------------------------------------------------------
template <unsigned N>
class ShiftBrightnessOnset : public LightFilter {
public:
  SawDecay sd1;
  unsigned sample;
public:
  ShiftBrightnessOnset (): sd1(5, 2), sample(0) {}
  void pixel (unsigned x, MusicState const& state, Color& color);
};

template <unsigned N>
void ShiftBrightnessOnset<N>::pixel (unsigned x, MusicState const& state, Color& color) {
  if (state.sample != sample) {
    sample = state.sample;
    sd1.update(state.sample, state.onsetSaw(1024, 10));
  }
  p1->pixel(x, state, color);
  color.hsvRepresentation();
  if (state.samplesSinceOnset < 15) {
    color.x3 = Color::addSaturate(color.x3, state.onsetSaw(25, 15));
  }
}


//------------------------------------------------------------------------------
template <unsigned N>
class ColorShifter : public LightFilter {
private:
  SawDecay sd1;
  unsigned sample;
public:
  ColorShifter (): sd1(5, 2), sample(0) {}
  void pixel (unsigned x, MusicState const& state, Color& color);
};


template <unsigned N>
void ColorShifter<N>::pixel (unsigned x, MusicState const& state, Color& color) {
  if (state.sample != sample) {
    sample = state.sample;
    sd1.update(state.sample, state.onsetSaw(1024, 10));
    //Serial.println(sd1.height());
  }
  p1->pixel(x, state, color);
  
  // pulse with hfc
  /*
  color.hsvRepresentation();
  unsigned colorAdd = state.onsetSaw();
  color.x1 += colorAdd;
  color.x1 %= 1536;
  */

  // pulse with onsets
  color.hsvRepresentation();
  if (state.samplesSinceOnset < 15) {
    color.x3 += state.onsetSaw(25, 15);
  }
  int oldx1 = color.x1;
  color.x1 += sd1.height();
  color.x1 %= 1536;
  //if (x == 10) { Serial.print(oldx1); Serial.print(" + "); Serial.print(sd1.height()); Serial.print(" --> "); Serial.print(color.x1); Serial.print(" at "); Serial.println(state.sample); }

  // pulse with beat
  /*
  color.hsvRepresentation();
  unsigned quarterCounter = state.beat % 4;
  color.x3 += (1024*quarterCounter + state.beatpos) / 400;
  */

  // saturate with hfc
  /*
  color.hsvRepresentation();
  unsigned hfc = state.hfc;
  //hfc >>= 16;
  //hfc = log2_fp(hfc);
  color.x2 += hfc / 300000;
  color.x2 &= 0xFF;
  */

  // rotate with beat
  /*
  color.hsvRepresentation();
  unsigned quarterCounter = state.beat % 8;
  color.x1 += (1024*quarterCounter + state.beatpos) * 1536 / 8192;
  color.x1 = color.x1 % 1536;
  */
}

//------------------------------------------------------------------------------
template <unsigned N>
class FlickerBrightness : public LightFilter {
public:
  FlickerFeature<N> randomwalk;
  bool saturateAtOne;
public:
  FlickerBrightness () {}
  void init (XorShift32& rand);
  inline void lanternsMode () { randomwalk.lanternsMode(); }
  inline void pixel (unsigned x, MusicState const& state, Color& color);
  int test () { if (p1 == 0) { return 0; } else { return 1; } }
};

template <unsigned N>
void FlickerBrightness<N>::init (XorShift32& rand) {
  if (rand.uint32() <= 3435973836) { // = 0.8 * 2^32
    saturateAtOne = true;
  } else {
    saturateAtOne = false;
  }
  randomwalk.init(rand);
}

template <unsigned N>
void FlickerBrightness<N>::pixel (unsigned x, MusicState const& state, Color& color) {
  randomwalk.update(state);
  p1->pixel(x, state, color);
  color.hsvRepresentation();
  color.x3 = Color::addSaturate(color.x3, randomwalk.height(x));
  if (saturateAtOne) {
    color.x3 = Color::addSaturate(color.x3, randomwalk.height(x));
  } else {
    color.x3 = Color::addSaturateOne(color.x3, randomwalk.height(x));
  }
}


//==============================================================================
// Motion Filters
//==============================================================================

//------------------------------------------------------------------------------
// class MarqueeMotion

//------------------------------------------------------------------------------
// class Bands


//==============================================================================
// Geometry Filters
//==============================================================================

//------------------------------------------------------------------------------
template <unsigned N>
class RotateProgram : public LightFilter {
public:
  unsigned rotation;
  RotateProgram (unsigned r): rotation(r) {}
  inline void pixel (unsigned x, MusicState const& state, Color& color) {
    if (x < rotation) { x += N; }
    p1->pixel(x - rotation, state, color);
  }
};

//------------------------------------------------------------------------------
template <unsigned N>
class VUMeter : public LightFilter {
public:
  VUMeter () {}
  inline void pixel (unsigned x, MusicState const& state, Color& color);
};

template <unsigned N>
void VUMeter<N>::pixel (unsigned x, MusicState const& state, Color& color) {
  unsigned max = state.hfcLin(N);
  if (max >= N) { max = N; }
  if (x <= max) {
    p1->pixel(x, state, color);
  } else {
    color = Color(0x000000);
  }
}

//------------------------------------------------------------------------------
template <unsigned N>
class ProgramRepeater : public LightFilter {
public:
  //LightProgram* p1;
  LinearInterpolator lin;
  unsigned copies;
  bool flipOdd;
public:
  ProgramRepeater (): lin(N, N/16), copies(16), flipOdd(true) {}
  ProgramRepeater (unsigned l, unsigned c): lin(l, N/c), copies(c), flipOdd(true) {}
  void init (XorShift32& rand);
  void pixel (unsigned x, MusicState const& state, Color& color);
  // we need to provide a custom definition, since we use lin's program and not our own
  void connect (LightProgram* p, unsigned n = 0) { lin.connect(p); };
};

template <unsigned N>
void ProgramRepeater<N>::init (XorShift32& rand) {
  copies = 2 << (rand.uint32() % 4);
  flipOdd = rand.uint32() & 0x10 ? true : false;
}

template <unsigned N>
void ProgramRepeater<N>::pixel (unsigned x, MusicState const& state, Color& color) {
  unsigned ledsPerCopy = N / copies;
  unsigned program_number = x / ledsPerCopy;
  unsigned pixel_number = x % ledsPerCopy;
  if (flipOdd && (program_number & 0x1)) {
    lin.setFlip(true);
  } else {
    lin.setFlip(false);
  }
  lin.pixel(pixel_number, state, color);
}


//==============================================================================
// Program Branches (LightPrograms with two LightProgram inputs)
//==============================================================================

//------------------------------------------------------------------------------
template <unsigned N>
class Lanterns : public LightBranch {
private:
  unsigned nLanterns;
  unsigned width;
  unsigned separation;
public:
  inline Lanterns (): nLanterns(8), width(6) { separation = N / nLanterns; }
  inline Lanterns (unsigned l, unsigned w): nLanterns(l), width(w) { separation = N / nLanterns; }
  void pixel (unsigned x, MusicState const& state, Color& color);
};

template <unsigned N>
void Lanterns<N>::pixel (unsigned x, MusicState const& state, Color& color) {
  if ((x + width / 2) % separation < width) {
    p1->pixel(x, state, color);
  } else {
    p2->pixel(x, state, color);
  }
}

//------------------------------------------------------------------------------
// class SwitcherBranch

//------------------------------------------------------------------------------
// class MaskBranch

//------------------------------------------------------------------------------
// class TransitionBranch


//------------------------------------------------------------------------------
template <unsigned N>
void SpectrumProgram<N>::pixel (unsigned x, MusicState const& state, Color& color) {
  unsigned power = si.pixel(state.spectrum, x, N-1);
  power &= 0xFF; // make sure we saturate at 255
  color = Color(cubehelix2[power]);
}

#endif
