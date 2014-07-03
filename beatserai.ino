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

const unsigned cubehelix[] = {
0x000000, 0x000000, 0x000000, 0x010000, 0x010000, 0x020001, 0x030001, 0x030001, 0x030101, 0x030101, 0x030101, 0x040102, 0x040102, 0x040102, 0x050103, 0x060103, 0x060104, 0x060204, 0x070204, 0x070204, 0x070205, 0x070205, 0x070205, 0x080206, 0x080206, 0x090207, 0x090207, 0x090308, 0x090308, 0x0a0309, 0x0a0309, 0x0a0309,
0x0a030a, 0x0a030a, 0x0b040b, 0x0b040b, 0x0b040c, 0x0b040c, 0x0b040d, 0x0b040d, 0x0c050e, 0x0c050e, 0x0c050f, 0x0c0510, 0x0c0610, 0x0c0610, 0x0c0611, 0x0c0612, 0x0c0712, 0x0c0712, 0x0c0713, 0x0c0714, 0x0c0814, 0x0c0814, 0x0c0815, 0x0c0815, 0x0c0916, 0x0b0917, 0x0b0917, 0x0b0a17, 0x0b0a18, 0x0b0b18, 0x0b0b18, 0x0b0b19,
0x0a0c19, 0x0a0c1a, 0x0a0c1a, 0x0a0c1a, 0x0a0d1a, 0x0a0d1a, 0x0a0d1a, 0x090e1b, 0x090f1b, 0x090f1c, 0x090f1c, 0x08101c, 0x08101c, 0x08111c, 0x07111c, 0x07111c, 0x07121c, 0x07121c, 0x07131c, 0x06141d, 0x06141c, 0x06141c, 0x06141c, 0x06151c, 0x06151c, 0x05161c, 0x05161c, 0x05171c, 0x05181c, 0x04181c, 0x04181c, 0x04181c,
0x04191b, 0x04191b, 0x041a1b, 0x041a1b, 0x041b1a, 0x041c1a, 0x041c1a, 0x041c19, 0x031d19, 0x031d18, 0x031e18, 0x041e18, 0x041e17, 0x041f17, 0x041f16, 0x041f16, 0x042016, 0x042015, 0x042115, 0x042114, 0x052114, 0x052114, 0x052213, 0x052212, 0x062312, 0x062312, 0x062311, 0x062310, 0x072410, 0x07240f, 0x08240f, 0x08240f,
0x08240e, 0x08240e, 0x09250d, 0x09250d, 0x0a250c, 0x0b250c, 0x0c250b, 0x0c260b, 0x0c260b, 0x0d260a, 0x0e2609, 0x0f2609, 0x0f2608, 0x102608, 0x112608, 0x112608, 0x122607, 0x132607, 0x142606, 0x152606, 0x152606, 0x162606, 0x172605, 0x182605, 0x192605, 0x1a2605, 0x1b2605, 0x1c2604, 0x1d2504, 0x1e2504, 0x1f2504, 0x202504,
0x212504, 0x222504, 0x232504, 0x242404, 0x252404, 0x262404, 0x272405, 0x282405, 0x292305, 0x2a2305, 0x2b2305, 0x2c2306, 0x2d2206, 0x2e2206, 0x2f2207, 0x302207, 0x312108, 0x322108, 0x332109, 0x342109, 0x35200a, 0x36200a, 0x36200b, 0x37200c, 0x381f0c, 0x391f0d, 0x3a1f0e, 0x3b1f0e, 0x3c1e0f, 0x3d1e10, 0x3e1e11, 0x3e1e12,
0x3f1d13, 0x401d14, 0x411d15, 0x411d16, 0x421c17, 0x431c18, 0x441c19, 0x441c1a, 0x451c1b, 0x451c1c, 0x461b1d, 0x471b1e, 0x471b1f, 0x481b20, 0x481b22, 0x481b23, 0x491b24, 0x491b25, 0x4a1a27, 0x4a1a28, 0x4a1a29, 0x4a1a2a, 0x4b1a2c, 0x4b1a2d, 0x4b1a2e, 0x4b1a30, 0x4b1a31, 0x4b1a32, 0x4b1b33, 0x4b1b35, 0x4b1b36, 0x4b1b37,
0x4b1b39, 0x4b1b3a, 0x4b1b3b, 0x4b1c3d, 0x4a1c3e, 0x4a1c3f, 0x4a1c40, 0x4a1c42, 0x491d43, 0x491d44, 0x491d45, 0x481e47, 0x481e48, 0x471e49, 0x471f4a, 0x461f4b, 0x46204c, 0x45204d, 0x44214e, 0x44214f, 0x432150, 0x422251, 0x422252, 0x412353, 0x402454, 0x3f2455, 0x3f2556, 0x3e2557, 0x3e2557, 0x3e2557, 0x3e2557, 0x3e2557,
0x3e2557, 0x3e2557, 0x3e2557, 0x3d2658 
};

const int cubehelix2[] = {
0x000000,
0x000000,
0x000000,
0x000000,
0x000000,
0x000000,
0x000001,
0x000001,
0x000001,
0x000001,
0x000001,
0x000002,
0x000102,
0x010102,
0x010102,
0x010102,
0x010103,
0x010103,
0x010103,
0x010103,
0x010103,
0x010204,
0x010204,
0x010204,
0x010204,
0x010204,
0x010204,
0x010205,
0x010305,
0x010305,
0x010305,
0x010305,
0x010305,
0x010305,
0x010306,
0x010406,
0x010406,
0x010406,
0x010406,
0x010406,
0x010406,
0x010406,
0x010506,
0x010507,
0x010507,
0x010507,
0x010507,
0x010507,
0x010607,
0x010607,
0x010607,
0x010607,
0x010607,
0x010707,
0x000707,
0x000707,
0x000707,
0x000707,
0x000707,
0x000807,
0x000807,
0x000807,
0x000807,
0x000807,
0x000807,
0x000907,
0x000907,
0x000907,
0x000907,
0x000907,
0x000907,
0x000a07,
0x000a07,
0x000a07,
0x010a07,
0x010a07,
0x010a07,
0x010b06,
0x010b06,
0x010b06,
0x010b06,
0x010b06,
0x010b06,
0x010c06,
0x010c06,
0x010c06,
0x010c06,
0x010c05,
0x010c05,
0x020c05,
0x020d05,
0x020d05,
0x020d05,
0x020d05,
0x020d05,
0x020d04,
0x020d04,
0x030d04,
0x030e04,
0x030e04,
0x030e04,
0x030e04,
0x030e04,
0x040e03,
0x040e03,
0x040e03,
0x040e03,
0x040e03,
0x050f03,
0x050f03,
0x050f02,
0x050f02,
0x060f02,
0x060f02,
0x060f02,
0x060f02,
0x070f02,
0x070f02,
0x070f02,
0x070f01,
0x080f01,
0x080f01,
0x080f01,
0x090f01,
0x090f01,
0x090f01,
0x0a0f01,
0x0a0f01,
0x0a0f01,
0x0b0f01,
0x0b0f01,
0x0b0f01,
0x0c0f01,
0x0c0f00,
0x0c0f00,
0x0d0f00,
0x0d0f00,
0x0e0f00,
0x0e0f00,
0x0e0f00,
0x0f0f00,
0x0f0f00,
0x0f0f00,
0x100f01,
0x100f01,
0x110f01,
0x110f01,
0x110f01,
0x120f01,
0x120f01,
0x130f01,
0x130f01,
0x140f01,
0x140f01,
0x140f02,
0x150e02,
0x150e02,
0x160e02,
0x160e02,
0x160e02,
0x170e03,
0x170e03,
0x180e03,
0x180e03,
0x180e03,
0x190e04,
0x190e04,
0x1a0e04,
0x1a0d04,
0x1a0d05,
0x1b0d05,
0x1b0d05,
0x1c0d06,
0x1c0d06,
0x1c0d06,
0x1d0d06,
0x1d0d07,
0x1d0d07,
0x1e0d08,
0x1e0d08,
0x1e0d08,
0x1f0c09,
0x1f0c09,
0x1f0c09,
0x200c0a,
0x200c0a,
0x200c0b,
0x210c0b,
0x210c0c,
0x210c0c,
0x220c0c,
0x220c0d,
0x220c0d,
0x220c0e,
0x230c0e,
0x230c0f,
0x230c0f,
0x230c10,
0x230c10,
0x240b11,
0x240b11,
0x240b12,
0x240b12,
0x240b13,
0x250b13,
0x250b14,
0x250b15,
0x250b15,
0x250b16,
0x250b16,
0x250b17,
0x250b17,
0x250b18,
0x250c18,
0x250c19,
0x260c1a,
0x260c1a,
0x260c1b,
0x260c1b,
0x260c1c,
0x260c1c,
0x260c1d,
0x260c1e,
0x250c1e,
0x250c1f,
0x250c1f,
0x250c20,
0x250d20,
0x250d21,
0x250d21,
0x250d22,
0x250d23,
0x250d23,
0x240d24,
0x240d24,
0x240e25,
0x240e25,
0x240e26,
0x240e26,
0x230e27,
0x230e27,
0x230f28,
0x230f28,
0x220f29,
0x220f29,
0x22102a,
0x22102a,
0x21102b,
0x21102b,
0x21102c,
0x20112c,
0x20112c,
0x20112d,
0x1f112d,
0x1f122e,
0x1f122e
};

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
void spectroColors (volatile uint16_t const* spectrum) {
  for (unsigned i=0; i<ledsPerStrip; ++i) {
    unsigned bin1 = 8 + 2*i;
    unsigned power = 16*(spectrum[bin1] + spectrum[bin1+1]);
    power &= 0xFF; // make sure we saturate at 255
    int value = cubehelix2[power];
    leds.setPixel(i, value);
    leds.setPixel(i+ledsPerStrip, value);
    leds.setPixel(i+2*ledsPerStrip, value);
    //leds.setPixel(i, cubehelix[i]);
    //leds.setPixel(i+ledsPerStrip, cubehelix[i]);
    //leds.setPixel(i+2*ledsPerStrip, cubehelix[i]);
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
class VUMeter {
public:
  unsigned lastPrint;
  static const int scale = 3000000;
  VUMeter (): lastPrint(0) {}
  void draw (OctoWS2811& leds);
};
void VUMeter::draw (OctoWS2811& leds) {
  unsigned hfcaverage = static_cast<unsigned>(myHFC.rawHFC.mean());
  hfcaverage = hfcaverage * ledsPerStrip / scale;
  if (hfcaverage >= ledsPerStrip) { hfcaverage = ledsPerStrip - 1; }
  if (lastPrint == 1000) {
    lastPrint = 0;
    Serial.println(hfcaverage);
  }
  showColorScale(leds, &cscaleCubeHelix, hfcaverage);
  ++lastPrint;
}
VUMeter vumeter;

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

  profiler.call(lightsync);
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
  /*
  Serial.print("lights for beat ");
  Serial.print(newBeat);
  Serial.print(" : ");
  Serial.print(beatPos);
  Serial.println();
  */
  profiler.finish(lightsync);

  /*
  if (myHFC.samplesSinceLastOnset < 5) {
    colorChange(colors[color+1]);
  } else {
    colorChange(colors[color]);
  }
  */
  profiler.call(colorchange);
  //spectroColors(myHFC.output);
  vumeter.draw(leds);
  //colorChange(0x212504);
  //showColorScale(leds, &cscaleCubeHelix);
  //lanterns();
  profiler.finish(colorchange);
}

