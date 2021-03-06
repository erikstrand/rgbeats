//==============================================================================
// esProfiler.hpp
// Created 2014-06-26
//==============================================================================

#ifndef ESPROFILER
#define ESPROFILER


//------------------------------------------------------------------------------
enum ProfilerFunction {
  analyzefft,
  analyzeother,
  otherfft,
  hfccalculation,
  bufferAddSample,
  extractorAddSample,
  extractorFrequency,
  extractorPhase,
  trackerAddHypothesis,
  colorchange,
  printing,
  lightsync,
  onsetDetection
  }; 

char const* const FunctionNames[] = {
  "analyze fft",
  "analyze other",
  "other fft",
  "hfccalculation",
  "buffer add sample",
  "extractor add sample",
  "extractor frequency",
  "extractor phase",
  "tracker add hypothesis",
  "color change",
  "printing",
  "light syncing",
  "onset detection"
  };
const int pfunctions = 13;

//------------------------------------------------------------------------------
class Profiler {
public:
  // counts[a][0] gives millis in function a, counts[a][1] gives number of calls
  unsigned starttime;
  unsigned counts[pfunctions][2];
  unsigned start[pfunctions];

  Profiler (): starttime(millis()) { memset(this, 0, sizeof(Profiler)); }

  inline void call (ProfilerFunction f) { start[f] = millis(); }
  inline void finish (ProfilerFunction f) {
    unsigned finish = millis();
    counts[f][0] += finish - start[f];
    counts[f][1] ++;
  }
  inline void resetStartTime () { starttime = millis(); }
  inline void printStats () const {
    Serial.println();
    Serial.print("runtime: ");
    Serial.print(millis() - starttime);
    Serial.print(" millis");
    Serial.println();
    for (int i=0; i<pfunctions; ++i) {
      Serial.print(FunctionNames[i]);
      if (counts[i][1] > 0) {
        Serial.print(": ");
        Serial.print(counts[i][0]);
        Serial.print(" millis, ");
        Serial.print(counts[i][1]);
        Serial.print(" calls, ");
        Serial.print(counts[i][0]/((float)counts[i][1]));
        Serial.print(" millis per call");
      } else {
        Serial.print(": 0 calls");
      }
      Serial.println();
    }
  }
};

#endif

