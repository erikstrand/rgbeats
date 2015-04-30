//==============================================================================
// AiffSampler.cpp
// Created 2014-06-06
//==============================================================================

#include "AiffSampler.h"
//#include "FFT.hpp"
//#include "Onset.hpp"
#include <iostream>


//==============================================================================
// AiffSampler Methods
//==============================================================================

//------------------------------------------------------------------------------
void AiffSampler::load (char const* filename) {
  buffer = aiff.load(filename);
}

//------------------------------------------------------------------------------
void AiffSampler::copyChunk (int firstSample, unsigned length, Complex<int16_t>* data) {
  // zero everything, so we don't have to zero im parts individually
  // and we don't have to zero any padding at the end
  Complex<int16_t>::zero(data, length);

  // Figure out where to start
  unsigned i = 0;
  unsigned buffer_i = firstSample;
  if (firstSample < 0) {
    buffer_i = 0;
    i = static_cast<unsigned>(-firstSample);
  }

  // Figure out when to stop
  unsigned stop;
  if (length <= buffer.sizeSamples - firstSample) {
    stop = length;
  } else {
    stop = buffer.sizeSamples - firstSample;
  }

  // Copy the data (we iterate over i and buffer_i)
  for (; i<stop; ++i, ++buffer_i) {
    float temp = 0;
    for (unsigned j=0; j<buffer.format.channels; ++j) {
      temp += buffer.bin(buffer_i, j);
    }
    temp = temp * 32767. / buffer.format.channels + 0.5;
    data[i].re() = temp;
  }

  // If buffer.sizeSamples - firstSample was less than length,
  // there is unfilled data at the end of data. But we already
  // zeroed it so there is nothing left to do.
}

/*
//------------------------------------------------------------------------------
float AiffSampler::calculateChunkHFC (unsigned firstSample, unsigned length, Complex<float>* data) {
  copyChunk(firstSample, length, data);
  fft(data, length, false);
  return calculateHFC(data, length);
}

//------------------------------------------------------------------------------
std::vector<float> AiffSampler::generateHFCSeries (unsigned windowSize, unsigned stepSize, int start) {
  if (windowSize % stepSize != 0) {
    return std::vector<float>();
  }

  // We want the unique integer n such that
  // samples - stepSize <= start + n*stepSize < samples
  // or equivalently,
  // samples - stepSize - start <= n*stepSize < samples - start
  unsigned n = (static_cast<unsigned>(buffer.sizeSamples - start))/stepSize;
  if (start + n*stepSize == buffer.sizeSamples) { --n; }

  // allocate memory
  Complex<float>* window = new Complex<float>[windowSize];
  std::vector<float> hfcs = std::vector<float>(n);

  // loop through windows
  unsigned i = 0;
  int s = start;
  for (; i<n; ++i, s+=stepSize) {
    //std::cout << "Start sample: " << s << "; end sample: " << s + windowSize << '\n';
    hfcs[i] = calculateChunkHFC(s, windowSize, window);
  }
  
  // finish up
  delete[] window;
  return hfcs;
}
*/

//------------------------------------------------------------------------------
/*
int AiffSampler::writeClicks (char* outFileName, std::vector<unsigned> clickSamples) {
  char inFileName[] = "music/fakeblock.aiff";
  int frames = 3675;
  int zframes = 44100;
	int ret = 0;

	long channels = mAiffGetNumberOfChannels(inFileName);
	if (channels < 1) {
		std::cout << "File is unreadable or missing: " << inFileName << '\n';
		exit(-1);
	}
	float **data = mAiffAllocateAudioBuffer(channels, frames);
	float **zero = mAiffAllocateAudioBuffer(channels, zframes);
  for (unsigned i=0; i<zframes; ++i) {
    **zero = 0.;
  }
  //memset(*zero, 0, zframes*sizeof(float));

	// Read from input file
	printf("Reading from input: %s\n", inFileName);
	ret = mAiffReadData(inFileName, data, 0, frames, channels);
	if (ret < 0) {
		std::cout << "! Error reading input file " << inFileName << " - error #" << ret << "\n! Wrong format or filename\n";
		exit(-1);
	}
	std::cout << "Done reading input file " << inFileName << '\n';
	std::cout << "Read " << ret << " frames.\n";

	// Create output file
	long wordlength = mAiffGetWordlength(inFileName);
	float sampleRate = (float)mAiffGetSampleRate(inFileName);
	printf("Creating output (sampleRate = %f, wordlength = %d, channels = %d)\n", sampleRate, wordlength, channels);
	ret = mAiffInitFile(outFileName, sampleRate, wordlength, channels);

	// and write stuff to it
  */
  /*
  for (int i=0; i<20; ++i) {
    ret = mAiffWriteData(outFileName, data, frames, channels);
    printf("...%d written\n", ret);
  }
  return 0;
  */
/*
	printf("Writing to output: %s\n", outFileName);
  unsigned written = 0;
  for (int i=0; i<clickSamples.size(); ++i) {
    //std::cout << "Next sample: " << clickSamples[i] << '\n';

    // write chunks of zeros
    while (written + zframes <= clickSamples[i]) {
      //std::cout << "Writing whole chunk of zeros\n";
      ret = mAiffWriteData(outFileName, zero, zframes, channels);
      //printf("...%d written\n", ret);
      if (ret < 0) {
        std::cout << "Error writing file\n";
        break;
      }
      written += ret>>1;
      //std::cout << "Written: " << written << '\n';
    }

    // write final zeros
    if (clickSamples[i] - written > 0) {
      //std::cout << "Writing partial chunk of zeros: " << clickSamples[i] - written << "\n";
      ret = mAiffWriteData(outFileName, zero, clickSamples[i] - written, channels);
      //printf("...%d written\n", ret);
      if (ret < 0) {
        std::cout << "Error writing file\n";
        break;
      }
      written += ret>>1;
      //std::cout << "Written: " << written << '\n';
    }

    if (i+1 < clickSamples.size() and clickSamples[i+1] - clickSamples[i] < frames) {
      // write partial click
      //std::cout << "Writing partial click\n";
      ret = mAiffWriteData(outFileName, data, clickSamples[i+1] - clickSamples[i], channels);
      //printf("...%d written\n", ret);
      if (ret < 0) {
        std::cout << "Error writing file\n";
        break;
      }
      written += ret>>1;
      //std::cout << "Written: " << written << '\n';
    } else {

      // write click
      //std::cout << "Writing click\n";
      ret = mAiffWriteData(outFileName, data, frames, channels);
      //printf("...%d written\n", ret);
      if (ret < 0) {
        std::cout << "Error writing file\n";
        break;
      }
      written += ret>>1;
      //std::cout << "Written: " << written << '\n';
    }
  }

	mAiffDeallocateAudioBuffer(data, channels);
	mAiffDeallocateAudioBuffer(zero, channels);
  return 0;
}
*/
