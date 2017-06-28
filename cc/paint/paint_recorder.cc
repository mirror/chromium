// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_recorder.h"

#include "cc/paint/display_item_list.h"
#include "ui/gfx/skia_util.h"

namespace cc {

PaintRecorder::PaintRecorder() = default;

PaintRecorder::~PaintRecorder() = default;

PaintCanvas* PaintRecorder::beginRecording(const SkRect& bounds) {
  display_item_list_ = base::MakeRefCounted<DisplayItemList>();
  display_item_list_->StartPaint();
  bounds_ = bounds;
  canvas_.emplace(display_item_list_.get(), bounds);
  return getRecordingCanvas();
}

sk_sp<PaintRecord> PaintRecorder::finishRecordingAsPicture() {
  // SkPictureRecorder users expect that their saves are automatically
  // closed for them.
  //
  // NOTE: Blink paint in general doesn't appear to need this, but the
  // RecordingImageBufferSurface::fallBackToRasterCanvas finishing off the
  // current frame depends on this.  Maybe we could remove this assumption and
  // just have callers do it.
  canvas_->restoreToCount(1);

  // Some users (e.g. printing) use the existence of the recording canvas
  // to know if recording is finished, so reset it here.
  canvas_.reset();

  display_item_list_->EndPaintOfUnpaired(
      gfx::ToEnclosingRect(gfx::SkRectToRectF(bounds_)));
  display_item_list_->Finalize();
  return display_item_list_->ReleaseAsRecord();
}

}  // namespace cc
