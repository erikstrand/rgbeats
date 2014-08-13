//==============================================================================
// ColorUtils.h
// Created 2014-08-10
//==============================================================================

#ifndef COLORUTILS
#define COLORUTILS

#include <stdint.h>

//------------------------------------------------------------------------------
inline void splitComponents (unsigned color, unsigned& r, unsigned& g, unsigned& b) {
  r = color >> 16;
  g = (color >> 8) & 0xFF;
  b = color & 0xFF;
}

//------------------------------------------------------------------------------
inline unsigned combineComponents (unsigned r, unsigned g, unsigned b) {
  return (r << 16) | (g << 8) | b;
}
 
//------------------------------------------------------------------------------
class Color {
public:
  unsigned x1;
  unsigned x2;
  unsigned x3;
  bool hsv;
public: 
  Color (): x1(0), x2(0), x3(0), hsv(false) {}
  Color (unsigned c): x1(c >> 16), x2((c >> 8) & 0xFF), x3(c & 0xFF), hsv(false) {}
  Color (unsigned r, unsigned g, unsigned b): x1(r&0xFF), x2(g&0xFF), x3(b&0xFF), hsv(false) {}
  inline unsigned pack () const;
  inline unsigned rgbPack () const;
  inline unsigned hsvPack () const;
  inline void rgbRepresentation ();
  inline void hsvRepresentation ();
private:
  inline static unsigned packComponents (unsigned c1, unsigned c2, unsigned c3);
  void rgbComponents (unsigned& r, unsigned& g, unsigned& b) const;
  void hsvComponents (unsigned& r, unsigned& g, unsigned& b) const;
};

//------------------------------------------------------------------------------
unsigned Color::pack () const {
  return packComponents(x1, x2, x3);
}

//------------------------------------------------------------------------------
unsigned Color::rgbPack () const {
  if (hsv == false) {
    return pack();
  } else {
    unsigned r, g, b;
    rgbComponents(r, g, b);
    return packComponents(r, g, b);
  }
}

//------------------------------------------------------------------------------
unsigned Color::hsvPack () const {
  if (hsv == true) {
    return pack();
  } else {
    unsigned h, s, v;
    hsvComponents(h, s, v);
    return packComponents(h, s, v);
  }
}

//------------------------------------------------------------------------------
unsigned Color::packComponents (unsigned c1, unsigned c2, unsigned c3) {
  return (c1 << 16) | (c2 << 8) | c3;
}

//------------------------------------------------------------------------------
void Color::rgbRepresentation () {
  if (hsv == false) { return; }
  unsigned r, g, b;
  rgbComponents(r, g, b);
  x1 = r;
  x2 = g;
  x3 = b;
  hsv = false;
}

//------------------------------------------------------------------------------
void Color::hsvRepresentation () {
  if (hsv == true) { return; }
  unsigned h, s, v;
  hsvComponents(h, s, v);
  x1 = h;
  x2 = s;
  x3 = v;
  hsv = true;
}

//==============================================================================
// Log2
//==============================================================================

//------------------------------------------------------------------------------
uint16_t log2_fp (uint16_t x_16t);

#endif

