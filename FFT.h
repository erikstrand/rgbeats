//==============================================================================
// FFT.hpp
// Created 2/5/12.
//==============================================================================

#ifndef ESTDLIB_FFT
#define ESTDLIB_FFT

#include <cmath>
#include "Complex.h"


//==============================================================================
// Explanation
//==============================================================================
/**
 * This code computes the Discrete Fourier Transform of sampled data.
 * For a theoretical overview and derivation of the algorithm
 * see Theory.pdf in Code/FFT/Theory.
 */


//==============================================================================
// Declarations
//==============================================================================

//------------------------------------------------------------------------------
// Rearranges the n long array data so that the jth item becomes the kth,
// where k is the reverse of j in binary (ie 001 -> 100, 011 -> 110, etc).
// n must be a power of two.
template<class T>
inline void bitReverse (T* data, unsigned n);

//------------------------------------------------------------------------------
// Computes the Fourier Transform of the size long array data.
// size must be a power of 2.
template<typename T>
void fft (Complex<T>* data, unsigned n, bool inverse);


//==============================================================================
// Definitions
//==============================================================================

//------------------------------------------------------------------------------
template <class T>
void bitReverse (T* data, unsigned n) {
   unsigned i=1;    // i is iterated 0, 1, ..., n-1
   unsigned j=n>>1; // j is always the bit reverse of i
   unsigned m;      // m helps when we change j
   T temp;          // used for swaps
   // Note that we could start with i = j = 0, but we know nothing will
   // happen in this iteration so we just start with i = 1.
   
   for (; i<n; ++i) {
      // only swap if j>1 (otherwise we'd swap everything twice)
      if (j>i) {
         temp = data[j];
         data[j] = data[i];
         data[i] = temp;
      }
      
      // This increments j, in bit reversed fashion.
      // Turn all the leading ones into zeros, then turn the first
      // (original) zero into a one. This is just a reflected version
      // of what iterates a normal unsigned.
      m = n>>1;
      while (m >= 2 and j >= m) {
         j  -= m;
         m >>= 1;
      }
      j += m;
   }
}

//------------------------------------------------------------------------------
// Computes the Fourier Transform of the n long array data.
// n must be a power of 2.
template <typename T>
void fft (Complex<T>* data, unsigned n, bool inverse) {
   // bit reverse our data
   bitReverse< Complex<T> >(data, n);
   
   unsigned kmax = 2;   // points computed for each transform in this iteration
   unsigned psep = 1;   // separation of each pair in this iteration = kmax/2
   Complex<T> w;        // e^{-2 pi i j k / n}
   Complex<T> v(-1, 0); // v * w = e^{-2 pi i j (k+1) / n}
   while (kmax <= n) {
      w.re() = 1;
      w.im() = 0;
      // on each iteration we get k and k + kmax/2,
      // so k goes from 0 up to psep = kmax/2.
      for (unsigned k=0; k<psep; ++k) {
         for (unsigned a=k; a<n; a+=kmax) {
            // we will transform data[a] and data[b]
            unsigned b = a + psep;
            Complex<T> temp = data[b] * w;
            data[b] = data[a] - temp;            
            data[a] += temp;
         }
         w *= v;
      }
      
      // Next iteration we'll have twice as many points for each transform
      kmax <<= 1;
      psep <<= 1;
      
      // update v
      T cos = v.re();
      v.re() = sqrt(0.5*(1+cos));
      v.im() = sqrt(0.5*(1-cos));
      // for the forward transform w rotates clockwise
      if (!inverse) {
         v.conjugate();
      }
   }
   
   // We have to rescale the data for the inverse transform
   if (inverse) {
      T s = 1.0 / n;
      for (unsigned i=0; i<n; ++i) {
         data[i].re() *= s;
         data[i].im() *= s;
      }
   }
}

//------------------------------------------------------------------------------
// Replaces an array of Complex number (as output by a forward fft) with their power spectrum.
template <typename T>
void powerSpectrum (Complex<T>* data, unsigned n) {
  for (unsigned i=0; i<n; ++i) {
    data[i].re() = data[i].normsquare();
    data[i].im() = T(0);
  }
}

#endif // ESTDLIB_FFT

