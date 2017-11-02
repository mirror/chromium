// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_FILTER_H_
#define CC_PAINT_PAINT_FILTER_H_

#include "cc/paint/paint_export.h"
#include "cc/paint/paint_record.h"
#include "third_party/skia/include/core/SkImageFilter.h"

namespace cc {

class CC_PAINT_EXPORT PaintFilter : public SkImageFilter {
 public:
  // TODO(khushalsagar): We can likely have this class cover PaintImage and
  // PaintFlags too.
  static sk_sp<SkImageFilter> Make(sk_sp<PaintRecord> record, SkRect bounds);

  ~PaintFilter() override;

  SK_TO_STRING_OVERRIDE()
  SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(PaintFilter);
  SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP()

 protected:
  void flatten(SkWriteBuffer& buffer) const override;
  sk_sp<SkSpecialImage> onFilterImage(SkSpecialImage* source,
                                      const Context& ctx,
                                      SkIPoint* offset) const override;
  sk_sp<SkImageFilter> onMakeColorSpace(
      SkColorSpaceXformer* xformer) const override;

 private:
  explicit PaintFilter(sk_sp<PaintRecord> record, SkRect bounds);

  sk_sp<PaintRecord> record_;
  SkRect bounds_;
  sk_sp<SkColorSpace> color_space_;

  DISALLOW_COPY_AND_ASSIGN(PaintFilter);
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_FILTER_H_
