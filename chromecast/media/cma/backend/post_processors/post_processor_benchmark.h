// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_POST_PROCESSORS_POST_PROCESSOR_BENCHMARK_H
#define CHROMECAST_MEDIA_CMA_BACKEND_POST_PROCESSORS_POST_PROCESSOR_BENCHMARK_H

#include <string>
#include <vector>

#include "chromecast/public/media/audio_post_processor_shlib.h"

namespace chromecast {
namespace media {
namespace post_processor_test {

// Generate testing 2 channels waveform, run it on post-processor
// with given sample rate.
// Measure amount of CPU time |pp| takes to run [x] seconds of stereo audio at |sample_rate|.
void AudioProcessorBenchmark(AudioPostProcessor* pp, int sample_rate);

}  // namespace post_processor_test
}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_POST_PROCESSORS_POST_PROCESSOR_BENCHMARK_H
