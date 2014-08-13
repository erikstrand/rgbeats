//==============================================================================
// LEDRing.h
// Created 2014-08-07
//==============================================================================

#ifndef LEDRING
#define LEDRING

#include "esOctoWS2811.h"
#include "LightProgram.h"
#include "AudioAnalyzeHfcOnset.h"
#include "BeatTracker.h"


//------------------------------------------------------------------------------
template <typename TRACKER, unsigned LPS, unsigned N>
class LEDRing {
public:
   OctoWS2811* leds;
   AudioAnalyzeHfcOnset* myHFC;
   TRACKER* tracker;

public:
   LEDRing (OctoWS2811* leds, AudioAnalyzeHfcOnset* hfc, TRACKER* tracker);
   inline void setup ();
   inline void setPixel (unsigned n, unsigned color);
   void runProgram (LightProgram* program);
};

//------------------------------------------------------------------------------
template <typename TRACKER, unsigned LPS, unsigned N>
LEDRing<TRACKER, LPS, N>::LEDRing (OctoWS2811* leds, AudioAnalyzeHfcOnset* hfc, TRACKER* tracker)
: leds(leds), myHFC(hfc), tracker(tracker)
{}

//------------------------------------------------------------------------------
template <typename TRACKER, unsigned LPS, unsigned N>
void LEDRing<TRACKER, LPS, N>::setup () {
   leds->begin();
   leds->show();
}

//------------------------------------------------------------------------------
template <typename TRACKER, unsigned LPS, unsigned N>
void LEDRing<TRACKER, LPS, N>::setPixel (unsigned n, unsigned color) {
   if (n > 2*LPS) {
      n += 4*LPS;
   }
   leds->setPixel(n, color);
}

//------------------------------------------------------------------------------
template <typename TRACKER, unsigned LPS, unsigned N>
void LEDRing<TRACKER, LPS, N>::runProgram (LightProgram* program) {
  static unsigned id = 0;
  unsigned value, beat, beatpos;
  tracker->currentPosition(myHFC->sampleNumber, beat, beatpos);
  for (unsigned i=0; i<N; ++i) {
    value = program->pixel(i, id, beat, beatpos, (unsigned)(myHFC->rawHFC.mean()), myHFC->output);
    setPixel(i, value);
  }
  ++id;
  leds->show();
}

#endif

