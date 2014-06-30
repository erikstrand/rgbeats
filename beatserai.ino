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

#define RED    0x030000
#define RED2   0x0B0000
#define GREEN  0x000400
#define GREEN2 0x000800
#define BLUE   0x000004
#define BLUE2  0x000008
#define YELLOW 0x020300
#define YELLOW2 0x040600
#define PINK   0x020002
#define PINK2  0x040004
#define ORANGE 0x030100
#define ORANGE2 0x040200
#define WHITE  0x020202

unsigned long t1 = 0;
int color = 0;
int colors[] = {RED, RED2, ORANGE, ORANGE2, GREEN, GREEN2, BLUE, BLUE2, PINK, PINK2, WHITE};


void colorChange(int color) {
  profiler.call(colorchange);
  for (int i=0; i<leds.numPixels(); ++i) {
    leds.setPixel(i, color);
  }
  leds.show();
  profiler.finish(colorchange);
}


//------------------------------------------------------------------------------
//RingBufferWithMedian<int, 64> hfcBuffer;
const unsigned hfcWindow = 512;
const unsigned samplesPerHFC = 512;
const unsigned HFCPerBeatHypothesis = 128;
BeatExtractor<hfcWindow, samplesPerHFC> extractor;
BeatTracker<hfcWindow, samplesPerHFC, HFCPerBeatHypothesis> tracker;
unsigned currentBeat = 0;
unsigned triggerSample = 0;
//Complex<int16_t> testbuffer[512];


//------------------------------------------------------------------------------
void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.6);

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
    Serial.print("HFC samples: ");
    for (int i=0; i<512; ++i) {
      Serial.print(myHFC.smoothedHFC.buffer[i]);
      Serial.print(", ");
    }
    Serial.println();
    profiler.finish(extractorAddSample);
    
    // Add beats to the tracker
    profiler.call(trackerAddHypothesis);
    tracker.addBeatHypothesis(extractor.beat);
    profiler.finish(trackerAddHypothesis);

    // Print stuff
    profiler.call(printing);
    Serial.print("state: ");
    Serial.print(tracker.state);
    Serial.println();
    Serial.print("samples per beat: ");
    Serial.print(tracker.currentHypothesis.samplesPerBeat);
    Serial.println();
    Serial.print("tempo: ");
    Serial.print(tracker.tempoGuess());
    Serial.println();
    profiler.printStats();
    Serial.print("audio sample: ");
    Serial.print(windowEndSample);
    Serial.print(", HFC sample: ");
    Serial.print(windowEnd);
    Serial.print(", dropped samples: ");
    Serial.print(static_cast<int>(windowEndSample / samplesPerHFC) - static_cast<int>(windowEnd));
    Serial.println();
    profiler.finish(printing);

    profiler.call(lightsync);
    if (tracker.state < 2) {
      if (tracker.firstFinal + tracker.nPredictions < currentBeat) {
        // beat chain was broken; find the next beat in the new chain
        currentBeat = tracker.firstFinal;
        triggerSample = tracker.beatLocation(currentBeat);
        unsigned i=0;
        while (i < tracker.nPredictions and triggerSample <= myHFC.sampleNumber) {
          ++currentBeat;
          triggerSample = tracker.beatLocation(currentBeat);
          ++i;
        }
      } else {
        if (myHFC.sampleNumber >= triggerSample) {
          color += 2;
          if (color >= 8) {
            color = 0;
          }
          ++currentBeat;
          triggerSample = tracker.beatLocation(currentBeat);
          while (currentBeat < tracker.firstFinal + tracker.nPredictions and triggerSample <= myHFC.sampleNumber) {
            ++currentBeat;
            triggerSample = tracker.beatLocation(currentBeat);
          }
        }
      }
    }
    profiler.finish(lightsync);

    //Serial.print(hfc);
    //Serial.println();
  }

  if (myHFC.samplesSinceLastOnset < 5) {
    colorChange(colors[color+1]);
  } else {
    colorChange(colors[color]);
  }

  //int microsec = 2000000 / leds.numPixels();  // change them all in 2 seconds

  // uncomment for voltage controlled speed
  // millisec = analogRead(A9) / 40;

/*
  colorWipe(RED, microsec);
  colorWipe(GREEN, microsec);
  colorWipe(BLUE, microsec);
  colorWipe(YELLOW, microsec);
  colorWipe(PINK, microsec);
  colorWipe(ORANGE, microsec);
  colorWipe(WHITE, microsec);
*/
/*
  if (millis() > t1 + 1000) {
    t1 = millis();
    ++color;
    if (color >= 7) color = 0;
    colorChange(colors[color]);
  }
  */
}

void colorWipe(int color, int wait)
{
  for (int i=0; i < leds.numPixels(); i++) {
    leds.setPixel(i, color);
    leds.show();
    delayMicroseconds(wait);
  }
}

