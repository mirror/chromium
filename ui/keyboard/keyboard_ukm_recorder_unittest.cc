// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/keyboard_ukm_recorder.h"

#include "base/test/scoped_task_environment.h"
#include "components/ukm/test_ukm_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/ime/dummy_text_input_client.h"

namespace keyboard {
namespace {

class DummyTextInputClientWithSourceInfo : public ui::DummyTextInputClient {
 public:
  DummyTextInputClientWithSourceInfo(ui::TextInputType type, std::string info)
      : ui::DummyTextInputClient(type), source_info_(info) {}

  std::string GetClientSourceInfo() const override { return source_info_; }

 private:
  std::string source_info_;
};

}  // namespace

TEST(KeyboardUkmRecorderTest, RecordUkmWithEmptySourceInfo) {
  base::test::ScopedTaskEnvironment env;
  ukm::TestAutoSetUkmRecorder test_recorder;
  DummyTextInputClientWithSourceInfo input_client_none(ui::TEXT_INPUT_TYPE_NONE,
                                                       "");
  KeyboardUkmRecorder recorder;

  test_recorder.Purge();
  test_recorder.EnableRecording();
  ASSERT_EQ(0u, test_recorder.entries_count());
  recorder.UpdateUkmSource(
      static_cast<ui::TextInputClient*>(&input_client_none));
  EXPECT_EQ(0u, test_recorder.entries_count());
  recorder.RecordUkm();
  EXPECT_EQ(0u, test_recorder.sources_count());
  EXPECT_EQ(0u, test_recorder.entries_count());
}

TEST(KeyboardUkmRecorderTest, RecordUkm) {
  base::test::ScopedTaskEnvironment env;
  ukm::TestAutoSetUkmRecorder test_recorder;
  GURL url("chrome://flags");
  DummyTextInputClientWithSourceInfo input_client_text(ui::TEXT_INPUT_TYPE_TEXT,
                                                       "chrome://flags");
  KeyboardUkmRecorder recorder;

  test_recorder.Purge();
  test_recorder.EnableRecording();
  ASSERT_EQ(0u, test_recorder.entries_count());
  recorder.UpdateUkmSource(&input_client_text);
  EXPECT_EQ(0u, test_recorder.entries_count());
  recorder.RecordUkm();
  EXPECT_EQ(1u, test_recorder.sources_count());
  EXPECT_EQ(1u, test_recorder.entries_count());
  auto* source = test_recorder.GetSourceForUrl(url);
  test_recorder.ExpectMetric(*source, "VirtualKeyboard.Open", "TextInputType",
                             ui::TEXT_INPUT_TYPE_TEXT);
}

}  // namespace keyboard
