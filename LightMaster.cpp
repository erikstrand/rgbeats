//==============================================================================
// LightMaster.cpp
// Created 2014-08-21
//==============================================================================

#include "LightMaster.h"


//------------------------------------------------------------------------------
ProgramGenerator::ProgramGenerator () {
  basePrograms[0] = &solid;
  basePrograms[1] = &spectrum;

  effectPrograms[0] = &flicker;
  effectPrograms[1] = &shiftBright;
  effectPrograms[2] = &shiftColor;
  effectPrograms[4] = &repeater;
  effectPrograms[3] = &vu;
  effectPrograms[5] = &lanternSplitter;

  buildProgram(123456);
}

//------------------------------------------------------------------------------
void ProgramGenerator::buildProgram (unsigned randomSeed) {
  rand.setState(randomSeed, randomSeed);

  LightProgram* baseProgram = 0;
  LightProgram* lastProgram = 0;
  LightFilter* effect1 = 0;
  LightFilter* effect2 = 0;
  LightFilter* effect3 = 0;

  // identify a base program
  if (rand.uint32() < 429496729) {
    baseProgram = basePrograms[1];
  } else {
    baseProgram = basePrograms[0];
  }
  baseProgram->init(rand);
  lastProgram = baseProgram;

  // identify an effect program
  if (rand.uint32() & 0x1) {
    effect1 = effectPrograms[rand.uint32() % nRepeatableEffect];
    effect1->init(rand);
    effect1->connect(lastProgram);
    lastProgram = effect1;
  }

  // identify an effect program
  if (rand.uint32() & 0x7 == 0) {
    do {
      effect2 = effectPrograms[rand.uint32() % nRepeatableEffect];
    } while (effect2 == effect1);
    effect2->init(rand);
    effect2->connect(lastProgram);
    lastProgram = effect2;
  }

  // identify an effect program
  do {
    effect3 = effectPrograms[rand.uint32() % nEffect];
  } while (effect3 == effect1 or effect3 == effect2);
  effect3->init(rand);
  effect3->connect(lastProgram);

  // deal with lanterns
  if (effect3 == &lanternSplitter) {
    effect3->connect(lastProgram, 1);
    lanternsolid.init(rand);
    effect3->connect(&lanternsolid, 0);
  }
  lastProgram = effect3;

  p1 = effect3;
}

//------------------------------------------------------------------------------
void ProgramGenerator::lanternsMode () {
  lanternsolid.setColor(Color(0xA02000));
  offsolid.setColor(Color(0x000000));
  flicker.lanternsMode();
  flicker.connect(&lanternsolid);
  lanternSplitter.connect(&flicker, 0);
  lanternSplitter.connect(&offsolid, 1);
  p1 = &lanternSplitter;
}

//------------------------------------------------------------------------------
void LightMaster::pixel (unsigned x, MusicState const& state, Color& color) {
  // if we're in a transition, wait for it to finish before starting a new one
  if (!fader.inTransition()) {
    // start transition to new mode if necessary
    if (beatsMode != targetMode) {
      if (targetMode) {
        // transition to beats mode
        rand.setState(state.random(), 12345);
        lastProgramStart = state.sample;
        nextGenerator->buildProgram(state.random());
        fader.cueProgram(nextGenerator);
        fader.beginTransition(transitionLength);
        ProgramGenerator* tempGenerator = currentGenerator;
        currentGenerator = nextGenerator;
        nextGenerator = tempGenerator;
        beatsMode = true;
      } else {
        // transition to lanterns mode
        nextGenerator->lanternsMode();
        fader.cueProgram(nextGenerator);
        fader.beginTransition(modeTransitionLength);
        ProgramGenerator* tempGenerator = currentGenerator;
        currentGenerator = nextGenerator;
        nextGenerator = tempGenerator;
        beatsMode = false;
      }
    } else {
      if (beatsMode) {
        unsigned passedSamples = state.sample - lastProgramStart;
        if (passedSamples >= programLength) {
          rand.setState(state.random(), 12345);
          lastProgramStart = state.sample;
          nextGenerator->buildProgram(state.random());
          fader.cueProgram(nextGenerator);
          fader.beginTransition(transitionLength);
          ProgramGenerator* tempGenerator = currentGenerator;
          currentGenerator = nextGenerator;
          nextGenerator = tempGenerator;
        }
      }
    }
  }

  controls.pixel(x, state, color);
  //fader.pixel(x, state, color);
}

