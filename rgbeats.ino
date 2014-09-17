#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include "arm_math.h"

#include "RingBuffer.h"
#include "esOctoWS2811.h"
#include "AudioAnalyzeHfcOnset.h"
#include "BeatExtractor.h"
#include "BeatTracker.h"
#include "esProfiler.h"
#include "LightProgram.h"
#include "LightProgramTemplates.h"
#include "LightMaster.h"
#include "MusicState.h"
#include "LEDRing.h"
#include "ColorUtils.h"


//------------------------------------------------------------------------------
// Profiler
Profiler profiler;


//------------------------------------------------------------------------------
// Audio constants
const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioInputI2S       audioInput;         // audio shield: mic or line-in
AudioAnalyzeHfcOnset  myHFC(20);
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components
//
AudioConnection c1(audioInput, 0, audioOutput, 0);
AudioConnection c2(audioInput, 0, myHFC, 0);
AudioConnection c3(audioInput, 1, audioOutput, 1);

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;


//------------------------------------------------------------------------------
// WS2811 constants
const int ledsPerStrip = 173;
const int nStrips = 4;
const int nLeds = 692;
DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

#define RED    0x040000
#define RED2   0x0A0000
#define GREEN  0x000400
#define GREEN2 0x000A00
#define BLUE   0x000004
#define BLUE2  0x00000A
#define YELLOW 0x020300
#define YELLOW2 0x050700
#define PINK   0x020002
#define PINK2  0x050005
#define ORANGE 0x030300
#define ORANGE2 0x050500
#define WHITE  0x020202

unsigned long t1 = 0;
int color = 0;
int colors[] = {RED, RED2, ORANGE, ORANGE2, GREEN, GREEN2, BLUE, BLUE2, PINK, PINK2, WHITE};

//extern const unsigned cubehelix[];
extern const unsigned cubehelix2[];

//------------------------------------------------------------------------------
// function that maps [0, 255] to RGB values
//typedef int (*ColorScale) (int);

int cscaleCubeHelix (int x) {
  return cubehelix2[x];
}

//------------------------------------------------------------------------------
//RingBufferWithMedian<int, 64> hfcBuffer;
const unsigned hfcWindow = 512;
const unsigned samplesPerHFC = 512;
const unsigned HFCPerBeatHypothesis = 128;
BeatExtractor<hfcWindow, samplesPerHFC> extractor;
BeatTracker<hfcWindow, samplesPerHFC, HFCPerBeatHypothesis> tracker;
unsigned lastBeat = 0;
unsigned newBeat = 0;
unsigned beatPos = 0;

//------------------------------------------------------------------------------
LEDRing< BeatTracker<hfcWindow, samplesPerHFC, HFCPerBeatHypothesis>, ledsPerStrip, nLeds > ledring(&leds, &myHFC, &tracker);

//------------------------------------------------------------------------------
CubeHelixScale cubehelixScale;
const int COLORPIN = A11;

//------------------------------------------------------------------------------
RingBufferWithMedian<int, 64> brightnessControlBuffer;
RingBufferWithMedian<int, 64> colorControlBuffer;
LightMaster lightMaster;
// base programs
Solid<nLeds> solid1;
ShiftBrightnessOnset<NLEDS> shiftBright;
/*
SpectrumProgram<nLeds> spectrum1;

// effect filters
FlickerBrightness<nLeds> flicker;
ShiftBrightnessOnset<nLeds> shiftBright;
ShiftColorOnset<nLeds> shiftColor;

// other
//Solid<nLeds> solidGlow(0xA02000);
Solid<nLeds> solidGlow(0x150050);
//Solid<nLeds> solidGlow(0x5000C0);
Solid<nLeds> solidBlack(0x000000);

ColorShifter<nLeds> shifter1;
VUMeter<nLeds> vu1;
VUMeter<nLeds/16> vu2;

ProgramRepeater<nLeds> doublevu(nLeds/16, 16);
Lanterns<nLeds> lanterns1(8, 6);
RotateProgram<nLeds> rotate1(nLeds-3);
*/


//------------------------------------------------------------------------------
void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.3);

  ledring.setup();

  profiler.resetStartTime();
  Serial.print("Starting system...");
  Serial.println();

  pinMode(0, INPUT_PULLUP); // Mode switch
  pinMode(A10, INPUT); // Mode switch
  pinMode(COLORPIN, INPUT); // Mode switch

  /*
  // Setup lights
  flicker.connect(&solidGlow);
  //shifter1.connect(&spectrum2);
  vu1.connect(&shifter1);
  //vu2.connect(&spectrum2);
  doublevu.connect(&vu1);
  lanterns1.connect(&flicker, 0);
  lanterns1.connect(&solidBlack, 1);
  rotate1.connect(&lanterns1);
  */
  solid1.init(lightMaster.rand);
  shiftBright.init(lightMaster.rand);
  shiftBright.connect(&solid1);
}

void printColor (unsigned color) {
    int r, g, b;
    unpackColor(color, r, g, b);
    Serial.print("x1: ");
    Serial.print(r);
    Serial.print(", x2: ");
    Serial.print(g);
    Serial.print(", x3: ");
    Serial.print(b);
    Serial.println();
}


//------------------------------------------------------------------------------
void loop() {

  if (myHFC.available()) {
    unsigned windowEnd = myHFC.hfcWindowEnd;
    unsigned windowEndSample = myHFC.hfcWindowEndSample;

    // Copy HFC samples to the extractor
    profiler.call(extractorAddSample);
    myHFC.smoothedHFC.copySamples(extractor.workingMemory, hfcWindow, windowEnd);
    extractor.extractBeat(windowEndSample);
    /*
    Serial.print("HFC samples: ");
    for (int i=0; i<512; ++i) {
      Serial.print(myHFC.smoothedHFC.buffer[i]);
      Serial.print(", ");
    }
    Serial.println();
    */
    profiler.finish(extractorAddSample);
    
    // Add beats to the tracker
    profiler.call(trackerAddHypothesis);
    tracker.addBeatHypothesis(extractor.beat);
    profiler.finish(trackerAddHypothesis);

    Serial.print(brightnessControlBuffer.mean());
    Serial.print(", raw color: ");
    Serial.print(analogRead(COLORPIN));
    Serial.print(", color: ");
    Serial.println(colorControlBuffer.mean());


    /*
    profiler.call(printing);
    Serial.print("state: ");
    Serial.print(tracker.state);
    Serial.println();
    Serial.print("samples per beat: ");
    Serial.print(tracker.hypothesis1.samplesPerBeat);
    Serial.print(" (");
    Serial.print(tracker.hypothesis2.samplesPerBeat);
    Serial.print(")");
    Serial.println();
    Serial.print("tempo: ");
    Serial.print(tracker.tempoGuess());
    Serial.println();
    */
    //profiler.printStats();
    /*
    Serial.print("audio sample: ");
    Serial.print(windowEndSample);
    Serial.print(", HFC sample: ");
    Serial.print(windowEnd);
    Serial.print(", dropped samples: ");
    Serial.print(static_cast<int>(windowEndSample / samplesPerHFC) - static_cast<int>(windowEnd));
    Serial.println();
    Serial.println("========");
    profiler.finish(printing);
    */
  }

  // check switch
  if (digitalRead(0) == HIGH) {
    // Lanterns
    lightMaster.setMode(false);
  } else {
    lightMaster.setMode(true);
  }

  // check pots
  //Serial.println(analogRead(A10));
  //Serial.println(analogRead(A10));
  brightnessControlBuffer.addSample(static_cast<int>(analogRead(A10) >> 2) - 128);
  lightMaster.controls.brightnessAdjustment = brightnessControlBuffer.mean();
  colorControlBuffer.addSample(static_cast<int>(analogRead(COLORPIN)));
  //colorControlBuffer.addSample(static_cast<int>(analogRead(A12)) * 3 / 2);
  lightMaster.controls.colorAdjustment = colorControlBuffer.mean();


  profiler.call(lightsync);
  /*
  if (tracker.state < 2) {
    lastBeat = newBeat;
    tracker.currentPosition(myHFC.sampleNumber, newBeat, beatPos);
    if (newBeat > lastBeat) {
      color += 2;
      if (color >= 8) {
        color = 0;
      }
    }
  } else {
    newBeat = 0;
  }
  */
  /*
  Serial.print("lights for beat ");
  Serial.print(newBeat);
  Serial.print(" : ");
  Serial.print(beatPos);
  Serial.println();
  */

  //ledring.runProgram(&solid1);
  //ledring.runProgram(&flicker);
  //ledring.runProgram(&lanterns1);
  //ledring.runProgram(&rotate1);
  //ledring.runProgram(&spectrum1);
  //ledring.runProgram(&vu1);
  //ledring.runProgram(&doublevu);
  //ledring.runProgram(&shifter1);
  ledring.runProgram(&lightMaster);
  //ledring.runProgram(&shiftBright);

  profiler.finish(lightsync);


  /*
  profiler.finish(lightsync);

  if (myHFC.samplesSinceLastOnset < 5) {
    colorChange(colors[color+1]);
  } else {
    colorChange(colors[color]);
  }
  */
  //profiler.finish(colorchange);
}

