// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/solid_color_analyzer.h"

#include "base/trace_event/trace_event.h"
#include "cc/paint/paint_op_buffer.h"

namespace cc {
namespace {
const int kMaxOpsToAnalyze = 1;

bool ActsLikeClear(SkBlendMode mode, unsigned src_alpha) {
  switch (mode) {
    case SkBlendMode::kClear:
      return true;
    case SkBlendMode::kSrc:
    case SkBlendMode::kSrcIn:
    case SkBlendMode::kDstIn:
    case SkBlendMode::kSrcOut:
    case SkBlendMode::kDstATop:
      return src_alpha == 0;
    case SkBlendMode::kDstOut:
      return src_alpha == 0xFF;
    default:
      return false;
  }
}

bool IsSolidColor(SkColor color, SkBlendMode blendmode) {
  return SkColorGetA(color) == 255 &&
         (blendmode == SkBlendMode::kSrc || blendmode == SkBlendMode::kSrcOver);
}

bool IsSolidColorPaint(const PaintFlags& flags) {
  SkBlendMode blendmode = flags.getBlendMode();

  // Paint is solid color if the following holds:
  // - Alpha is 1.0, style is fill, and there are no special effects
  // - Xfer mode is either kSrc or kSrcOver (kSrcOver is equivalent
  //   to kSrc if source alpha is 1.0, which is already checked).
  return IsSolidColor(flags.getColor(), blendmode) && !flags.HasShader() &&
         !flags.getLooper() && !flags.getMaskFilter() &&
         !flags.getColorFilter() && !flags.getImageFilter() &&
         flags.getStyle() == PaintFlags::kFill_Style;
}

// Returns true if the specified drawn_rect will cover the entire canvas, and
// that the canvas is not clipped (i.e. it covers ALL of the canvas).
bool IsFullQuad(const SkCanvas& canvas, const SkRect& drawn_rect) {
  if (!canvas.isClipRect())
    return false;

  SkIRect clip_irect;
  if (!canvas.getDeviceClipBounds(&clip_irect))
    return false;

  // if the clip is smaller than the canvas, we're partly clipped, so abort.
  if (!clip_irect.contains(SkIRect::MakeSize(canvas.getBaseLayerSize())))
    return false;

  const SkMatrix& matrix = canvas.getTotalMatrix();
  // If the transform results in a non-axis aligned
  // rect, then be conservative and return false.
  if (!matrix.rectStaysRect())
    return false;

  SkRect device_rect;
  matrix.mapRect(&device_rect, drawn_rect);
  SkRect clip_rect;
  clip_rect.set(clip_irect);
  return device_rect.contains(clip_rect);
}

}  // namespace

SolidColorAnalyzer::SolidColorAnalyzer(const PaintOpBuffer* paint_op_buffer)
    : PaintOpBufferAnalyzer<SolidColorAnalyzer>(paint_op_buffer) {}

SolidColorAnalyzer::~SolidColorAnalyzer() = default;

bool SolidColorAnalyzer::GetColorIfSolid(SkColor* color) const {
  if (is_transparent_) {
    *color = SK_ColorTRANSPARENT;
    return true;
  }
  if (is_solid_color_) {
    *color = color_;
    return true;
  }
  return false;
}

void SolidColorAnalyzer::HandleDrawOp(const PaintOp* op) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "SolidColorAnalyzer::HandleDrawOp");
  ++num_ops_;
  if ((op->GetType() != PaintOpType::DrawColor &&
       op->GetType() != PaintOpType::DrawRect) ||
      num_ops_ > kMaxOpsToAnalyze) {
    is_solid_color_ = false;
    is_transparent_ = false;
    abort_analysis();
    return;
  }

  if (op->GetType() == PaintOpType::DrawColor) {
    const DrawColorOp* color_op = static_cast<const DrawColorOp*>(op);
    HandleDrawColor(color_op->color, color_op->mode);
  } else {
    const DrawRectOp* rect_op = static_cast<const DrawRectOp*>(op);
    HandleDrawRect(rect_op->rect, rect_op->flags);
  }
}

void SolidColorAnalyzer::HandleDrawColor(SkColor color, SkBlendMode blendmode) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "SolidColorAnalyzer::HandleDrawColor");
  const SkCanvas& canvas = state_canvas();
  SkRect rect;
  if (!canvas.getLocalClipBounds(&rect)) {
    is_transparent_ = false;
    is_solid_color_ = false;
    abort_analysis();
  }

  bool does_cover_canvas = IsFullQuad(canvas, rect);
  uint8_t alpha = SkColorGetA(color);
  if (does_cover_canvas && ActsLikeClear(blendmode, alpha))
    is_transparent_ = true;
  else if (alpha != 0 || blendmode != SkBlendMode::kSrc)
    is_transparent_ = false;

  if (does_cover_canvas && IsSolidColor(color, blendmode)) {
    is_solid_color_ = true;
    color_ = color;
  } else {
    is_solid_color_ = false;
  }
}

void SolidColorAnalyzer::HandleDrawRect(const SkRect& rect,
                                        const PaintFlags& flags) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "SolidColorAnalyzer::HandleDrawRect");
  const SkCanvas& canvas = state_canvas();

  if (flags.nothingToDraw())
    return;

  bool does_cover_canvas = IsFullQuad(canvas, rect);
  SkBlendMode blendmode = flags.getBlendMode();
  if (does_cover_canvas && ActsLikeClear(blendmode, flags.getAlpha()))
    is_transparent_ = true;
  else if (flags.getAlpha() != 0 || blendmode != SkBlendMode::kSrc)
    is_transparent_ = false;

  if (does_cover_canvas && IsSolidColorPaint(flags)) {
    is_solid_color_ = true;
    color_ = flags.getColor();
  } else {
    is_solid_color_ = false;
  }
}

void SolidColorAnalyzer::HandleSaveOp(const PaintOp* op) {
  if (op->GetType() != PaintOpType::Save) {
    is_solid_color_ = false;
    is_transparent_ = false;
    abort_analysis();
  }
}

}  // namespace cc
