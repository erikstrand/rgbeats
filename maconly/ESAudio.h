//==============================================================================
// ESAudioFile.h
// Created 8/31/11.
//==============================================================================

#ifndef ES_AUDIO
#define ES_AUDIO

#include <iostream>


//==============================================================================
// Notes
//==============================================================================

//------------------------------------------------------------------------------
// Basics
/*
 * We call a single monophonic stream of audio a channel. To store its
 * waveform, we take a predetermined number of samples each second of its height.
 * If enough samples are taken each second, when they are fed through a
 * loudspeaker it will reproduce the recorded waveform quite accurately (as far
 * as our ears/brains are concerned).
 *
 * When we have multiple channels, we use the word sample to refer to the heights
 * of all the channels' waveforms at a single time, and the word bin to refer
 * to the height of a single waveform at a single time. Visually, samples are
 * columns and channels are rows in the following diagram (for two channels):
 *
 *           | Sample 0 | Sample 1 | Sample 2 | Sample 3 | Sample 4 |  ...
 * Channel 0 | Bin 0    | Bin 2    | Bin 4    | Bin 6    | Bin 8    |  ...  
 * Channel 1 | Bin 1    | Bin 3    | Bin 5    | Bin 7    | Bin 9    |  ...
 *
 * Bins are stored in memory in a linear array, indexed as above.
 */


//==============================================================================
// Enum AudioStreamCompatibilityResult
//==============================================================================

enum ASCREnum {
   compatible        = 0,  // already good to go
   incompatible      = 1,  // impossible
   possible          = 2,  // not compatible yet, but can become so
   undefined         = 3   // returned when an AudioStreamFormat is undefined
};

struct AudioStreamCompatibilityResult {
   ASCREnum compatibility;
   
   AudioStreamCompatibilityResult(ASCREnum compatibility): compatibility(compatibility) {}
   operator bool () { return ((compatibility == compatible) ? true : false); }
};


//==============================================================================
// Struct AudioStreamFormat
//==============================================================================

//------------------------------------------------------------------------------
// Used to ensure compatibility between AudioStream components.
struct AudioStreamFormat {
   // ToDo: this whole thing can fit in an unsigned
   // (ex: 4 bits c, 21 bits sps, 4 bits bps, 1 bit fp, 1 bit be, 1 bit d)
   unsigned channels;         // == binsPerSample
   unsigned samplesPerSecond;
   unsigned bytesPerBin;
   bool     floatingPoint;    // false -> integer, true -> floating point
   bool     bigEndian;        // false -> little endian, true -> big endian
   bool     defined;          // false -> format not yet defined, true -> good to go

   inline AudioStreamFormat (): defined(false) {}
   inline AudioStreamFormat (unsigned channels, unsigned samplesPerSecond, unsigned bytesPerBin,
                             bool floatingPoint, bool bigEndian);
   inline AudioStreamCompatibilityResult operator== (AudioStreamFormat const& format) const;
};

//------------------------------------------------------------------------------
AudioStreamFormat::AudioStreamFormat (unsigned channels, unsigned samplesPerSecond, unsigned bytesPerBin,
                                      bool floatingPoint, bool bigEndian)
: channels(channels), samplesPerSecond(samplesPerSecond), bytesPerBin(bytesPerBin),
  floatingPoint(floatingPoint), bigEndian(bigEndian), defined(true)
{}

//------------------------------------------------------------------------------
AudioStreamCompatibilityResult AudioStreamFormat::operator== (AudioStreamFormat const& format) const {
   if (!defined or !format.defined)
      return undefined;
   if (channels         == format.channels         and
       samplesPerSecond == format.samplesPerSecond and
       bytesPerBin      == format.bytesPerBin      and
       floatingPoint    == format.floatingPoint    and
       bigEndian        == format.bigEndian)
      return compatible;
   return incompatible;
}


//==============================================================================
// The Global Default AudioStreamFormat Object
//==============================================================================

extern AudioStreamFormat defaultFormat;


//==============================================================================
// Struct SampleBuffer
//==============================================================================

//------------------------------------------------------------------------------
/* Completely general buffer of samples. Can be used to represent audio data
 * with any AudioStreamFormat (ie any number of channels, samplesPerSecond,
 * and bytesPerBin, as well as integer/floating point and little/big endian bins).
 *
 * SampleBuffers do not own their data.
 *
 * For a serious DAW, 32 bit little endian integer samples should be compiled in.
 */
struct SampleBuffer {
   void* data;
   unsigned sizeSamples;
   AudioStreamFormat format;
   
   inline SampleBuffer (): sizeSamples(0) {}
   inline SampleBuffer (void* data, unsigned sizeSamples, AudioStreamFormat format);
   inline SampleBuffer (SampleBuffer const& buffer);
   inline void* bin (unsigned sample, unsigned channel);
};

//------------------------------------------------------------------------------
SampleBuffer::SampleBuffer (void* data, unsigned sizeSamples, AudioStreamFormat format)
: data(data), sizeSamples(sizeSamples), format(format)
{}

//------------------------------------------------------------------------------
SampleBuffer::SampleBuffer (SampleBuffer const& buffer)
: data(buffer.data), sizeSamples(buffer.sizeSamples), format(buffer.format)
{}

//------------------------------------------------------------------------------
void* SampleBuffer::bin (unsigned sample, unsigned channel) {
   char* byte = static_cast<char*> (data);
   return static_cast<void*> (&byte[sample*format.channels*format.bytesPerBin + channel*format.bytesPerBin]);
}


//==============================================================================
// SampleBuffer Specializations
//==============================================================================

// Why do I need these?

//------------------------------------------------------------------------------
struct SampleBufferI : public SampleBuffer {
   SampleBufferI () {}
   SampleBufferI (SampleBuffer buffer): SampleBuffer(buffer) {
      if (buffer.format.floatingPoint == true         or
          buffer.format.bytesPerBin   != sizeof(int))
      {
         std::cout << "Hnnngghh! Invalid conversion to SampleBufferI.\n";
         exit(1);
      }
   }

   int& bin (unsigned sample, unsigned channel) {
      return *static_cast<int*> (SampleBuffer::bin(sample, channel));
   }
};

//------------------------------------------------------------------------------
struct SampleBufferF : public SampleBuffer {
   SampleBufferF () {}
   SampleBufferF (SampleBuffer buffer): SampleBuffer(buffer) {
      if (buffer.format.floatingPoint == false          or
          buffer.format.bytesPerBin   != sizeof(float))
      {
         std::cout << "Hnnngghh! Invalid conversion to SampleBufferF.\n";
         exit(1);
      }
   }

   float& bin (unsigned sample, unsigned channel) {
      return *static_cast<float*> (SampleBuffer::bin(sample, channel));
   }
};


//==============================================================================
// Virtual Class AudioStreamSource
//==============================================================================

//------------------------------------------------------------------------------
// Anything that outputs audio data. 
class AudioStreamSource {
protected:
   AudioStreamFormat _format;    // format output by the AudioStreamSource

public:
   inline AudioStreamSource () : _format() {}
   inline AudioStreamSource (AudioStreamFormat format) : _format(format) {}
   virtual ~AudioStreamSource () {};
   
   inline AudioStreamFormat format () const { return _format; }
   virtual bool setFormat (AudioStreamFormat format) = 0;

   virtual unsigned render (SampleBuffer sampleBuffer) = 0;
   virtual void seekabs (unsigned sampleNumber) = 0;
   virtual void seekrel (int sampleNumber) = 0;
};


//==============================================================================
// Virtual Class AudioStreamNode
//==============================================================================

//------------------------------------------------------------------------------
// An AudioStreamSource that pulls audio from another AudioStreamSource.
class AudioStreamNode : public AudioStreamSource {
protected:
   AudioStreamSource* _input;

public:
   inline AudioStreamNode (): AudioStreamSource(), _input(0) {}
   inline AudioStreamNode (AudioStreamFormat format): AudioStreamSource(format), _input(0) {}

   virtual inline AudioStreamFormat inputFormat () const { return _format; }
   inline bool connect (AudioStreamSource* input);

   void seekabs (unsigned sampleNumber) { if (_input) _input->seekabs(sampleNumber); }
   void seekrel (int nSamples)     { if (_input) _input->seekrel(nSamples); }
};

//------------------------------------------------------------------------------
bool AudioStreamNode::connect (AudioStreamSource* input) {
   if (this->inputFormat() == input->format()) {
      _input = input;
      return true;
   }
   std::cout << "Can't make connection.\n";
   return false;
}


//==============================================================================
// Struct AudioPlayer
//==============================================================================

struct AudioPlayer : AudioStreamSource {
public:
   SampleBufferF _data;
   unsigned _currentSample;

public:
   AudioPlayer (): _currentSample(0) {}
   AudioPlayer (SampleBufferF buffer): AudioStreamSource(buffer.format), _data(buffer), _currentSample(0) {}

   inline AudioPlayer& operator= (SampleBufferF const& buf);
   inline bool setFormat (AudioStreamFormat format) { return _format == format; }

   inline unsigned render (SampleBuffer sampleBuffer);

   void seekabs (unsigned sampleNumber) { _currentSample = sampleNumber; }
   void seekrel (int sampleNumber) { _currentSample += sampleNumber; }
};

//------------------------------------------------------------------------------
AudioPlayer& AudioPlayer::operator= (SampleBufferF const& buf) {
   _format = buf.format;
   _data = buf;
   _currentSample = 0;
   return *this;
}

//------------------------------------------------------------------------------
unsigned AudioPlayer::render (SampleBuffer sampleBuffer) {
//   std::cout << "AudioPlayer is rendering...\n";
   SampleBufferF buffer(sampleBuffer);
   unsigned copiedSamples = _data.sizeSamples - _currentSample;
   if (copiedSamples > buffer.sizeSamples) copiedSamples = buffer.sizeSamples;
   memcpy(&buffer.bin(0, 0), &_data.bin(_currentSample, 0), copiedSamples * _format.channels * _format.bytesPerBin);
   _currentSample += copiedSamples;
//   std::cout << "AudioPlayer is done rendering.\n";
   return copiedSamples;
}

#endif // ES_AUDIO
