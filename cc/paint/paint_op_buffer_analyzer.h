// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_OP_BUFFER_ANALYZER_H_
#define CC_PAINT_PAINT_OP_BUFFER_ANALYZER_H_

#include <vector>

#include "base/optional.h"
#include "cc/paint/paint_export.h"
#include "third_party/skia/include/utils/SkNoDrawCanvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/skia_util.h"

namespace cc {
class PaintOpBuffer;
struct PaintOp;

template <typename Derived>
class CC_PAINT_EXPORT PaintOpBufferAnalyzer {
 public:
  explicit PaintOpBufferAnalyzer(const PaintOpBuffer* paint_op_buffer);
  virtual ~PaintOpBufferAnalyzer();

  void RunAnalysis(const gfx::Rect& rect,
                   const std::vector<size_t>* indices = nullptr);

 protected:
  void abort_analysis() { analysis_aborted_ = true; }
  const SkCanvas& state_canvas() { return *state_canvas_; }

 private:
  // This functions can be "overriden" by the derived classes, and the derived
  // class version will be run.
  bool WillDrawPaintOpBuffer(const PaintOp* op) { return true; }
  void HandleDrawOp(const PaintOp* op) {}
  void HandleStateOp(const PaintOp* op) {}
  void HandleSaveOp(const PaintOp* op) {}
  void HandleRestoreOp(bool implicit) {}

  void CreateStateForAnalysis(const gfx::Rect& rect);

  bool analysis_aborted_ = false;
  const PaintOpBuffer* paint_op_buffer_ = nullptr;
  base::Optional<SkNoDrawCanvas> state_canvas_;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_OP_BUFFER_ANALYZER_H_
