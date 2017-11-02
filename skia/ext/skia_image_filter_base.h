// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_EXT_SKIA_IMAGE_FILTER_BASE_H_
#define SKIA_EXT_SKIA_IMAGE_FILTER_BASE_H_

#include "base/macros.h"
#include "third_party/skia/include/core/SkImageFilter.h"

class SkCanvas;
class SkReadBuffer;

namespace skia {

class SK_API SkImageFilterBase : public SkImageFilter {
 public:
  ~SkImageFilterBase() override;

 protected:
  SkImageFilterBase();
  SkImageFilterBase(const SkRect& bounds);

  virtual void Draw(SkCanvas* canvas) const = 0;
  virtual sk_sp<SkData> Serialize(void* ctx) const = 0;
  virtual bool Deserialize(const SkData* data, void* ctx) = 0;
  virtual sk_sp<SkImageFilterBase> Clone() const = 0;

  bool InitFromBuffer(SkReadBuffer& buffer);
  void flatten(SkWriteBuffer& buffer) const final;
  sk_sp<SkSpecialImage> onFilterImage(SkSpecialImage* source,
                                      const Context& ctx,
                                      SkIPoint* offset) const final;
  sk_sp<SkImageFilter> onMakeColorSpace(
      SkColorSpaceXformer* xformer) const final;

  SkRect bounds() const { return bounds_; }

 private:
  SkRect bounds_;
  sk_sp<SkColorSpace> color_space_;

  DISALLOW_COPY_AND_ASSIGN(SkImageFilterBase);
};

}  // namespace

#endif  // SKIA_EXT_SKIA_IMAGE_FILTER_BASE_H_
