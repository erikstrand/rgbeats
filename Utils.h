//==============================================================================
// utils.hpp
// Created 2014-06-10
//==============================================================================

#ifndef RGBEATS_UTILS
#define RGBEATS_UTILS

#include "Complex.h"


//==============================================================================
// Modular arithmetic
//==============================================================================

//------------------------------------------------------------------------------
template <unsigned N>
inline bool isMultipleOf (unsigned m) {
  return m % N == 0;
}

template <> inline bool isMultipleOf<8>    (unsigned m) { return (m & 0x7)   == 0; }
template <> inline bool isMultipleOf<16>   (unsigned m) { return (m & 0xF)   == 0; }
template <> inline bool isMultipleOf<32>   (unsigned m) { return (m & 0x1F)  == 0; }
template <> inline bool isMultipleOf<64>   (unsigned m) { return (m & 0x3F)  == 0; }
template <> inline bool isMultipleOf<128>  (unsigned m) { return (m & 0x7F)  == 0; }
template <> inline bool isMultipleOf<256>  (unsigned m) { return (m & 0xFF)  == 0; }
template <> inline bool isMultipleOf<512>  (unsigned m) { return (m & 0x1FF) == 0; }
template <> inline bool isMultipleOf<1024> (unsigned m) { return (m & 0x3FF) == 0; }
template <> inline bool isMultipleOf<2048> (unsigned m) { return (m & 0x7FF) == 0; }


//------------------------------------------------------------------------------
template <unsigned N>
inline unsigned modularPlusOne (unsigned m) {
  unsigned next = ++m;
  if (next < N) {
    return next;
  }
  if (next == N) {
    return 0;
  }
  return next % N;
}

template <> inline unsigned modularPlusOne<2>    (unsigned m) { return ((m + 1) & 0x1);   }
template <> inline unsigned modularPlusOne<4>    (unsigned m) { return ((m + 1) & 0x3);   }
template <> inline unsigned modularPlusOne<8>    (unsigned m) { return ((m + 1) & 0x7);   }
template <> inline unsigned modularPlusOne<16>   (unsigned m) { return ((m + 1) & 0xF);   }
template <> inline unsigned modularPlusOne<32>   (unsigned m) { return ((m + 1) & 0x1F);  }
template <> inline unsigned modularPlusOne<64>   (unsigned m) { return ((m + 1) & 0x3F);  }
template <> inline unsigned modularPlusOne<128>  (unsigned m) { return ((m + 1) & 0x7F);  }
template <> inline unsigned modularPlusOne<256>  (unsigned m) { return ((m + 1) & 0xFF);  }
template <> inline unsigned modularPlusOne<512>  (unsigned m) { return ((m + 1) & 0x1FF); }
template <> inline unsigned modularPlusOne<1024> (unsigned m) { return ((m + 1) & 0x3FF); }
template <> inline unsigned modularPlusOne<2048> (unsigned m) { return ((m + 1) & 0x7FF); }


//------------------------------------------------------------------------------
template <unsigned N>
inline unsigned& modularIncrement (unsigned& m) {
  return m = modularPlusOne<N>(m);
}

template <> inline unsigned& modularIncrement<8>    (unsigned& m) { ++m; return m &= 0x7;   }
template <> inline unsigned& modularIncrement<16>   (unsigned& m) { ++m; return m &= 0xF;   }
template <> inline unsigned& modularIncrement<32>   (unsigned& m) { ++m; return m &= 0x1F;  }
template <> inline unsigned& modularIncrement<64>   (unsigned& m) { ++m; return m &= 0x3F;  }
template <> inline unsigned& modularIncrement<128>  (unsigned& m) { ++m; return m &= 0x7F;  }
template <> inline unsigned& modularIncrement<256>  (unsigned& m) { ++m; return m &= 0xFF;  }
template <> inline unsigned& modularIncrement<512>  (unsigned& m) { ++m; return m &= 0x1FF; }
template <> inline unsigned& modularIncrement<1024> (unsigned& m) { ++m; return m &= 0x3FF; }
template <> inline unsigned& modularIncrement<2048> (unsigned& m) { ++m; return m &= 0x7FF; }


//------------------------------------------------------------------------------
template <unsigned N>
inline unsigned& modularDecrement (unsigned& m) {
  if (m == 0) { m = N - 1; }
  else if (m < N) { --m; }
  else { m = (m - 1) % N; }
  return m;
}

template <> inline unsigned& modularDecrement<8>    (unsigned& m) { --m; return m &= 0x7;   }
template <> inline unsigned& modularDecrement<16>   (unsigned& m) { --m; return m &= 0xF;   }
template <> inline unsigned& modularDecrement<32>   (unsigned& m) { --m; return m &= 0x1F;  }
template <> inline unsigned& modularDecrement<64>   (unsigned& m) { --m; return m &= 0x3F;  }
template <> inline unsigned& modularDecrement<128>  (unsigned& m) { --m; return m &= 0x7F;  }
template <> inline unsigned& modularDecrement<256>  (unsigned& m) { --m; return m &= 0xFF;  }
template <> inline unsigned& modularDecrement<512>  (unsigned& m) { --m; return m &= 0x1FF; }
template <> inline unsigned& modularDecrement<1024> (unsigned& m) { --m; return m &= 0x3FF; }
template <> inline unsigned& modularDecrement<2048> (unsigned& m) { --m; return m &= 0x7FF; }


//==============================================================================
// Peak Tests
//==============================================================================

//------------------------------------------------------------------------------
template <typename T, unsigned N>
inline bool isPeak (Complex<T> const* x) { return isPeak<T, N-1>(x) && x->re() > (x-N)->re() && x->re() > (x+N)->re(); }

template <>
inline bool isPeak<float, 0> (Complex<float> const* x) { return true; }


//==============================================================================
// BeatHypothesis
//==============================================================================

//------------------------------------------------------------------------------
// A guess about the frequency and phase of the dominant pulse of an audio sample.
class BeatHypothesis {
public:
  unsigned measurementSample; // Last audio sample in the window that this hypothesis is based on
  unsigned anchorSample;      // Position of a known beat in audio samples (determines phase)
  unsigned samplesPerBeat;    // Number of audio samples per beat (determines frequency)
  // samplesPerBeat will always be less than or equal to measurementSample
  // samplesPerBeat will always be greater than or equal to measurementSample - samplesPerBeat
  // TODO: check that above conditions are enforced

public:
  inline BeatHypothesis& operator= (BeatHypothesis const& rhs) {
    memcpy(this, &rhs, sizeof(BeatHypothesis));
    return *this;
  }
};

#endif

