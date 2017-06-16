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

class CC_PAINT_EXPORT PaintOpBufferAnalyzer {
 public:
  explicit PaintOpBufferAnalyzer(const PaintOpBuffer* paint_op_buffer);
  virtual ~PaintOpBufferAnalyzer();

  void RunAnalysis(const gfx::Rect& rect,
                   const std::vector<size_t>* indices = nullptr);

 protected:
  virtual void HandleDrawOp(const PaintOp* op) {}
  virtual void HandleSaveOp(const PaintOp* op) {}
  virtual void HandleRestoreOp() {}

  void abort_analysis() { analysis_aborted_ = true; }
  const SkCanvas& state_canvas() { return *state_canvas_; }

 private:
  void CreateStateForAnalysis(const gfx::Rect& rect);

  bool analysis_aborted_ = false;
  const PaintOpBuffer* paint_op_buffer_ = nullptr;
  base::Optional<SkNoDrawCanvas> state_canvas_;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_OP_BUFFER_ANALYZER_H_
