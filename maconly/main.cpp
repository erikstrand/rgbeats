//==============================================================================
// main.cpp
// Created 2014-06-06
//==============================================================================

#include <cmath>
#include "Complex.h"
//#include "FFT.h"
//#include "RingBuffer.h"

#include "maconly/AiffSampler.h"
#include "HFCMeasure.h"
#include "FFTKiss.h"

/*
#include "Onset.hpp"
#include "BeatExtractor.hpp"
#include "BeatTracker.hpp"
#include <vector>
*/
#include <iostream>

using namespace std;

//----------------------------------------------------------------------------
int main (int argc, char** argv) {

  // load an audio file
  AiffSampler sampler;
  if (sampler.stateIsOK()) {
    std::cout << "state is fine\n";
  } else {
    std::cout << "no good\n";
  }
  sampler.load("music/TwoCanWin.aiff");
  if (!sampler.stateIsOK()) {
    cout << "Could not open file.\n";
    exit(1);
  }

  // declarations and definitions
  Complex<int16_t> buffer[1024];
  int16_t* simplebuffer = buffer[0].asArray();
  uint16_t spectrum[512];
  HFCMeasure <1024, FFTKiss<1024, 32> > hfcMeasure;
  HFCRecord <32, 1024> hfcRecord;
  int32_t hfc;

  // move samples to buffer
  sampler.copyChunk(0, 1024, buffer);
  hfc = hfcMeasure.calculateSpectrumAndHFC(simplebuffer, spectrum);
  for (unsigned i=0; i<32; ++i) {
    hfcRecord.addSample(hfc + 1000+i);
  }

  std::cout << '\n';
  std::cout << "Spectrum: " << hfc << '\n';
  for (unsigned i=0; i<512; ++i) {
    std::cout << spectrum[i] << ", ";
  }
  std::cout << '\n';
  std::cout << "HFC: " << hfc << '\n';

  std::cout << "Sum: " << hfcRecord.rawHFC.sum() << '\n';
  std::cout << '\n';
  std::cout << "Mean: " << hfcRecord.rawHFC.mean() << '\n';
  std::cout << '\n';

  std::cout << "raw buffer: " << '\n';
  for (unsigned i=0; i<32; ++i) {
    std::cout << hfcRecord.rawHFC.nthNewestSample(i) << ", ";
  }
  std::cout << '\n';
  std::cout << "stddev: " << hfcRecord.rawHFC.stddeviation() << '\n';
}

