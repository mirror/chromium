// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/clip_recorder.h"

#include "cc/paint/clip_display_item.h"
#include "cc/paint/clip_path_display_item.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/record_paint_canvas.h"
#include "ui/compositor/paint_context.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/path.h"
#include "ui/gfx/skia_util.h"

namespace ui {

ClipRecorder::ClipRecorder(const PaintContext& context) : context_(context) {}

ClipRecorder::~ClipRecorder() {
  for (int i = 0; i < num_closers_; ++i) {
    cc::RecordPaintCanvas record_canvas(context_.list_->StartPaint(),
                                        SkRect::MakeEmpty());
    record_canvas.restore();
    context_.list_->EndPaintOfPairedEnd();
  }
}

void ClipRecorder::ClipRect(const gfx::Rect& clip_rect) {
  bool antialias = false;

  cc::RecordPaintCanvas record_canvas(context_.list_->StartPaint(),
                                      SkRect::MakeEmpty());
  record_canvas.save();
  record_canvas.clipRect(gfx::RectToSkRect(clip_rect), SkClipOp::kIntersect,
                         antialias);
  context_.list_->EndPaintOfPairedBegin();
  ++num_closers_;
}

void ClipRecorder::ClipPath(const gfx::Path& clip_path) {
  bool antialias = false;

  cc::RecordPaintCanvas record_canvas(context_.list_->StartPaint(),
                                      SkRect::MakeEmpty());
  record_canvas.save();
  record_canvas.clipPath(clip_path, SkClipOp::kIntersect, antialias);
  context_.list_->EndPaintOfPairedBegin();
  ++num_closers_;
}

void ClipRecorder::ClipPathWithAntiAliasing(const gfx::Path& clip_path) {
  bool antialias = true;

  cc::RecordPaintCanvas record_canvas(context_.list_->StartPaint(),
                                      SkRect::MakeEmpty());
  record_canvas.save();
  record_canvas.clipPath(clip_path, SkClipOp::kIntersect, antialias);
  context_.list_->EndPaintOfPairedBegin();
  ++num_closers_;
}

}  // namespace ui
