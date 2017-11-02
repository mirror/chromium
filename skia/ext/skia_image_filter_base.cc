// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/ext/skia_image_filter_base.h"

#include "base/logging.h"
#include "third_party/skia/include/core/SkColorSpaceXformCanvas.h"
#include "third_party/skia/src/core/SkReadBuffer.h"
#include "third_party/skia/src/core/SkSpecialImage.h"
#include "third_party/skia/src/core/SkSpecialSurface.h"
#include "third_party/skia/src/core/SkColorSpaceXformer.h"

namespace skia {

SkImageFilterBase::~SkImageFilterBase() = default;

SkImageFilterBase::SkImageFilterBase()
  : SkImageFilter(nullptr, 0, nullptr) {}

SkImageFilterBase::SkImageFilterBase(const SkRect& bounds)
  : SkImageFilter(nullptr, 0, nullptr),
    bounds_(bounds) {}

bool SkImageFilterBase::InitFromBuffer(SkReadBuffer& buffer) {
  if (!buffer.readBool())
    return false;

  buffer.readRect(&bounds_);
  auto data = buffer.readByteArrayAsData();
  if (!this->Deserialize(data.get(), nullptr))
    return false;

  return true;
}

void SkImageFilterBase::flatten(SkWriteBuffer& buffer) const {
  // TODO(khushalsagar): Pass ctx.
  auto data = this->Serialize(nullptr);
  if (!data) {
    buffer.writeBool(false);
    return;
  }

  buffer.writeBool(true);
  buffer.writeRect(bounds_);
  // TODO(khushalsagar): No colorspace?
  buffer.writeDataAsByteArray(data.get());
}

sk_sp<SkSpecialImage> SkImageFilterBase::onFilterImage(
    SkSpecialImage* source, const Context& ctx, SkIPoint* offset) const {
  SkRect floatBounds;
  ctx.ctm().mapRect(&floatBounds, bounds_);
  SkIRect bounds = floatBounds.roundOut();
  if (!bounds.intersect(ctx.clipBounds())) {
    return nullptr;
  }
  DCHECK(!bounds.isEmpty());

  sk_sp<SkSpecialSurface> surf(
      source->makeSurface(ctx.outputProperties(), bounds.size()));
  if (!surf) {
    return nullptr;
  }

  SkCanvas* canvas = surf->getCanvas();
  DCHECK(canvas);
  canvas->clear(0x0);

  std::unique_ptr<SkCanvas> xformCanvas = nullptr;
  if (color_space_) {
    // Only non-null in the case where onMakeColorSpace() was called.  This
    // instructs us to do the color space xform on playback.
    xformCanvas = SkCreateColorSpaceXformCanvas(canvas, color_space_);
    canvas = xformCanvas.get();
  }
  canvas->translate(-SkIntToScalar(bounds.fLeft), -SkIntToScalar(bounds.fTop));
  canvas->concat(ctx.ctm());
  this->Draw(canvas);

  offset->fX = bounds.fLeft;
  offset->fY = bounds.fTop;
  return surf->makeImageSnapshot();
}

sk_sp<SkImageFilter> SkImageFilterBase::onMakeColorSpace(
    SkColorSpaceXformer* xformer) const {
  sk_sp<SkColorSpace> dst_cs = xformer->dst();
  if (SkColorSpace::Equals(dst_cs.get(), color_space_.get())) {
    return this->refMe();
  }

  auto transformed_filter = this->Clone();
  transformed_filter->color_space_ = std::move(dst_cs);
  return transformed_filter;
}

}  // namespace skia
