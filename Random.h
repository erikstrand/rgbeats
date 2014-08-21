//==============================================================================
// Random.h
// Created 2014/08/14
//==============================================================================

#include <stdint.h>

//------------------------------------------------------------------------------
class XorShift32 {
private:
   uint32_t _x, _y;

public:
   XorShift32 (uint32_t lowSeed, uint32_t highSeed) { setState(lowSeed, highSeed); }
   XorShift32 (): XorShift32(0, 0) {}
   void setState (uint32_t lowSeed, uint32_t highSeed);
   void state (uint32_t& x, uint32_t& y) const { x = _x; y = _y; }
   // these functions automatically increment state
   uint32_t uint32 () { next(); return _y; }

   // moves to next state
   XorShift32& next ();
   // this does not change the state - it returns the same number until next is called
   uint32_t const_uint32 () { return _y; }
};


