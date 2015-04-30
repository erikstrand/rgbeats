//==============================================================================
// AiffReader.cpp
// Created 8/31/11.
//==============================================================================

#include "AiffReader.h"
#include <iostream>
#include <fstream>
// This has way more than we need; we just want AIFF.h
#include <CoreServices/CoreServices.h>
//#include "estdlib.h"

using namespace std;


//==============================================================================
// Member Function Definitions
//==============================================================================

//------------------------------------------------------------------------------
// ToDo: replace ifstream with simple filebuf
// ToDo: try to determine endianness of file
SampleBufferF AiffReader::load (char const* filename) {
   _state = OK;
   cout << "Loading " << filename << '\n';
   ifstream file(filename, ifstream::in | ifstream::binary);
   if (!file.good()) {
      cout << "Couldn't open the file.\n";
      return SampleBufferF();
   }

   char chunkID[5];
   UInt32 chunkSize = 0;
   
   // Read FORM chunk
   file.get(chunkID, 5);
   file.read(reinterpret_cast<char*> (&chunkSize), 4);
   // Apparently there are small endian aiffs out there... we'll see when this crashes.
   swapEndianness(chunkSize);
   cout << "ID: " << chunkID << '\n';
   cout << "Size: " << chunkSize << '\n';
   file.get(chunkID, 5);
   cout << "ID: " << chunkID << '\n';
   if ( strcmp(chunkID, "AIFF")!=0 and strcmp(chunkID, "AIFC")!=0 ) {
      cout << "This is not an aiff file.\n";
      _state = NotAiff;
      return SampleBufferF();
   }

   // Find COMM chunk, extract sample rate etc.
   chunkSize = 0;
   if (!findChunk(file, "COMM", chunkSize)) {
      cout << "Something went wrong.\n";
      _state = CorruptFile;
      return SampleBufferF();
   }
   // things are easier if we copy the whole chunk (header included)
   chunkSize += sizeof(ChunkHeader);
   file.seekg(-sizeof(ChunkHeader), ios_base::cur);
   cout << "Total COMM chunk size: " << chunkSize << '\n';
   char* commonChunkData = new char[chunkSize];
   CommonChunk* commonChunk = reinterpret_cast<CommonChunk*> (commonChunkData);
   file.read(commonChunkData, chunkSize);
   swapEndianness(commonChunk->ckID);
   swapEndianness(commonChunk->ckSize);
   swapEndianness(commonChunk->numChannels);
   swapEndianness(commonChunk->numSampleFrames);
   swapEndianness(commonChunk->sampleSize);
//   swapEndianness(commonChunk->sampleRate);
   
   cout << "Float Test\n";
   char* testbytes;
   char testmask;
   float testnum = 1;
   testbytes = reinterpret_cast<char*> (&testnum);
   for (unsigned i=0; i<4; ++i) {
      testmask = (0x1);
      for (unsigned j=0; j<8; ++j) {
         cout << ((testbytes[i] & testmask) ? 1 : 0);
         testmask <<= 1;
      }
      cout << ' ';
   }
   cout << '\n';
   
   
   // Extract the sample rate (from the extended80 format)
   testbytes = reinterpret_cast<char*> (&commonChunk->sampleRate);
   cout << "sampleRate...\n";
   cout << (0x1<<6) << " is shifted 6 and shifted 7 is " << (0x1<<7) << '\n';
   for (unsigned i=0; i<10; ++i) {
      testmask = (0x1);
      for (unsigned j=0; j<8; ++j) {
         cout << ((testbytes[i] & testmask) ? 1 : 0);
         testmask <<= 1;
      }
      cout << ' ';
   }
   cout << '\n';
   
   // Debug
   cout << "ID:            " << commonChunk->ckID << '\n';
   cout << "size:          " << commonChunk->ckSize << '\n';
   cout << "channels:      " << commonChunk->numChannels << '\n';
   cout << "sample frames: " << commonChunk->numSampleFrames << '\n';
   cout << "sample size:   " << commonChunk->sampleSize << '\n';
   cout << "I'm assuming the sample rate is 44100...\n";
   //   cout << "sample rate:   " << (Float64)swapEndianness(commonChunk->sampleRate) << '\n';

   if (commonChunk->numChannels != 2) {
      cout << "Only dealing with stereo for now.\n";
      return SampleBufferF();
   }
   if (commonChunk->sampleSize != 16) {
      cout << "Only dealing with 16 bit audio for now.\n";
      return SampleBufferF();
   }
   //   if (commonChunk->sampleSize != (extended80)44100.0) {
//      cout << "Only dealing with audio sampled at 44100 hz for now.\n";
//      return AudioPointer(0, 0);
//   }

   
   // Find SSND chunk
   if (!findChunk(file, "SSND", chunkSize)) {
      cout << "Something went wrong.\n";
      _state = CorruptFile;
      return SampleBufferF();
   }
   
   // Extract Sound
   UInt32 offset;
   UInt32 blockSize;
   file.read(reinterpret_cast<char*> (&offset), 4);
   file.read(reinterpret_cast<char*> (&blockSize), 4);
   swapEndianness(offset);
   swapEndianness(blockSize);
   cout << "Offset: " << offset << '\n';
   cout << "Block Size: " << blockSize << '\n';
   chunkSize -= 8; // don't need the offset and blockSize
   if (chunkSize != commonChunk->numSampleFrames * commonChunk->numChannels * (commonChunk->sampleSize>>3)) {
      cout << "The file is corrupt.\n";
      _state = CorruptFile;
      return SampleBufferF();
   }
   char* data = static_cast<char*> (malloc(chunkSize));
   file.read(data, chunkSize);

   // switch endianness of everything
   SInt16* samples16 = reinterpret_cast<SInt16*> (data);
   for (unsigned i=0; i<commonChunk->numSampleFrames * 2; ++i) {
      swapEndianness(samples16[i]);
   }

   AudioStreamFormat outFormat(2, 44100, 4, true, false);
   // 2 channels, 4 bytes per bin
   char* outData = static_cast<char*> (malloc(commonChunk->numSampleFrames * 2 * 4));
   SampleBuffer outBuffer(outData, commonChunk->numSampleFrames, outFormat);
   SampleBufferF outBufferF(outBuffer);
   unsigned i = 0;
   for (unsigned s=0; s<outBufferF.sizeSamples; ++s) {
      for (unsigned b=0; b<outFormat.channels; ++b) {
         outBufferF.bin(s, b) = (float)(samples16[i]) / 32392.0;
         ++i;
      }
   }
   

   // FUCKING SHIT UP FOR TESTING PURPOSES
   
   // switch endianness of everything again
//   for (unsigned s=0; s<outBuffer.sizeSamples; ++s) {
//      for (unsigned b=0; b<outBuffer.format.channels; ++b) {
//         swapEndianness(static_cast<char*>(outBuffer.bin(s, b)), outBuffer.format.bytesPerBin);
//      }
//   }
//   outBufferF.format.bigEndian = true;
   
   
   
   // cleanup
   delete[] commonChunkData;
   free(data);

//   for (unsigned i=0; i<commonChunk->numSampleFrames; i=i+440) {
//      cout << "Sample " << i << ": " << samples[i].left << " --- " << samples[i].right << '\n';
//   }
   
   return outBufferF;
}

//------------------------------------------------------------------------------
bool AiffReader::findChunk (ifstream& file, char const* targetChunkID, unsigned& outChunkSize) {
   char chunkID[5];
   unsigned chunkSize = 0;
   do {
      file.seekg(chunkSize, ios_base::cur);
      file.get(chunkID, 5);
      file.read(reinterpret_cast<char*> (&chunkSize), 4);
      swapEndianness(chunkSize);
      cout << "ChunkID:    " << chunkID << '\n';
      cout << "Chunk Size: " << chunkSize << '\n';
   } while (file.good() and strcmp(chunkID, targetChunkID) != 0);
   if (!file.good()) {
      return false;
   }
   outChunkSize = chunkSize;
   return true;
}
