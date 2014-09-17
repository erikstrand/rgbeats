//==============================================================================
// LightMaster.h
// Created 2014-08-21
//==============================================================================

#ifndef RGBEATS_LIGHTMASTER
#define RGBEATS_LIGHTMASTER

#include "LightProgram.h"
#include "LightProgramTemplates.h"
#include "Random.h"
#include "MusicState.h"
#include "ColorUtils.h"

const int NLEDS = 692;


//==============================================================================
// ProgramGenerator
//==============================================================================

class ProgramGenerator : public LightProgram {
public:
  LightProgram* p1;
  XorShift32 rand;

  const static unsigned nBase = 2;
  const static unsigned nEffect = 6;
  const static unsigned nRepeatableEffect = 4;

  // LightProgram arrays
  LightProgram* basePrograms[nBase];
  LightFilter* effectPrograms[nEffect];

  // base programs
  Solid<NLEDS> lanternsolid;
  Solid<NLEDS> offsolid;
  Solid<NLEDS> solid;
  SpectrumProgram<NLEDS> spectrum;

  // effect filters
  FlickerBrightness<NLEDS> flicker;
  ShiftBrightnessOnset<NLEDS> shiftBright;
  ShiftColorOnset<NLEDS> shiftColor;

  // geometry
  VUMeter<NLEDS> vu;
  ProgramRepeater<NLEDS> repeater;

  // lanterns
  Lanterns<NLEDS> lanternSplitter;

public:
  ProgramGenerator ();
  void buildProgram (unsigned randomSeed);
  void lanternsMode ();
  void pixel (unsigned x, MusicState const& state, Color& color) { p1->pixel(x, state, color); }
};


//==============================================================================
// LightMaster
//==============================================================================

// Runs the light show
class LightMaster : public LightProgram {
public:
  FinalControl controls;
  FadeTransition fader;
  ProgramGenerator generator1;
  ProgramGenerator generator2;
  ProgramGenerator* currentGenerator;
  ProgramGenerator* nextGenerator;

  static const unsigned transitionLength = 250;
  static const unsigned programLength = 5000; // includes transition lengths
  static const unsigned modeTransitionLength = 150;
  bool beatsMode;
  bool targetMode;
  unsigned lastProgramStart;

  LightProgram* p1;
  XorShift32 rand;

public:
  LightMaster (): currentGenerator(&generator1), nextGenerator(&generator2), beatsMode(false), targetMode(false), lastProgramStart(0) {
    controls.connect(&fader);
    fader.p1 = &generator1;
    generator1.lanternsMode();
  }
  void pixel (unsigned x, MusicState const& state, Color& color);
  inline void setMode (bool setToBeats) { targetMode = setToBeats; }
};

#endif

