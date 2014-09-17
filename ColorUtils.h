//==============================================================================
// ColorUtils.h
// Created 2014-08-10
//==============================================================================

#ifndef COLORUTILS
#define COLORUTILS

#include <stdint.h>

//------------------------------------------------------------------------------
inline unsigned packColor (unsigned c1, unsigned c2, unsigned c3) {
  return (c1 << 16) | (c2 << 8) | c3;
}

//------------------------------------------------------------------------------
inline void unpackColor (unsigned color, int& r, int& g, int& b) {
  r = static_cast<int>((color >> 16) & 0xFF);
  g = static_cast<int>((color >> 8) & 0xFF);
  b = static_cast<int>(color & 0xFF);
}

//------------------------------------------------------------------------------
class Color {
public:
  // For RGB representation, x1 is red, x2 green, and x3 blue. 0 <= xi < 256.
  // For HSV, x1 is hue, x2 value, and x3 chroma. 0 <= x1 < 1536, 0 <= x2 < 256, 0 <= x2 < 256.
  int x1;
  int x2;
  int x3;
  bool hsv;
public: 
  Color (): x1(0), x2(0), x3(0), hsv(false) {}
  Color (unsigned c): hsv(false) { unpackColor(c, x1, x2, x3); }
  Color (int r, int g, int b): x1(r), x2(g), x3(b), hsv(false) {}
  inline unsigned pack () const;
  inline unsigned rgbPack () const;
  inline unsigned hsvPack () const;
  inline void rgbRepresentation ();
  inline void hsvRepresentation ();
  inline Color& operator= (Color const& c) { x1 = c.x1; x2 = c.x2; x3 = c.x3; hsv = c.hsv; return *this; }
  inline static int addSaturate (int a, int b) { int c = a + b; if (c > 255) { c = 255; } else if (c < 0) { c = 0; } return c; }
  inline static int addSaturateOne (int a, int b) { int c = a + b; if (c > 255) { c = 255; } else if (c < 1) { c = (a == 0) ? 0 : 1; } return c; }
private:
  void rgbComponents (int& r, int& g, int& b) const;
  void hsvComponents (int& r, int& g, int& b) const;
};

//------------------------------------------------------------------------------
unsigned Color::pack () const {
  return packColor(x1, x2, x3);
}

//------------------------------------------------------------------------------
unsigned Color::rgbPack () const {
  if (hsv == false) {
    return pack();
  } else {
    int r, g, b;
    rgbComponents(r, g, b);
    return packColor(r, g, b);
  }
}

//------------------------------------------------------------------------------
unsigned Color::hsvPack () const {
  if (hsv == true) {
    return pack();
  } else {
    int h, s, v;
    hsvComponents(h, s, v);
    return packColor(h, s, v);
  }
}

//------------------------------------------------------------------------------
void Color::rgbRepresentation () {
  if (hsv == false) { return; }
  int r, g, b;
  rgbComponents(r, g, b);
  x1 = r;
  x2 = g;
  x3 = b;
  hsv = false;
}

//------------------------------------------------------------------------------
void Color::hsvRepresentation () {
  if (hsv == true) { return; }
  int h, s, v;
  hsvComponents(h, s, v);
  x1 = h;
  x2 = s;
  x3 = v;
  hsv = true;
}

#endif

