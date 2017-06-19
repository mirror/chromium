// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/base/lap_timer.h"
#include "cc/layers/recording_source.h"
#include "cc/paint/display_item_list.h"
#include "cc/raster/raster_source.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"

namespace cc {
namespace {

static const int kTimeLimitMillis = 2000;
static const int kWarmupRuns = 5;
static const int kTimeCheckInterval = 10;

class DisplayItemListPerftest : public testing::Test {
 public:
  DisplayItemListPerftest()
      : timer_(kWarmupRuns,
               base::TimeDelta::FromMilliseconds(kTimeLimitMillis),
               kTimeCheckInterval) {}

  void RunSolidColorAnalysisTest(const gfx::Rect& rect) {
    gfx::Rect bounds;
    auto display_item_list = BuildDisplayItemList(&bounds);
    RecordingSource recording_source;
    Region last_invalidation;
    recording_source.UpdateAndExpandInvalidation(&last_invalidation,
                                                 bounds.size(), bounds);
    recording_source.UpdateDisplayItemList(display_item_list, 0);
    auto raster_source = recording_source.CreateRasterSource(true);

    timer_.Reset();
    do {
      SkColor color;
      raster_source->PerformSolidColorAnalysis(rect, 1.f, &color);
      timer_.NextLap();
    } while (!timer_.HasTimeLimitExpired());

    perf_test::PrintResult("solid_color_analysis", "", rect.ToString(),
                           timer_.LapsPerSecond(), "runs/s", true);
  }

 protected:
  scoped_refptr<DisplayItemList> BuildDisplayItemList(gfx::Rect* bounds) {
    auto inner_list = base::MakeRefCounted<DisplayItemList>();
    auto* inner_pob = inner_list->StartPaint();
    PaintFlags flags;
    flags.setColor(SK_ColorRED);
    inner_pob->push<DrawRectOp>(SkRect::MakeWH(16, 256), flags);
    inner_list->EndPaintOfUnpaired(gfx::Rect(0, 0, 16, 256));
    auto record = inner_list->ReleaseAsRecord();

    auto list = base::MakeRefCounted<DisplayItemList>();
    for (int y = 0; y <= 256 * 16; y += 256) {
      for (int x = 0; x <= 16 * 10; x += 16) {
        auto* pob = list->StartPaint();
        pob->push<SaveOp>();
        pob->push<TranslateOp>(x, y);
        pob->push<DrawRecordOp>(record);
        pob->push<RestoreOp>();
        list->EndPaintOfUnpaired(gfx::Rect(x, y, 16, 256));
      }
    }
    list->Finalize();
    *bounds = gfx::Rect(0, 0, 16 * 10, 256 * 16);
    return list;
  }

  LapTimer timer_;
};

TEST_F(DisplayItemListPerftest, SolidColorAnalysis) {
  RunSolidColorAnalysisTest(gfx::Rect(0, 0, 256, 256));
  RunSolidColorAnalysisTest(gfx::Rect(0, 7 * 256, 256, 256));
  RunSolidColorAnalysisTest(gfx::Rect(0, 15 * 256, 256, 256));
}

}  // namespace
}  // namespace cc
