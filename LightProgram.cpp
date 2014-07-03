#include "LightProgram.h"

//------------------------------------------------------------------------------
unsigned Spectrum::pixel (unsigned x, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  unsigned bin1 = 8 + 2*x;
  unsigned power = 16*(spectrum[bin1] + spectrum[bin1+1]);
  power &= 0xFF; // make sure we saturate at 255
  return cubehelix2[power];
}

//------------------------------------------------------------------------------
unsigned VUMeter::pixel (unsigned x, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  unsigned max = hfc * ledsPerStrip / scale;
  if (max >= ledsPerStrip) { max = ledsPerStrip; }
  if (x <= max) {
    return p1->pixel(x, beat, beatpos, hfc, spectrum);
  } else {
    return 0;
  }
}

//------------------------------------------------------------------------------
unsigned ProgramRepeater::pixel (unsigned x, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
   if (x < (ledsPerStrip / copies)) {
      return p1->pixel(ledsPerStrip - (x % (ledsPerStrip / copies) * copies), beat, beatpos, hfc, spectrum);
   } else {
      return p1->pixel(x % (ledsPerStrip / copies) * copies, beat, beatpos, hfc, spectrum);
   }
}

//------------------------------------------------------------------------------
unsigned ColorScaleProgram::pixel (unsigned x, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  return cs->color(x * ColorScale::max / ledsPerStrip);
}

//------------------------------------------------------------------------------
unsigned CubeHelixScale::color (unsigned x) {
  return cubehelix2[x * 256 / ColorScale::max];
}
