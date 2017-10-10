// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "cc/base/lap_timer.h"
#include "chrome/browser/vr/elements/text.h"
#include "chrome/browser/vr/test/constants.h"
#include "chrome/browser/vr/test/gl_test_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"
#include <iostream>
#include <fstream>
#include "base/trace_event/trace_log.h"
#include "base/memory/ref_counted_memory.h"
#include "base/test/scoped_task_environment.h"

namespace vr {

namespace {

constexpr int kMaximumTextWidthPixels = 512;
constexpr size_t kNumberOfRuns = 35;
constexpr float kFontHeightMeters = 0.048f;
constexpr float kTextWidthMeters = 1.056f;

}  // namespace

class TextPerfTest : public testing::Test {
 public:
  void SetUp() override {
    gl_test_environment_ =
        base::MakeUnique<GlTestEnvironment>(kPixelHalfScreen);
    text_element_ = base::MakeUnique<Text>(kMaximumTextWidthPixels,
                                           kFontHeightMeters, kTextWidthMeters);
    text_element_->Initialize();
  }

  void TearDown() override {
    text_element_.reset();
    gl_test_environment_.reset();
  }

 protected:
  void PrintResults(const std::string& name) {
    perf_test::PrintResult(name, "", "render_time_avg", timer_.MsPerLap(), "ms",
                           true);
    perf_test::PrintResult(name, "", "number_of_runs",
                           static_cast<size_t>(timer_.NumLaps()), "runs", true);
  }

  void RenderAndLapTimer() {
    static_cast<UiElement*>(text_element_.get())->PrepareToDraw();
    // Make sure all GL commands are applied before we measure the time.
    glFinish();
    // timer_.NextLap();
  }

  std::unique_ptr<Text> text_element_;
  cc::LapTimer timer_;

 private:
  std::unique_ptr<GlTestEnvironment> gl_test_environment_;
};

TEST_F(TextPerfTest, RenderLoremIpsum100Chars) {
  base::string16 text = base::UTF8ToUTF16(kLoremIpsum100Chars);
  timer_.Reset();
  for (size_t i = 0; i < kNumberOfRuns; i++) {
    text_element_->SetText(text);
    RenderAndLapTimer();
  }
  PrintResults("render_lorem_ipsum_100_chars");
}

TEST_F(TextPerfTest, RenderLoremIpsum700Chars) {
  base::string16 text = base::UTF8ToUTF16(kLoremIpsum700Chars);

  // start of the test
  base::trace_event::TraceLog::GetInstance()->SetEnabled(
      base::trace_event::TraceConfig("gpu", ""),
      base::trace_event::TraceLog::RECORDING_MODE);

  text_element_->set_name(kTestText);

  timer_.Reset();
  // for (size_t i = 0; i < kNumberOfRuns; i++) {
    text_element_->SetText(text);
    RenderAndLapTimer();
  // }

  base::trace_event::TraceLog::GetInstance()->SetDisabled(base::trace_event::TraceLog::RECORDING_MODE);
  
  std::ofstream myfile;
  myfile.open ("/sdcard/test_trace.json", std::ios_base::trunc);
  myfile << "[";

  base::test::ScopedTaskEnvironment env(base::test::ScopedTaskEnvironment::MainThreadType::UI);

  base::trace_event::TraceLog::GetInstance()->Flush(
    base::Bind([](std::ofstream* myfile,
                  const scoped_refptr<base::RefCountedString>& str,
                  bool has_more_events) {
      (*myfile) << str->data();
    }, &myfile));
  env.RunUntilIdle();
  myfile << "]";
  myfile.close();

  // PrintResults("render_lorem_ipsum_700_chars");
}

}  // namespace vr
