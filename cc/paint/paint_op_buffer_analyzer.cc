// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_op_buffer_analyzer.h"

#include "base/trace_event/trace_event.h"
#include "cc/paint/paint_op_buffer.h"

namespace cc {

PaintOpBufferAnalyzer::PaintOpBufferAnalyzer(
    const PaintOpBuffer* paint_op_buffer)
    : paint_op_buffer_(paint_op_buffer) {}
PaintOpBufferAnalyzer::~PaintOpBufferAnalyzer() = default;

void PaintOpBufferAnalyzer::RunAnalysis(const gfx::Rect& rect,
                                        const std::vector<size_t>* indices) {
  TRACE_EVENT0("cc", "PaintOpBufferAnalyzer::RunAnalysis");
  // Nothing to do if we're analyzing 0 ops.
  if (paint_op_buffer_->size() == 0 || (indices && indices->empty()))
    return;

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
  std::vector<Frame> stack{
      Frame(PaintOpBuffer::Iterator(paint_op_buffer_, indices),
            state_canvas_->getTotalMatrix(), state_canvas_->getSaveCount())};

  while (!analysis_aborted_ && !stack.empty()) {
    auto& frame = stack.back();
    if (!frame.iter) {
      while (state_canvas_->getSaveCount() > frame.save_count) {
        HandleRestoreOp();
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
        HandleDrawOp(op);
        break;
      // Save, save layer, and restore ops need to be handled by both our
      // derived classes and by the state canvas.
      case PaintOpType::Save:
      case PaintOpType::SaveLayer:
      case PaintOpType::SaveLayerAlpha:
        HandleSaveOp(op);
        op->Raster(&*state_canvas_, original_ctm);
        break;
      case PaintOpType::Restore:
        HandleRestoreOp();
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
        op->Raster(&*state_canvas_, original_ctm);
        break;
      // Handle the rest of the enums so that we can be confident we're handling
      // any new ops that might appear.
      case PaintOpType::Noop:
        break;
    }
    ++frame.iter;
  }
  state_canvas_.reset();
}

void PaintOpBufferAnalyzer::CreateStateForAnalysis(const gfx::Rect& rect) {
  analysis_aborted_ = false;

  DCHECK(!state_canvas_);
  state_canvas_.emplace(rect.width(), rect.height());
  state_canvas_->translate(-rect.x(), -rect.y());
  state_canvas_->clipRect(gfx::RectToSkRect(rect), SkClipOp::kIntersect, false);
}

}  // namespace cc
