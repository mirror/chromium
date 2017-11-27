// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/keyboard_ukm_recorder.h"

#include "base/test/scoped_task_environment.h"
#include "components/ukm/test_ukm_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/ime/text_input_type.h"

namespace keyboard {

TEST(KeyboardUkmRecorderTest, RecordUkmWithEmptySourceInfo) {
  base::test::ScopedTaskEnvironment env;
  ukm::TestAutoSetUkmRecorder test_recorder;
  KeyboardUkmRecorder recorder;

  test_recorder.Purge();
  test_recorder.EnableRecording();
  EXPECT_EQ(0u, test_recorder.entries_count());
  recorder.RecordUkm(GURL::EmptyGURL(), ui::TEXT_INPUT_TYPE_NONE);
  EXPECT_EQ(0u, test_recorder.sources_count());
  EXPECT_EQ(0u, test_recorder.entries_count());
}

TEST(KeyboardUkmRecorderTest, RecordUkm) {
  base::test::ScopedTaskEnvironment env;
  ukm::TestAutoSetUkmRecorder test_recorder;
  GURL url("chrome://flags");
  KeyboardUkmRecorder recorder;

  test_recorder.Purge();
  test_recorder.EnableRecording();
  ASSERT_EQ(0u, test_recorder.entries_count());
  recorder.RecordUkm(url, ui::TEXT_INPUT_TYPE_TEXT);
  EXPECT_EQ(1u, test_recorder.sources_count());
  EXPECT_EQ(1u, test_recorder.entries_count());
  auto* source = test_recorder.GetSourceForUrl(url);
  test_recorder.ExpectMetric(*source, "VirtualKeyboard.Open", "TextInputType",
                             ui::TEXT_INPUT_TYPE_TEXT);
}

}  // namespace keyboard
