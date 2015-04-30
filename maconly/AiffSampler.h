//==============================================================================
// AiffSampler.h
// Created 2014-06-06
//==============================================================================

#ifndef BEATSERAI_AIFFSAMPLER
#define BEATSERAI_AIFFSAMPLER

#include "AiffReader.h"
#include "ESAudio.h"
#include "Complex.h"
//#include "MiniAiff.h"
#include <vector>


//==============================================================================
// Class AiffSampler
//==============================================================================

//------------------------------------------------------------------------------
// Provides a means of loading audio files,
// and copying chunks of them into a format used by FFT
class AiffSampler {
public:
  AiffReader aiff;
  SampleBufferF buffer;

public:
  AiffSampler () {}

  void load (char const* filename);
  inline bool stateIsOK () { return aiff.stateIsOK(); }
  void copyChunk (int firstSample, unsigned length, Complex<int16_t>* data);
  //float calculateChunkHFC (unsigned firstSample, unsigned length, Complex<float>* data);
  //std::vector<float> generateHFCSeries (unsigned windowSize, unsigned stepSize, int start);

  //int writeClicks (char* outFileName, std::vector<unsigned> clickSamples);
};


#endif // BEATSERAI_AIFFSAMPLER

