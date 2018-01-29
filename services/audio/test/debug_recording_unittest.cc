// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/debug_recording.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace audio {

class DebugRecordingTest : public testing::Test {
 public:
  DebugRecordingTest() {}
  ~DebugRecordingTest() override {}

 protected:
  // DebugRecording debug_recording_;
};

TEST_F(DebugRecordingTest, Enable) {}

}  // namespace audio
