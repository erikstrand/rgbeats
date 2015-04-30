//==============================================================================
// AiffReader.h
// Created 8/31/11.
//==============================================================================

#ifndef AIFF_READER
#define AIFF_READER

#include <cstdlib>
#include <iosfwd>
#include "ESAudio.h"


//==============================================================================
// Endianness Swappers
//==============================================================================

inline void swapEndianness (char* byte, unsigned bytes) {
   char temp;
   for (unsigned i=0; i<(bytes>>1); ++i) {
      temp = byte[i];
      byte[i] = byte[bytes-1-i];
      byte[bytes-1-i] = temp;
   }
}   

template <class Type>
Type& swapEndianness (Type& t) {
   swapEndianness(reinterpret_cast<char*> (&t), sizeof(Type));
   return t;
}


//==============================================================================
// Class AiffReader
//==============================================================================

class AiffReader {
public:
   enum State { OK, NotAiff, CorruptFile };

private:
   State _state;

public:
   AiffReader () : _state(OK) {}
   inline bool stateIsOK () { return _state == OK; }
   
   SampleBufferF load (char const* filename);
   void unLoad (SampleBufferF& buffer) { if (buffer.data) free(buffer.data); }

private:
   bool findChunk (std::ifstream& file, char const* chunkID, unsigned& outChunkSize);
};


//------------------------------------------------------------------------------
// Inline Method Definitions
//------------------------------------------------------------------------------


//==============================================================================
// About The Aiff File Format 
//==============================================================================

//------------------------------------------------------------------------------
// History
/*
 * The Audio Interchange File Format was developed by Apple back during the
 * cretacious period of computers (late 70s, early 80s?). It is based on the
 * Interchange File Format standard put forth by ??? at an even earlier time.
 *
 * Because Apple computers were at the time big endian, aiff files were
 * originally all big endian. However, at some point a compressed version of
 * aiff was developed (AIFC), and when Apple switched to little endian intel
 * chips they took advantage of this by developing the "sowt"
 * "compression codec". Of course the sowt codec does not compress aiff files
 * at all, it just switches the endianness of everything in them. Theoretically
 * this is the predominant species of aiff on modern Apple computers, though I
 * have yet to observe one in the wild.
 */

//------------------------------------------------------------------------------
// Specs
/*
 * An Aiff file is made up of "chunks". Each chunk has a header that includes
 * a 32 bit chunkID, and a 32 bit unsigned chunkSize. The chunkID identifies
 * the type of chunk, and the chunkSize stores the size of the chunk in bytes,
 * excluding the header. All Aiff files must have a FORM chunk, a COMM chunk,
 * and a SSND chunk. The FORM chunk simply identifies the file as an Aiff file
 * (AIFF or AIFC). The COMM chunk includes information like bit depth and sample
 * rate. The SSND chunk contains the actual audio data (generally 16 bit
 * big endian integer samples). The FORM chunk must come first, and in this
 * code I assume the COMM chunk comes before the SSND chunk, though I'm not sure
 * this is actually required by the standard. Many other chunk types are defined
 * (see Aiff.h), but must of them are useless.
 */


#endif // AIFF_READER
