// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/media_capabilities_reporter.h"
#include "base/message_loop/message_loop.h"
#include "media/mojo/interfaces/media_capabilities_recorder.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// #include "base/run_loop.h"
// #include "base/test/test_message_loop.h"

namespace media {

class MediaCapabilitiesInterceptor : public mojom::MediaCapabilitiesRecorder {
 public:
  MediaCapabilitiesInterceptor() {}
  ~MediaCapabilitiesInterceptor() override {}

  void StartNewRecord(VideoCodecProfile profile,
                      const gfx::Size& natural_size,
                      int frames_per_sec) override {}

  void UpdateRecord(int frames_decoded, int frames_dropped) override {}

  void FinalizeRecord() override {}
};

class MediaCapabilitiesReporterTest : public ::testing::Test {
 public:
  MediaCapabilitiesReporterTest() {}

 private:
  base::MessageLoop message_loop_;
};

mojom::MediaCapabilitiesRecorderPtr GetRecorderPtr() {
  mojom::MediaCapabilitiesRecorderPtr recorder_ptr;
  mojom::MediaCapabilitiesRecorderRequest request =
      mojo::MakeRequest(&recorder_ptr);
  mojo::MakeStrongBinding(base::MakeUnique<MediaCapabilitiesInterceptor>(),
                          mojo::MakeRequest(&recorder_ptr));
  return recorder_ptr;
}

TEST_F(MediaCapabilitiesReporterTest, Foo) {
  mojom::MediaCapabilitiesRecorderPtr recorder_ptr = GetRecorderPtr();
  EXPECT_TRUE(!!recorder_ptr);
}

}  // namespace media