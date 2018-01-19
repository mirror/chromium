// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/debug_recording.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace audio {

class DebugRecordingFileProviderTest : public testing::Test {
 public:
  DebugRecordingFileProviderTest() {}
  ~DebugRecordingFileProviderTest() override {}

  MOCK_METHOD1(OnFileCreated, void(bool));
  void FileCreated(base::File file) { OnFileCreated(file.IsValid()); }

 protected:
};

TEST_F(DebugRecordingFileProviderTest, Create) {
  mojom::DebugRecordingFileProviderPtr file_provider_ptr;
  DebugRecordingFileProvider file_provider(
      mojo::MakeRequest(file_provider_ptr, file_path));
  EXPECT_CALL(this, OnFileCreated(true));
  file_provider_ptr->Create(
      suffix, base::BindOnce(&DebugRecordingFileProviderTest::FileCreated,
                             base::Unretained(this)));
}  // namespace audio
