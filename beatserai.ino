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
const int ledsPerStrip = 240;
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
void colorChange (int color) {
  for (int i=0; i<leds.numPixels(); ++i) {
    leds.setPixel(i, color);
  }
  leds.show();
}

//------------------------------------------------------------------------------
void showColorScale (OctoWS2811& leds, int (*cs)(int), unsigned number = ledsPerStrip) {
  for (unsigned i=0; i<number; ++i) {
    int color = (*cs)(i*ledsPerStrip/255);
    leds.setPixel(i, color);
    leds.setPixel(i+ledsPerStrip, color);
    leds.setPixel(i+2*ledsPerStrip, color);
  }
  for (unsigned i=number; i<ledsPerStrip; ++i) {
    leds.setPixel(i, 0x000000);
    leds.setPixel(i+ledsPerStrip, 0x000000);
    leds.setPixel(i+2*ledsPerStrip, 0x000000);
  }
  leds.show();
}

//------------------------------------------------------------------------------
void lanterns () {
  static unsigned lanternYellow = 0xFF6000;
  for (unsigned i=0; i<ledsPerStrip; ++i) {
    leds.setPixel(i, 0x000000);
    leds.setPixel(i+ledsPerStrip, 0x000000);
    leds.setPixel(i+2*ledsPerStrip, 0x000000);
  }
  for (unsigned i=0; i<ledsPerStrip; i+=ledsPerStrip/8) {
    for (unsigned j=i-3; j<ledsPerStrip and j<i+3; ++j) {
      leds.setPixel(j, lanternYellow);
      //leds.setPixel(j+ledsPerStrip, lanternYellow);
      //leds.setPixel(j+2*ledsPerStrip, lanternYellow);
    }
  }
  leds.show();
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
CubeHelixScale cubehelixScale;

//------------------------------------------------------------------------------
Spectrum spectrumProgram;
ColorScaleProgram cubehelixProgram(&cubehelixScale);
//VUMeter vumeterProgram(&cubehelixProgram);
VUMeter vumeterProgram(&spectrumProgram);
ProgramRepeater doublevuProgram(2, &vumeterProgram);


//------------------------------------------------------------------------------
void runProgram (LightProgram* program) {
  unsigned value, beat, beatpos;
  tracker.currentPosition(myHFC.sampleNumber, beat, beatpos);
  for (unsigned i=0; i<ledsPerStrip; ++i) {
    value = program->pixel(i, beat, beatpos, (unsigned)myHFC.rawHFC.mean(), myHFC.output);
    leds.setPixel(i, value);
    leds.setPixel(i+ledsPerStrip, value);
    leds.setPixel(i+2*ledsPerStrip, value);
  }
  leds.show();
}


//------------------------------------------------------------------------------
void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.7);

  leds.begin();
  leds.show();

  profiler.resetStartTime();
  Serial.print("Starting system...");
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

    // Print stuff
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
    profiler.printStats();
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

  /*profiler.call(lightsync);
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

  profiler.call(lightsync);
  //runProgram(reinterpret_cast<LightProgram*>(&spectrumProgram));
  //runProgram(&spectrumProgram);
  //runProgram(&vumeterProgram);
  runProgram(&doublevuProgram);
  //runProgram(&cubehelixProgram);
  profiler.finish(lightsync);


  /*
  profiler.finish(lightsync);

  if (myHFC.samplesSinceLastOnset < 5) {
    colorChange(colors[color+1]);
  } else {
    colorChange(colors[color]);
  }
  */
  //profiler.call(colorchange);
  //spectroColors(myHFC.output);
  //vumeter.draw(leds);
  //colorChange(0x212504);
  //showColorScale(leds, &cscaleCubeHelix);
  //lanterns();
  //profiler.finish(colorchange);
}

