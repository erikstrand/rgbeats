//==============================================================================
// CompileConfig.h
// Created 2014/11/04
//==============================================================================

#ifndef COMPILECONFIG
#define COMPILECONFIG

// If this line is defined, we compile for Mac. Otherwise we compile for Teensy.
#define COMPILEFORMAC

#ifdef COMPILEFORMAC
#define int16_t __int16_t
#endif

#endif

