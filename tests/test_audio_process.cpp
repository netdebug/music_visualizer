#include <iostream>
using std::cout;
using std::endl;
#include <fstream>
using std::ifstream;
#include <array>
using std::array;
#include <stdexcept>
#include <chrono>
namespace chrono = std::chrono;

#include "fake_clock.h"
#include "AudioProcess.h"
#include "AudioStreams/WavAudioStream.h"
#include "AudioStreams/ProceduralAudioStream.h"
using AudioStreamT = ProceduralAudioStream;

#include "catch2/catch.hpp"

constexpr int canvas_height = 400;
constexpr int canvas_width = 1024;
static array<array<float, canvas_width>, canvas_height> canvas;
void paint(const struct audio_data &my_audio_data) {
	// accumulate blur into canvas
	for (int j = 0; j < canvas_width; ++j) {
		float y = my_audio_data.audio_l[j];
		y = .5f*y + .5f;
		y *= canvas_height;
		if (y > 399) y = 399;
		if (y < 0) y = 0;
		canvas[int(y)][j] = 1.f;
	}
}

int score() {
	int sum = 0;
	for (int i = 0; i < canvas_height; ++i) {
		for (int j = 0; j < canvas_width; ++j) {
			sum += int(canvas[i][j]);
		}
	}
    return sum;
}

// Outputs a measure of performance of the waveform stabilization optimizations in audioprocess.cpp
TEST_CASE("Optimization performance test") {
	// TODO make AudioProcess take flags by argument so that I can turn off renormalization and other stuff
	// for testing specific optimizations.

	AudioStreamT as([](float* l, float* r, int s) {
		static int j = 0;
		static int t = 0;
		const double step1 = 3.141592 * 2. / float(s/2);
		const double step2 = 3.141592 * 2. / float(s/8);
		for (int i = 0; i < s; ++i, ++j, ++t)
			l[i] = r[i] = float(sin(step1 * j) + .3*cos(step2 * t));
		j %= s / 2;
		t %= s / 8;
		std::this_thread::sleep_for(std::chrono::microseconds(8560));
	});
	AudioProcess<fake_clock, AudioStreamT> ap(as);

	auto start = fake_clock::now();
	int i = 100;
	while (i > 0) {
		ap.step();
		paint(ap.get_audio_data());
		fake_clock::advance(chrono::microseconds(8650));
		i--;
	}
    cout << "AudioProcess optimization performance: " << score() << endl;
}