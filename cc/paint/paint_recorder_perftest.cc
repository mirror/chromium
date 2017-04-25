// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_recorder.cc"

#include "base/threading/thread_task_runner_handle.h"
#include "cc/base/lap_timer.h"
#include "cc/paint/paint_record.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"

namespace cc {

namespace {

static const int kTimeLimitMillis = 10000;
static const int kWarmupRuns = 10;
static const int kTimeCheckInterval = 10;

class PaintRecorderPerfTest : public testing::Test {
 public:
  PaintRecorderPerfTest()
      : timer_(kWarmupRuns,
               base::TimeDelta::FromMilliseconds(kTimeLimitMillis),
               kTimeCheckInterval) {}

 protected:
  void SetUp() override {}

  void TearDown() override {}

  LapTimer timer_;
};

TEST_F(PaintRecorderPerfTest, OneOpNewRecorder) {
  timer_.Reset();

  SkRect rect = SkRect::MakeXYWH(1, 2, 3, 4);
  PaintFlags flags;
  do {
    PaintRecorder recorder;

    PaintCanvas* canvas = recorder.beginRecording(100, 100);
    canvas->drawRect(rect, flags);

    sk_sp<PaintRecord> record = recorder.finishRecordingAsPicture();

    timer_.NextLap();
  } while (!timer_.HasTimeLimitExpired());

  perf_test::PrintResult("one_op-new_recorder", "", "rate",
                         timer_.LapsPerSecond(), "runs/s", true);
}

TEST_F(PaintRecorderPerfTest, OneOpSharedRecorder) {
  timer_.Reset();

  SkRect rect = SkRect::MakeXYWH(1, 2, 3, 4);
  PaintFlags flags;
  PaintRecorder recorder;

  do {
    PaintCanvas* canvas = recorder.beginRecording(100, 100);
    canvas->drawRect(rect, flags);

    sk_sp<PaintRecord> record = recorder.finishRecordingAsPicture();

    timer_.NextLap();
  } while (!timer_.HasTimeLimitExpired());

  perf_test::PrintResult("one_op-shared_recorder", "", "rate",
                         timer_.LapsPerSecond(), "runs/s", true);
}

TEST_F(PaintRecorderPerfTest, ThreeOpNewRecorder) {
  timer_.Reset();

  SkRect rect = SkRect::MakeXYWH(1, 2, 3, 4);
  PaintFlags flags;
  uint8_t alpha = static_cast<uint8_t>(100u);
  do {
    PaintRecorder recorder;

    PaintCanvas* canvas = recorder.beginRecording(100, 100);
    canvas->saveLayerAlpha(nullptr, alpha);
    canvas->drawRect(rect, flags);
    canvas->restore();

    sk_sp<PaintRecord> record = recorder.finishRecordingAsPicture();

    timer_.NextLap();
  } while (!timer_.HasTimeLimitExpired());

  perf_test::PrintResult("three_op-new_recorder", "", "rate",
                         timer_.LapsPerSecond(), "runs/s", true);
}

TEST_F(PaintRecorderPerfTest, ThreeOpSharedRecorder) {
  timer_.Reset();

  SkRect rect = SkRect::MakeXYWH(1, 2, 3, 4);
  PaintFlags flags;
  uint8_t alpha = static_cast<uint8_t>(100u);
  PaintRecorder recorder;

  do {
    PaintCanvas* canvas = recorder.beginRecording(100, 100);
    canvas->saveLayerAlpha(nullptr, alpha);
    canvas->drawRect(rect, flags);
    canvas->restore();

    sk_sp<PaintRecord> record = recorder.finishRecordingAsPicture();

    timer_.NextLap();
  } while (!timer_.HasTimeLimitExpired());

  perf_test::PrintResult("three-shared_recorder", "", "rate",
                         timer_.LapsPerSecond(), "runs/s", true);
}

}  // namespace

}  // namespace cc
