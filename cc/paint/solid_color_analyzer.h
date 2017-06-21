// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_SOLID_COLOR_ANALYZER_H_
#define CC_PAINT_SOLID_COLOR_ANALYZER_H_

#include "cc/paint/paint_export.h"
#include "cc/paint/paint_flags.h"
#include "cc/paint/paint_op_buffer_analyzer.h"
#include "third_party/skia/include/core/SkColor.h"

namespace cc {
struct PaintOp;

class CC_PAINT_EXPORT SolidColorAnalyzer
    : public PaintOpBufferAnalyzer<SolidColorAnalyzer> {
 public:
  explicit SolidColorAnalyzer(const PaintOpBuffer* paint_op_buffer);
  ~SolidColorAnalyzer() override;

  bool GetColorIfSolid(SkColor* color) const;

  // PaintOpBufferAnalyzer functions.
  void HandleDrawOp(const PaintOp* op);
  void HandleSaveOp(const PaintOp* op);
  void HandleStateOp(const PaintOp* op);

 private:
  void HandleDrawColor(SkColor color, SkBlendMode blendmode);
  void HandleDrawRect(const SkRect& rect, const PaintFlags& flags);

  bool is_solid_color_ = false;
  bool is_transparent_ = true;
  SkColor color_ = SK_ColorTRANSPARENT;
  int num_ops_ = 0;
};

}  // namespace cc

#endif  // CC_PAINT_SOLID_COLOR_ANALYZER_H_
