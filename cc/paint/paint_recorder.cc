// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_recorder.h"

#include "cc/paint/display_item_list.h"
#include "ui/gfx/skia_util.h"

namespace cc {

PaintRecorder::PaintRecorder() {
  display_item_list_ = base::MakeRefCounted<DisplayItemList>(
      DisplayItemList::kToBeReleasedAsPaintOpBuffer);
}

PaintRecorder::~PaintRecorder() = default;

PaintCanvas* PaintRecorder::beginRecording(const SkRect& bounds) {
#if DCHECK_IS_ON()
  DCHECK(!in_recording_);
  in_recording_ = true;
#endif
  display_item_list_->StartPaint();
  bounds_ = bounds;
  canvas_.emplace(display_item_list_.get(), bounds);
  return getRecordingCanvas();
}

sk_sp<PaintRecord> PaintRecorder::finishRecordingAsPicture() {
#if DCHECK_IS_ON()
  DCHECK(in_recording_);
  in_recording_ = false;
#endif

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
