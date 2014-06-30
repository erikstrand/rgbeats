//==============================================================================
// BeatTracker.hpp
// Created 2014-06-15
//==============================================================================

#ifndef BEATSERAI_BEATTRACKER
#define BEATSERAI_BEATTRACKER

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
  unsigned predictions[M];   // array where predicted beats are stored (in audio samples)
  unsigned nPredictions;     // number of beats stored in predictions
  unsigned firstFinal;       // beat number of first beat in predictions
  unsigned firstPreliminary; // beat number of first preliminary prediction in predictions
  unsigned anchorBeatNumber; // this is the beat number of currentHypothesis.anchorSample
  BeatHypothesis currentHypothesis;
  BeatHypothesis alternateHypothesis;

public:
  BeatTracker (): state(Unaligned), nPredictions(0) {}

  void addBeatHypothesis (BeatHypothesis const& ph);

  unsigned hypothesesAreConsistent (BeatHypothesis const& bh1, BeatHypothesis const& bh2) const;
  void predictBeats (bool update);
  void promotePreliminaryPredictions ();
  void clearPredictions ();


  // Returns the predicted location of the specified beat, in audio samples.
  // If the specified beat has already occured, it returns the last audio sample
  // from the most recet BeatHypothesis.
  inline unsigned beatLocation (unsigned beatnumber) const;
  inline unsigned firstBeatNumber () const { return firstFinal; }

  inline float tempoGuess () const {
    return 44100.0*60.0/((float) currentHypothesis.samplesPerBeat);
  }
};

//------------------------------------------------------------------------------
template <unsigned SPH, unsigned HPB, unsigned M>
void BeatTracker<SPH, HPB, M>::addBeatHypothesis (BeatHypothesis const& bh) {

  //std::cout << "Current state: " << state << '\n';
  if (state == Unaligned) {
    // should go through ProvisionallyAligned state
    // for now we just accept the new hypothesis
    state = Aligned;
    //std::cout << "Moving to state " << state << '\n';
    currentHypothesis = bh;
    anchorBeatNumber = 0;
    predictBeats(false);
    return;
  }

  if (state == Aligned) {
    unsigned beatdiff = hypothesesAreConsistent(currentHypothesis, bh);
    if (beatdiff > 0) {
      // hypothesies are consistent; we are still aligned
      //std::cout << "Still aligned"<< '\n';
      currentHypothesis = bh;
      anchorBeatNumber += beatdiff;
      predictBeats(true);
      return;
    } else {
      // hypotheses are inconsistent; we are now unsure
      state = Unsure;
      //std::cout << "Moving to state " << state << '\n';
      promotePreliminaryPredictions();
      alternateHypothesis = bh;
      return;
    }
  }

  if (state == Unsure) {
    // first see if we can re-align with our original hypothesis
    unsigned beatdiff = hypothesesAreConsistent(currentHypothesis, bh);
    if (beatdiff > 0) {
      state = Aligned;
      //std::cout << "Current hypothesis aligned - Moving to state " << state << '\n';
      currentHypothesis = bh;
      anchorBeatNumber += beatdiff;
      predictBeats(true);
      return;
    }
    // See if our latest hypothesis holds
    beatdiff = hypothesesAreConsistent(alternateHypothesis, bh);
    if (beatdiff > 0) {
      state = Aligned;
      //std::cout << "Alternate hypothesis aligned - Moving to state " << state << '\n';
      currentHypothesis = bh;
      anchorBeatNumber = beatdiff;
      predictBeats(true);
      return;
    }

    // if we made it this far, we have three inconsistent hypotheses in a row
    state = Unaligned;
    //std::cout << "Moving to state " << state << '\n';
    clearPredictions();
  }

}

//------------------------------------------------------------------------------
template <unsigned SPH, unsigned HPB, unsigned M>
unsigned BeatTracker<SPH, HPB, M>::hypothesesAreConsistent (BeatHypothesis const& bh1, BeatHypothesis const& bh2) const {
  // TODO: optimize this math
  // calculate the largest whole number of beats from bh1 that occur before bh2's anchor
  unsigned beatdiff = (bh2.anchorSample - bh1.anchorSample) / bh1.samplesPerBeat;
  // calculate the number of samples between bh2's anchor, and the closest beat from bh1
  unsigned samplediff = bh2.anchorSample - (bh1.anchorSample + beatdiff*bh1.samplesPerBeat);
  // If we're more than half a beat off, the next beat is closer to bh2.anchorSample
  if (samplediff > (bh1.samplesPerBeat >> 1)) {
    samplediff = bh1.samplesPerBeat - samplediff;
    ++beatdiff;
  }
  //std::cout << "Hypothesis difference is " << beatdiff << " beats and " << samplediff << " samples\n";
  // if the samplediff is less than 1/4th of a bh1 pulse, we consider the hypotheses aligned
  if (samplediff <= (bh1.samplesPerBeat >> 2)) {
    return beatdiff;
  } else {
    return 0;
  }
}

//------------------------------------------------------------------------------
template <unsigned SPH, unsigned HPB, unsigned M>
void BeatTracker<SPH, HPB, M>::predictBeats (bool update) {
  //std::cout << "Predicting beats\n";

  // We only record beats that fall after the last measured sample.
  // We predict two BeatHypotheses worth.
  // The first BeatHypothesis worth are "final" predictions,
  // in that we won't have a chance to revise them until they have already occured.
  // The second group are preliminary; we should be able to update them once before they pass.
  nPredictions = 0;
  unsigned beatpos = currentHypothesis.anchorSample + currentHypothesis.samplesPerBeat;
  unsigned newFirstFinal = anchorBeatNumber + 1;
  unsigned nFinalBeats = 0;
  unsigned firstPreliminarySample = currentHypothesis.measurementSample + SPH*HPB;
  unsigned firstUnpredictedSample = firstPreliminarySample + SPH*HPB;
  if (update) {
    if (anchorBeatNumber < firstPreliminary - 1) {
      // If our anchor is matched with the second to last final prediction, we skip one new prediction
      beatpos += currentHypothesis.samplesPerBeat;
      ++newFirstFinal;
    } else if (anchorBeatNumber >= firstPreliminary) {
      // If our anchor is matched with a beat that was only preliminary, we just use the old prediction
      predictions[nPredictions++] = predictions[firstPreliminary - firstFinal];
      --newFirstFinal;
    }
  }

  // add new predicted beats
  while (beatpos < firstUnpredictedSample and nPredictions < M) {
    predictions[nPredictions++] = beatpos;
    if (beatpos < firstPreliminarySample) {
      ++nFinalBeats;
      //std::cout << "  predicted final beat: " << beatpos << '\n';
    } else {
      //std::cout << "  predicted preliminary beat: " << beatpos << '\n';
    }
    beatpos += currentHypothesis.samplesPerBeat;
  }
  firstFinal = newFirstFinal;
  firstPreliminary = firstFinal + nFinalBeats;

  //std::cout << "Predicted " << nPredictions << " beats\n";
  //std::cout << "Anchor beat number: " << anchorBeatNumber << ", First final: " << firstFinal << ", first preliminary: " << firstPreliminary << '\n';
}

//------------------------------------------------------------------------------
template <unsigned SPH, unsigned HPB, unsigned M>
void BeatTracker<SPH, HPB, M>::promotePreliminaryPredictions () {
  unsigned i = 0;
  unsigned j = firstPreliminary - firstFinal;
  for (; j < nPredictions; ++i, ++j) {
    predictions[i] = predictions[j];
  }
  nPredictions = i;
  firstFinal = firstPreliminary;
  firstPreliminary = firstFinal + nPredictions;
}

//------------------------------------------------------------------------------
template <unsigned SPH, unsigned HPB, unsigned M>
void BeatTracker<SPH, HPB, M>::clearPredictions () {
  nPredictions = 0;
}

//------------------------------------------------------------------------------
template <unsigned SPH, unsigned HPB, unsigned M>
unsigned BeatTracker<SPH, HPB, M>::beatLocation (unsigned beatnumber) const {
  if (beatnumber < firstFinal) {
    return currentHypothesis.measurementSample;
  }
  if (beatnumber >= firstFinal + nPredictions) {
    return SPH * (currentHypothesis.measurementSample + 2*SPH*HPB);
  }
  return predictions[beatnumber - firstFinal];
}

#endif // BEATSERAI_BEATTRACKER
