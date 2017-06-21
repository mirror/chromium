// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_op_buffer_analyzer.h"

#include "base/trace_event/trace_event.h"
#include "cc/paint/paint_op_buffer.h"

#include "cc/paint/solid_color_analyzer.h"

namespace cc {

template <typename Derived>
PaintOpBufferAnalyzer<Derived>::PaintOpBufferAnalyzer(
    const PaintOpBuffer* paint_op_buffer)
    : paint_op_buffer_(paint_op_buffer) {}

template <typename Derived>
PaintOpBufferAnalyzer<Derived>::~PaintOpBufferAnalyzer() = default;

template <typename Derived>
void PaintOpBufferAnalyzer<Derived>::RunAnalysis(
    const gfx::Rect& rect,
    const std::vector<size_t>* indices) {
  // Nothing to do if we're analyzing 0 ops.
  if (paint_op_buffer_->size() == 0 || (indices && indices->empty()))
    return;

  TRACE_EVENT_BEGIN1(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
                     "PaintOpBufferAnalyzer::RunAnalysis", "first_idx",
                     indices ? indices->front() : -1);

  struct Frame {
    Frame() = default;
    Frame(PaintOpBuffer::Iterator iter,
          const SkMatrix& original_ctm,
          int save_count)
        : iter(iter), original_ctm(original_ctm), save_count(save_count) {}

    PaintOpBuffer::Iterator iter;
    const SkMatrix original_ctm;
    int save_count = 0;
  };

  CreateStateForAnalysis(rect);
  std::vector<Frame> stack;
  // We expect to see at least one DrawRecordOp because of the way items are
  // constructed. Reserve this to 2, and go from there.
  stack.reserve(2);
  stack.emplace_back(PaintOpBuffer::Iterator(paint_op_buffer_, indices),
                     state_canvas_->getTotalMatrix(),
                     state_canvas_->getSaveCount());

  int loop_iterations = 0;
  while (!analysis_aborted_ && !stack.empty()) {
    ++loop_iterations;

    auto& frame = stack.back();
    if (!frame.iter) {
      while (state_canvas_->getSaveCount() > frame.save_count) {
        static_cast<Derived*>(this)->HandleRestoreOp(true);
        state_canvas_->restore();
      }
      stack.pop_back();
      if (!stack.empty())
        ++stack.back().iter;
      continue;
    }

    const PaintOp* op = *frame.iter;
    const SkMatrix& original_ctm = frame.original_ctm;
    switch (op->GetType()) {
      // Unpack draw records by pushing things on the stack.
      case PaintOpType::DrawRecord: {
        if (!static_cast<Derived*>(this)->WillDrawPaintOpBuffer(op))
          break;

        const DrawRecordOp* record_op = static_cast<const DrawRecordOp*>(op);
        stack.emplace_back(PaintOpBuffer::Iterator(record_op->record.get()),
                           state_canvas_->getTotalMatrix(),
                           state_canvas_->getSaveCount());
        continue;
      }
      // Handle all of the draw ops by deferring to HandleDrawOp().
      case PaintOpType::DrawArc:
      case PaintOpType::DrawCircle:
      case PaintOpType::DrawColor:
      case PaintOpType::DrawDRRect:
      case PaintOpType::DrawImage:
      case PaintOpType::DrawImageRect:
      case PaintOpType::DrawIRect:
      case PaintOpType::DrawLine:
      case PaintOpType::DrawOval:
      case PaintOpType::DrawPath:
      case PaintOpType::DrawPosText:
      case PaintOpType::DrawRect:
      case PaintOpType::DrawRRect:
      case PaintOpType::DrawText:
      case PaintOpType::DrawTextBlob:
        static_cast<Derived*>(this)->HandleDrawOp(op);
        break;
      // Save, save layer, and restore ops need to be handled by both our
      // derived classes and by the state canvas.
      case PaintOpType::Save:
      case PaintOpType::SaveLayer:
      case PaintOpType::SaveLayerAlpha:
        static_cast<Derived*>(this)->HandleSaveOp(op);
        op->Raster(&*state_canvas_, original_ctm);
        break;
      case PaintOpType::Restore:
        static_cast<Derived*>(this)->HandleRestoreOp(false);
        op->Raster(&*state_canvas_, original_ctm);
        break;
      // Ops affecting only clips and ctm only need to update the
      // |state_canvas_|.
      case PaintOpType::Annotate:
      case PaintOpType::ClipPath:
      case PaintOpType::ClipRect:
      case PaintOpType::ClipRRect:
      case PaintOpType::Concat:
      case PaintOpType::Scale:
      case PaintOpType::SetMatrix:
      case PaintOpType::Rotate:
      case PaintOpType::Translate:
        static_cast<Derived*>(this)->HandleStateOp(op);
        op->Raster(&*state_canvas_, original_ctm);
        break;
      // Handle the rest of the enums so that we can be confident we're handling
      // any new ops that might appear.
      case PaintOpType::Noop:
        break;
    }
    ++frame.iter;
  }

  TRACE_EVENT_END1(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
                   "PaintOpBufferAnalyzer::RunAnalysis", "loop_iterations",
                   loop_iterations);
}

template <typename Derived>
void PaintOpBufferAnalyzer<Derived>::CreateStateForAnalysis(
    const gfx::Rect& rect) {
  analysis_aborted_ = false;

  state_canvas_.emplace(rect.width(), rect.height());
  state_canvas_->translate(-rect.x(), -rect.y());
  state_canvas_->clipRect(gfx::RectToSkRect(rect), SkClipOp::kIntersect, false);
}

template class PaintOpBufferAnalyzer<SolidColorAnalyzer>;

}  // namespace cc
