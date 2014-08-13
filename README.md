RGBeats
=======

Realtime, beat matching light show for LED stings, as seen as Burning Man 2014 at camp Silky Way.


Hardware
========

This softare is designed for the Teensy 3.1, OctoWS2811 Adaptor, and the Teensy Audio Board.
The OctoWS2811 Adaptor drives WS2811 or WS2812 LED strings (aka NeoPixels from Adafruit).


Overview
========

RGBeats generates a light show that moves in time with music.
It can roughly be divided into two parts: the half that deals with processing the music,
and the half that generates the light patterns.

AudioAnalyzeHFCOnset, BeatExtractor, and BeatTracker are used to process the audio signal.
Audio is sampled at 44.1 kHz with a 16 bit sample depth. At varying intervals, the high
frequency content measure, 512-bin spectrum, onsets, and dominant rhythmic pulse are extracted.
These are the features that are used to generate light patterns.

ColorUtils, LEDRing, and LightProgram control the generation of the light patterns.
They rely on the output of the audio processing code, and control a ring of LEDs
using the OctoWS2811 library.
