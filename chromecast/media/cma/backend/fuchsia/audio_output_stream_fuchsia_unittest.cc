// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/fuchsia/audio_output_stream_fuchsia.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace chromecast {
namespace media {

class AudioOutputStreamFuchsiaTest : public ::testing::Test {
 protected:
  AudioOutputStreamFuchsia output_;
};

TEST_F(AudioOutputStreamFuchsiaTest, StartAndStop) {
  EXPECT_TRUE(output_.Start(48000, 2));
  EXPECT_EQ(output_.GetSampleRate(), 48000);
  output_.Stop();
}

}  // namespace media
}  // namespace chromecast