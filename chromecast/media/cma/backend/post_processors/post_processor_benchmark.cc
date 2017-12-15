// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/post_processors/post_processor_benchmark.h"

#include <sys/resource.h>
#include <sys/time.h>

#include <cmath>

#include "base/logging.h"

namespace chromecast {
namespace media {
namespace post_processor_test {

namespace {
const int kTestDurationMs = 1000;
const int kBlockSizeFrames = 256;
const int kNumChannels = 2;

// Creates an interleaved stereo chirp waveform with |frames| frames
// from |start_frequency_left| to |start_frequency_left| for left channel and
// from |start_frequency_right| to |end_frequency_right| for right channel,
// where |start_frequency_x| and |end_frequency_x| are normalized frequencies
// (2 * freq_in_hz / sample_rate) i.e. 0 - DC, 1 - nyquist.
std::vector<float> GetStereoChirp(size_t frames,
                                  float start_frequency_left,
                                  float end_frequency_left,
                                  float start_frequency_right,
                                  float end_frequency_right) {
  std::vector<float> chirp(frames * 2);
  float ang_left = 0.0;
  float ang_right = 0.0;
  for (size_t i = 0; i < frames; i += 2) {
    ang_left += start_frequency_left +
                i * (end_frequency_left - start_frequency_left) * M_PI / frames;
    ang_right +=
        start_frequency_right +
        i * (end_frequency_right - start_frequency_right) * M_PI / frames;
    chirp[i] = sin(ang_left);
    chirp[i + 1] = sin(ang_right);
  }

  return chirp;
}

// Get current user CPU time in milliseconds or -1 if unsuccessful.
int GetUserTimeMs() {
  struct rusage usе_time;
  if (getrusage(RUSAGE_SELF, &usе_time) == 0) {
    return usе_time.ru_utime.tv_sec * 1000 + usе_time.ru_utime.tv_usec / 1000;
  }
  return -1;
}

}  // namespace

void AudioProcessorBenchmark(AudioPostProcessor* pp, int sample_rate) {
  int test_size_frames =
      (kTestDurationMs * sample_rate / 1000 / kBlockSizeFrames) *
      kBlockSizeFrames;
  std::vector<float> data_in =
      GetStereoChirp(test_size_frames, 0.0, 1.0, 0.0, 1.0);
  int start_ms = GetUserTimeMs();
  for (int i = 0; i < test_size_frames; i += kBlockSizeFrames * kNumChannels) {
    pp->ProcessFrames(&data_in[i], kBlockSizeFrames, 1.0, 0.0);
  }
  LOG(INFO) << "At " << sample_rate << "frames per second CPU usage: "
            << 100.0 * (GetUserTimeMs() - start_ms) / kTestDurationMs << "%";
}

}  // namespace post_processor_test
}  // namespace media
}  // namespace chromecast
