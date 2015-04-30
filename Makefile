gcc = clang++ -Wall -std=c++11 -g
cc = clang

.PHONY: all
all: bin/main

.PHONY: test
test: bin/test

.PHONY: clean
clean:
	rm -rf bin/*

#targets = bin/ColorUtils.o bin/LightProgram.o bin/MusicState.o bin/Random.o

#bin/main: testmain.cpp $(targets) ColorUtils.h LightProgram.h LightProgramTemplates.h
bin/main: maconly/main.cpp bin/AiffSampler.o bin/AiffReader.o bin/kiss_fft.o bin/sqrt_integer.o lib/gtest.a
	$(gcc) $^ -o $@ -I. -Imaconly -IKissFFT -IPJRCAudio

bin/sqrt_integer.o: PJRCAudio/sqrt_integer.c PJRCAudio/sqrt_integer.h
	$(gcc) -c $< -o $@ -IPJRCAudio

bin/kiss_fft.o: KissFFT/kiss_fft.c KissFFT/kiss_fft.h
	$(cc) -c $< -o $@ -IKissFFT

bin/AiffSampler.o: maconly/AiffSampler.cpp maconly/AiffSampler.h
	$(gcc) -c $< -o $@ -I. -Imaconly

bin/AiffReader.o: maconly/AiffReader.cpp maconly/AiffReader.h
	$(gcc) -c $< -o $@ -I. -Imaconly

lib/gtest.a: bin/gtest.o
	ar -rv lib/gtest.a bin/gtest.o


bin/gtest.o: gtest/src/gtest-all.cc
	g++ -isystem gtest/include -Igtest/include -Igtest -pthread -c gtest/src/gtest-all.cc -o bin/gtest.o


bin/test: test/testmain.cpp lib/gtest.a bin/sqrt_integer_test.o bin/sqrt_integer.o bin/RingBufferTest.o bin/UtilsTest.o
	$(gcc) $^ -o $@ -I. -Igtest/include -pthread

bin/sqrt_integer_test.o: test/sqrt_integer_test.cpp
	$(gcc) -c $< -o $@ -I. -IPJRCAudio -Igtest/include/gtest -Igtest/include -pthread

bin/RingBufferTest.o: test/RingBufferTest.cpp RingBuffer.h
	$(gcc) -c $< -o $@ -I. -IPJRCAudio -Igtest/include/gtest -Igtest/include -pthread

bin/UtilsTest.o: test/UtilsTest.cpp Utils.h
	$(gcc) -c $< -o $@ -I. -Igtest/include/gtest -Igtest/include -pthread


#bin/ColorUtils.o: ColorUtils.cpp ColorUtils.h
#	$(gcc) -c $< -o $@
#
#bin/LightProgram.o: LightProgram.cpp LightProgram.h
#	$(gcc) -c $< -o $@
#
#bin/MusicState.o: MusicState.cpp MusicState.h
#	$(gcc) -c $< -o $@
#
#bin/Random.o: Random.cpp Random.h
#	$(gcc) -c $< -o $@

