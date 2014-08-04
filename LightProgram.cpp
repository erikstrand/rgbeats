#include "LightProgram.h"

//------------------------------------------------------------------------------
unsigned Spectrum::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  unsigned bin1 = 8 + 2*x;
  unsigned power = 16*(spectrum[bin1] + spectrum[bin1+1]);
  power &= 0xFF; // make sure we saturate at 255
  return cubehelix2[power];
}

//------------------------------------------------------------------------------
unsigned VUMeter::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  unsigned max = hfc * ledsPerStrip / scale;
  if (max >= ledsPerStrip) { max = ledsPerStrip; }
  if (x <= max) {
    return p1->pixel(x, id, beat, beatpos, hfc, spectrum);
  } else {
    return 0;
  }
}

//------------------------------------------------------------------------------
unsigned ProgramRepeater::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
   if (x < (ledsPerStrip / copies)) {
      return p1->pixel(ledsPerStrip - (x % (ledsPerStrip / copies) * copies), id, beat, beatpos, hfc, spectrum);
   } else {
      return p1->pixel(x % (ledsPerStrip / copies) * copies, id, beat, beatpos, hfc, spectrum);
   }
}

//------------------------------------------------------------------------------
unsigned AffineTransformationProgram::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
   int transposed = static_cast<int>( (a <= x) ? x - a : ledsPerStrip - x + a );
   return p->pixel( static_cast<unsigned>(transposed * numerator / denominator) % ledsPerStrip, id, beat, beatpos, hfc, spectrum );
}

//------------------------------------------------------------------------------
/*
unsigned BufferProgram::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
   if (id != currentid) {
      currentid = id;
      for (unsigned i=0; i<ledsPerStrip; ++i) {
         buffer[i] = p->pixel(x, id, beat, beatpos, hfc, spectrum);
      }
   }
   return buffer[x];
}
*/

//------------------------------------------------------------------------------
unsigned ColorScaleProgram::pixel (unsigned x, unsigned id, unsigned beat, unsigned beatpos, unsigned hfc, volatile uint16_t const* spectrum) {
  return cs->color(x * ColorScale::max / ledsPerStrip);
}

//------------------------------------------------------------------------------
unsigned CubeHelixScale::color (unsigned x) {
  return cubehelix2[x * 256 / ColorScale::max];
}
