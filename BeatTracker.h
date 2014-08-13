//==============================================================================
// BeatTracker.hpp
// Created 2014-06-15
//==============================================================================

#ifndef RGBEATS_BEATTRACKER
#define RGBEATS_BEATTRACKER

#include <cmath>
#include "utils.h"

// debug
//#include <iostream>

#include "esProfiler.h"
extern Profiler profiler;


//==============================================================================
// BeatTracker
//==============================================================================

//------------------------------------------------------------------------------
// States of a BeatTracker
enum BeatTrackerState {Aligned, Unsure, Unaligned, ProvisionallyAligned}; 

//------------------------------------------------------------------------------
/*
 * Maintains a buffer of predicted beat locations.
 * When steadily tracking beats, all beats are assigned a beat number (starting from 0).
 * SPH is audio samples per HFC sample.
 * S is the number of HFC samples per BeatHypothesis.
 * M is the maximum number of beats that we will predict.
 */
template <unsigned SPH, unsigned HPB, unsigned M>
class BeatTracker {
public:
  BeatTrackerState state;
  unsigned anchorBeatNumber; // this is the beat number of hypothesis1.anchorSample
  BeatHypothesis hypothesis1;
  BeatHypothesis hypothesis2;

  /*
  unsigned predictions[M];   // array where predicted beats are stored (in audio samples)
  unsigned nPredictions;     // number of beats stored in predictions
  unsigned firstFinal;       // beat number of first beat in predictions
  unsigned firstPreliminary; // beat number of first preliminary prediction in predictions
  */

public:
  BeatTracker (): state(Unaligned) {}

  void addBeatHypothesis (BeatHypothesis const& ph);
  unsigned hypothesesAreConsistent (BeatHypothesis const& bh1, BeatHypothesis const& bh2) const;

  /*
  void predictBeats (bool update);
  void promotePreliminaryPredictions ();
  void clearPredictions ();
  */

  // Returns the predicted location of the specified beat, in audio samples.
  // If the specified beat has already occured, it returns the last audio sample
  // from the most recet BeatHypothesis.
  inline unsigned beatLocation (unsigned beatnumber) const;
  inline void currentPosition (unsigned sampleNumber, unsigned& beatNumber, unsigned& beatPos) const;

  inline float tempoGuess () const {
    return 44100.0*60.0/((float) hypothesis1.samplesPerBeat);
  }
};

//------------------------------------------------------------------------------
template <unsigned SPH, unsigned HPB, unsigned M>
void BeatTracker<SPH, HPB, M>::addBeatHypothesis (BeatHypothesis const& bh) {

/*
  Serial.print("State: ");
  Serial.print(state);
  Serial.println();
  Serial.print("Adding beat. Current anchor number: ");
  Serial.print(anchorBeatNumber);
  Serial.println();
  Serial.print("new anchor sample: ");
  Serial.print(bh.anchorSample);
  Serial.print(", samplesPerBeat: ");
  Serial.print(bh.samplesPerBeat);
  Serial.print(", measurement sample: ");
  Serial.print(bh.measurementSample);
  Serial.println();
  */

  //std::cout << "Current state: " << state << '\n';
  if (state == Unaligned) {
    //std::cout << "Moving to state " << state << '\n';
    anchorBeatNumber = 0;
    // should go through ProvisionallyAligned state
    // for now we just accept the new hypothesis
    hypothesis1 = bh;
    state = Aligned;
    return;
  }

  if (state == Aligned) {
    unsigned beatdiff = hypothesesAreConsistent(hypothesis1, bh);
    if (beatdiff > 0) {
      // hypothesies are consistent; we are still aligned
      //std::cout << "Still aligned"<< '\n';
      hypothesis1 = bh;
      anchorBeatNumber += beatdiff;
      //predictBeats(true);
      return;
    } else {
      // hypotheses are inconsistent; we are now unsure
      state = Unsure;
      //std::cout << "Moving to state " << state << '\n';
      //promotePreliminaryPredictions();
      hypothesis2 = bh;
      return;
    }
  }

  if (state == Unsure) {
    // first see if we can re-align with our original hypothesis
    unsigned beatdiff = hypothesesAreConsistent(hypothesis1, bh);
    if (beatdiff > 0) {
      state = Aligned;
      //Serial.print("hypothesis1 aligned - Moving to aligned");
      //Serial.println();
      hypothesis1 = bh;
      anchorBeatNumber += beatdiff;
      //predictBeats(true);
      return;
    }
    // See if our latest hypothesis holds
    beatdiff = hypothesesAreConsistent(hypothesis2, bh);
    if (beatdiff > 0) {
      state = Aligned;
      //Serial.print("hypothesis2 aligned - Moving to aligned");
      //Serial.println();
      //std::cout << "Alternate hypothesis aligned - Moving to state " << state << '\n';
      hypothesis1 = bh;
      anchorBeatNumber = beatdiff;
      //predictBeats(true);
      return;
    }

    // if we made it this far, we have three inconsistent hypotheses in a row
    state = Unaligned;
    //std::cout << "Moving to state " << state << '\n';
    //clearPredictions();
  }

}

//------------------------------------------------------------------------------
template <unsigned SPH, unsigned HPB, unsigned M>
unsigned BeatTracker<SPH, HPB, M>::hypothesesAreConsistent (BeatHypothesis const& bh1, BeatHypothesis const& bh2) const {
  // calculate the largest whole number of beats from bh1 that occur before bh2's anchor
  unsigned beatdiff = (bh2.anchorSample - bh1.anchorSample) / bh2.samplesPerBeat;
  // calculate the number of samples between bh1's anchor, and the closest beat from bh2
  unsigned samplediff = bh2.anchorSample - beatdiff*bh2.samplesPerBeat - bh1.anchorSample;

  // calculate the largest whole number of beats from bh1 that occur before bh2's anchor
  //unsigned beatdiff = (bh2.anchorSample - bh1.anchorSample) / bh1.samplesPerBeat;
  // calculate the number of samples between bh2's anchor, and the closest beat from bh1
  //unsigned samplediff = bh2.anchorSample - (bh1.anchorSample + beatdiff*bh1.samplesPerBeat);

  // If we're more than half a beat off, the next beat is closer to bh2.anchorSample
  if (samplediff > (bh2.samplesPerBeat / 2)) {
    samplediff = bh2.samplesPerBeat - samplediff;
    ++beatdiff;
  }
  /*
  Serial.print("testing consistency. beat diff: ");
  Serial.print(beatdiff);
  Serial.print(", sample diff: ");
  Serial.print(samplediff);
  Serial.println();
  */
  //std::cout << "Hypothesis difference is " << beatdiff << " beats and " << samplediff << " samples\n";
  // if the samplediff is less than 1/4th of a bh1 pulse, we consider the hypotheses aligned
  return ((samplediff <= (bh2.samplesPerBeat / 4)) ? beatdiff : 0);
}

//------------------------------------------------------------------------------
template <unsigned SPH, unsigned HPB, unsigned M>
unsigned BeatTracker<SPH, HPB, M>::beatLocation (unsigned beatnumber) const {
  int beatDifference = static_cast<int>(beatnumber) - static_cast<int>(anchorBeatNumber);

  if (beatnumber > anchorBeatNumber) {
    return hypothesis1.anchorSample + hypothesis1.samplesPerBeat*(beatnumber - anchorBeatNumber);
  } else {
    // this could underflow, so this branch should be invoked with caution
    return hypothesis1.anchorSample - hypothesis1.samplesPerBeat*(anchorBeatNumber - beatnumber);
  }
}

//------------------------------------------------------------------------------
template <unsigned SPH, unsigned HPB, unsigned M>
inline void BeatTracker<SPH, HPB, M>::currentPosition (unsigned sampleNumber, unsigned& beatNumber, unsigned& beatPos) const {
  beatNumber = (sampleNumber - hypothesis1.anchorSample) / hypothesis1.samplesPerBeat;
  beatPos = (sampleNumber - beatNumber*hypothesis1.samplesPerBeat) * 1024 / hypothesis1.samplesPerBeat;
}

#endif
