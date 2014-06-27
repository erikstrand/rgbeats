//==============================================================================
// RingBuffer.hpp
// Created 2014-06-15
//==============================================================================

#ifndef BEATSERAI_RINGBUFFER
#define BEATSERAI_RINGBUFFER

#include "Complex.h"
#include "Utils.h"

#include "esProfiler.h"
extern Profiler profiler;


//==============================================================================
// Ring Buffers
//==============================================================================

//------------------------------------------------------------------------------
// T must be zeroable with memset and copyable with memcpy.
template <typename T, unsigned N>
class RingBuffer {
public:
  T buffer[N];      // circular buffer
  unsigned index;   // index of the most recently added sample
  unsigned counter; // total number of samples added (modulo overflow)

public:
  inline RingBuffer (): index(N-1), counter(0) {
    memset(buffer, 0, N*sizeof(T));
  }

  inline void addSample (T x);
  inline T oldestSample () const { return buffer[modularPlusOne<N>(index)]; }
  inline T newestSample () const { return buffer[index]; }
  void copySamples (Complex<T>* dst, unsigned n) const;
};

template <typename T, unsigned N>
void RingBuffer<T, N>::addSample (T x) {
  ++counter;
  modularIncrement<N>(index);
  buffer[index] = x;
}

// Copies n samples from the buffer to an array of Complex numbers.
// The newest n samples are copied, arranged so the oldest is at dst[0].re()
// and the newest is at dst[n-1].re(). If n > N, samples are copied circularly.
template <typename T, unsigned N>
void RingBuffer<T, N>::copySamples (Complex<T>* dst, unsigned n) const {
  Complex<T>::zero(dst, n);
  unsigned src_i = index;
  unsigned dst_i = n - 1;
  // after dst_i==0, dst_i overflows and becomes >= n
  for (; dst_i<n; modularDecrement<N>(src_i), --dst_i) {
    dst[dst_i].re() = buffer[src_i];
  }
}


//------------------------------------------------------------------------------
// T must be zeroable with memset and copyable with memcpy.
template <typename T, unsigned N>
class RingBufferWithMedian {
public:
  RingBuffer<T, N> rbuffer; // ring buffer
  T sbuffer[N];             // same contents as rbuffer, but sorted
  int sum;
  int sumsquare;

public:
  inline RingBufferWithMedian (): sum(0), sumsquare(0) {
    memset(sbuffer, 0, N*sizeof(T));
  }

  void addSample (T x);
  inline T oldestSample () const { return rbuffer.oldestSample(); }
  inline T newestSample () const { return rbuffer.newestSample(); }
  inline void copySamples (Complex<T>* dst, unsigned n) const { rbuffer.copySamples(dst, n); }
  inline T median () const { return sbuffer[N>>1]; }
  inline T average () const { return sum/N; }
  inline T variance () const { return sumsquare - sum*sum; }
  inline T stddeviation () const { return sqrt(variance()); }
};

template <typename T, unsigned N>
void RingBufferWithMedian<T, N>::addSample (T x) {
  profiler.call(bufferAddSample);
  T target = rbuffer.oldestSample();
  sum = sum - target + x;
  sumsquare = sumsquare - target*target + x*x;
  unsigned i;
  if (target <= x) {
    i=0;
    while (sbuffer[i] != target) { ++i; }
    while (i+1 < N and sbuffer[i+1] < x) { sbuffer[i] = sbuffer[i+1]; ++i; }
  } else {
    i=N-1;
    while (sbuffer[i] != target) { --i; }
    // when i==0, i-1 underflows and is at least N
    while (i-1 < N-1 and sbuffer[i-1] > x) { sbuffer[i] = sbuffer[i-1]; --i; }
  }
  sbuffer[i] = x;
  rbuffer.addSample(x);
  profiler.finish(bufferAddSample);
}

#endif // BEATSERAI_RINGBUFFER

