//==============================================================================
// RingBuffer.hpp
// Created 2014-06-15
//==============================================================================

#ifndef RGBEATS_RINGBUFFER
#define RGBEATS_RINGBUFFER

// this include goes with the commented out way to get stddev on arm
//include "arm_math.h"

#include "Complex.h"
#include "Utils.h"
#include "sqrt_integer.h"

//#compileif arduino
//#include "esProfiler.h"
//extern Profiler profiler;


//==============================================================================
// Ring Buffers
//==============================================================================

//------------------------------------------------------------------------------
// T must be zeroable with memset and copyable with memcpy.
template <typename T, unsigned N>
class RingBuffer {
protected:
  T buffer[N];      // circular buffer
  unsigned index;   // index of the most recently added sample
  unsigned counter; // total number of samples added (modulo overflow)

public:
  inline RingBuffer (): index(N-1), counter(0) {
    memset(buffer, 0, N*sizeof(T));
  }

  inline unsigned addSample (T x);
  inline unsigned samples () const { return counter; }
  inline T oldestSample () const { return buffer[modularPlusOne<N>(index)]; }
  inline T newestSample () const { return buffer[index]; }
  inline T nthNewestSample (unsigned n) const { return n <= index ? buffer[index - n] : buffer[N - (n - index)]; }
  void copySamples (Complex<T>* dst, unsigned n, unsigned end) const;
  inline void copySamples (Complex<T>* dst, unsigned n) const;
};

template <typename T, unsigned N>
unsigned RingBuffer<T, N>::addSample (T x) {
  modularIncrement<N>(index);
  buffer[index] = x;
  return ++counter;
}

// Copies n samples from the buffer to an array of Complex numbers.
// The newest n samples up to and including end are copied, arranged so the oldest is at dst[0].re()
// and the newest is at dst[n-1].re(). If n > N, samples are copied circularly.
// If counter - end > N, behavior is undefined.
template <typename T, unsigned N>
void RingBuffer<T, N>::copySamples (Complex<T>* dst, unsigned n, unsigned end) const {
  Complex<T>::zero(dst, n);
  unsigned src_i = (N + index - (counter - end)) % N; // does compiler optimize %N to &(N-1) ?
  unsigned dst_i = n - 1;
  // after dst_i==0, dst_i overflows and becomes >= n
  for (; dst_i<n; modularDecrement<N>(src_i), --dst_i) {
    dst[dst_i].re() = buffer[src_i];
  }
}

template <typename T, unsigned N>
void RingBuffer<T, N>::copySamples (Complex<T>* dst, unsigned n) const {
   copySamples(dst, n, counter);
}


//------------------------------------------------------------------------------
// Version of RingBuffer that can calculate mean, median, and std deviation.
// Specialized for int32_t.
// N MUST BE MULTIPLE OF 4. To avoid potential overflow, N < 
// Template based on type T is difficult because
// 1) algorithms would differ for floating point (rather than integer) T
// 2) for integers wider intermediate types are necessary to prevent overflow.
// The main difference is that a sorted version of all contents is stored,
// as well as a running sum. This allows median and mean to be calculated
// easily. Std dev is calculated fresh each time.
template <unsigned N>
class RingBufferWithMedian : public RingBuffer<int32_t, N> {
private:
  int32_t sbuffer[N];  // same contents as rbuffer, but sorted small to large
  int64_t _sum;        // sum of all samples in buffer
  /*
   * Note: recording a running total for std deviation does not work because:
   * 1) sum (buffer[i] - mu)^2 must be recalculated when mu changes
   * 2) sum buffer[i]^2 can overflow even uin64_t if buffer[i] ~ 800M
   */

public:
  inline RingBufferWithMedian (): _sum(0) {
    memset(sbuffer, 0, N*sizeof(int32_t));
  }

  // Note that smallest/largest samples are relative to current contents (not cumulative)
  inline int32_t smallestSample () const { return sbuffer[0]; }
  inline int32_t largestSample () const { return sbuffer[N-1]; }
  inline int32_t nthSmallestSample (unsigned n) const { return sbuffer[n]; }
  inline int64_t sum () const { return _sum; }
  inline int32_t median () const { return sbuffer[N/2]; }
  inline int32_t mean () const { return (_sum + N/2)/N; }
  // Warning: this function can overflow, but is unlikely to with natural audio data.
  // Worst case is N/2 samples of smallest int32 and N/2 samples of largest int32.
  // Then squareMean can overflow when i=3!
  inline int32_t stddeviationDangerous () {
    // for ARM may be best to replace this whole method with:
    // arm_std_q31(rbuffer.buffer, N, &sigma); return sigma;
    int32_t mu = mean();
    int64_t difference;
    int64_t squareMean = 0;
    for (unsigned i=0; i<N; ++i) {
      difference = RingBuffer<int32_t,N>::buffer[i] - mu;
      squareMean += difference * difference;
    }
    squareMean = (squareMean + ((N-1)/2)) / (N-1); // slightly more accurate version of squareMean/(N-1)
    squareMean = sqrt_uint32(squareMean);
    return squareMean;
  }
  inline uint32_t stddeviation () {
    // for ARM may be best to replace this whole method with:
    // arm_std_q31(rbuffer.buffer, N, &sigma); return sigma;
    int32_t mu = mean();
    int64_t dif0, dif1, dif2, dif3;
    uint64_t temp;
    uint64_t squareMean = 0;
    for (unsigned i=0; i<N; i+=4) {
      // In the worst case scenario for sum of 4 squares (all difs are -2^31)
      // we overflow exactly to 0.
      dif0 = RingBuffer<int32_t,N>::buffer[i] - mu;
      dif1 = RingBuffer<int32_t,N>::buffer[i+1] - mu;
      dif2 = RingBuffer<int32_t,N>::buffer[i+2] - mu;
      dif3 = RingBuffer<int32_t,N>::buffer[i+3] - mu;
      temp = dif0*dif0 + dif1*dif1 + dif2*dif2 + dif3*dif3;
      if (temp == 0 and dif3 != 0) {
        // temp should be 2^64 exactly; the following expression is 2^64/(N-1)
        // ToDo: would be good to round here instead of floor
        squareMean += (1ull << 63) / (N - 1) * 2;
      } else {
        squareMean += temp / (N - 1);
      }
    }
    return sqrt_uint64(squareMean);
  }
  inline unsigned addSample (int32_t x);
};

template <unsigned N>
unsigned RingBufferWithMedian<N>::addSample (int32_t x) {
  // this is the sample we will need to replace
  int32_t target = RingBuffer<int32_t,N>::oldestSample();

  // update sample total
  _sum = _sum - target + x;

  // find an replace oldest sample (target) with newest (x)
  unsigned i;
  if (target <= x) {
    // start from beginning (lowest number)
    i=0;
    // scan until sbuffer[i] == target
    while (sbuffer[i] != target) { ++i; }
    // copy sbuffer[i+1] into sbuffer[i] until sbuffer[i] is where x should go
    while (i+1 < N and sbuffer[i+1] < x) { sbuffer[i] = sbuffer[i+1]; ++i; }
  } else {
    // start from end (highest number)
    i=N-1;
    // scan down until sbuffer[i] == target
    while (sbuffer[i] != target) { --i; }
    // when i==0, i-1 underflows and is at least N
    while (i-1 < N-1 and sbuffer[i-1] > x) { sbuffer[i] = sbuffer[i-1]; --i; }
  }
  sbuffer[i] = x;
  return RingBuffer<int32_t,N>::addSample(x);
}

#endif

