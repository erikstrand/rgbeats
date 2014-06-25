#include <Audio.h>
#include <Wire.h>
#include <SD.h>

#include "RingBuffer.h"
#include "esOctoWS2811.h"
#include "es_analyze_fft1024.h"
#include "BeatExtractor.h"
#include "BeatTracker.h"

//------------------------------------------------------------------------------
// Audio constants
const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioInputI2S       audioInput;         // audio shield: mic or line-in
EsAudioAnalyzeFFT1024  myFFT(20);
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components
//
AudioConnection c1(audioInput, 0, audioOutput, 0);
AudioConnection c2(audioInput, 0, myFFT, 0);
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
#define RED2   0x080000
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
  for (int i=0; i<leds.numPixels(); ++i) {
    leds.setPixel(i, color);
  }
  leds.show();
}


//------------------------------------------------------------------------------
//RingBufferWithMedian<int, 64> hfcBuffer;
const unsigned samplesPerHFC = 512;
const unsigned HFCPerBeatHypothesis = 256;
BeatExtractor<float, 16, 512, HFCPerBeatHypothesis, samplesPerHFC> extractor;
BeatTracker<512, samplesPerHFC, HFCPerBeatHypothesis> tracker;
unsigned currentBeat = 0;
unsigned triggerSample = 0;


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
}

//------------------------------------------------------------------------------
void loop() {

  if (myFFT.available()) {
    // each time new FFT data is available
    // print it all to the Arduino Serial Monitor
    /*
    Serial.print("FFT: ");
    for (int i=0; i<1024; i++) {
      Serial.print(myFFT.output[i]);
      Serial.print(",");
    }
    Serial.println();
    */
    
    // Calculate HFC
    unsigned hfc = 0;
    for (unsigned i=1; i<1024; ++i) {
      hfc += i*myFFT.output[i];
      if (hfc > 0x80000000) {
         Serial.print("overflow may occur!");
      }
    }

    // Add HFC sample to the extractor
    if (extractor.addSample((float)hfc)) {
      tracker.addBeatHypothesis(extractor.beat);
      Serial.print("state: ");
      Serial.print(tracker.state);
      Serial.println();
      Serial.print("samples per beat: ");
      Serial.print(tracker.currentHypothesis.samplesPerBeat);
      Serial.println();
      Serial.print("tempo: ");
      Serial.print(tracker.tempoGuess());
      Serial.println();
    }

    if (tracker.state < 2) {
      if (tracker.firstFinal + tracker.nPredictions < currentBeat) {
        // beat chain was broken; find the next beat in the new chain
        currentBeat = tracker.firstFinal;
        triggerSample = tracker.beatLocation(currentBeat);
        unsigned i=0;
        while (i < tracker.nPredictions and triggerSample <= myFFT.sampleNumber) {
          ++currentBeat;
          triggerSample = tracker.beatLocation(currentBeat);
          ++i;
        }
      } else {
        if (myFFT.sampleNumber >= triggerSample) {
          color += 2;
          if (color >= 8) {
            color = 0;
          }
          ++currentBeat;
          triggerSample = tracker.beatLocation(currentBeat);
          while (currentBeat < tracker.firstFinal + tracker.nPredictions and triggerSample <= myFFT.sampleNumber) {
            ++currentBeat;
            triggerSample = tracker.beatLocation(currentBeat);
          }
        }
      }
    }

    if (extractor.rawHFC.newestSample() > extractor.rawHFC.median() + (extractor.rawHFC.stddeviation() * 0.5)) {
      colorChange(colors[color+1]);
    } else {
      colorChange(colors[color]);
    }
    //Serial.print(hfc);
    //Serial.println();
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

