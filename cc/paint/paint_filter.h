// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_FILTER_H_
#define CC_PAINT_PAINT_FILTER_H_

#include "cc/paint/paint_export.h"
#include "cc/paint/paint_record.h"
#include "skia/ext/skia_image_filter_base.h"

namespace cc {

class CC_PAINT_EXPORT PaintFilter : public skia::SkImageFilterBase {
 public:
  // TODO(khushalsagar): We can likely have this class cover PaintImage and
  // PaintFlags too.
  PaintFilter(sk_sp<PaintRecord> record, SkRect bounds);
  ~PaintFilter() override;

  SK_TO_STRING_OVERRIDE()
  SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(PaintFilter);
  SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP()

 protected:
  void Draw(SkCanvas* canvas) const override;
  sk_sp<SkData> Serialize(void* ctx) const override;
  bool Deserialize(const SkData* data, void* ctx) override;
  sk_sp<skia::SkImageFilterBase> Clone() const override;

 private:
  PaintFilter();
  sk_sp<PaintRecord> record_;

  DISALLOW_COPY_AND_ASSIGN(PaintFilter);
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_FILTER_H_
