// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_filter.h"

#include "third_party/skia/include/core/SkColorSpaceXformCanvas.h"
#include "third_party/skia/src/core/SkSpecialImage.h"
#include "third_party/skia/src/core/SkSpecialSurface.h"
//#include "third_party/skia/src/core/SkColorSpaceXformer.h"

namespace cc {

sk_sp<SkImageFilter> PaintFilter::Make(sk_sp<PaintRecord> record,
                                       SkRect bounds) {
  if (!record)
    return nullptr;
  return sk_sp<PaintFilter>(new PaintFilter(std::move(record), bounds));
}

PaintFilter::~PaintFilter() = default;

void PaintFilter::flatten(SkWriteBuffer& buffer) const {
  // TODO(khushalsagar): SkWriteBuffer needs to provide something through which
  // we can access our caches.
}

sk_sp<SkFlattenable> PaintFilter::CreateProc(SkReadBuffer& buffer) {
  // TODO(khushalsagar): SkReadBuffer needs to provide something through which
  // we can access our caches.
  return nullptr;
}

sk_sp<SkSpecialImage> PaintFilter::onFilterImage(SkSpecialImage* source,
                                                 const Context& ctx,
                                                 SkIPoint* offset) const {
  // TODO(khushalsagar): SkSpecialImage and SkSpecialSurface need to be exported
  // for use here.
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
  record_->Playback(canvas);

  offset->fX = bounds.fLeft;
  offset->fY = bounds.fTop;
  return surf->makeImageSnapshot();
}

sk_sp<SkImageFilter> PaintFilter::onMakeColorSpace(
    SkColorSpaceXformer* xformer) const {
  // TODO(khushalsagar): Doing this requires including SkColorSpaceXformer.h
  // which publicly uses SkTHash, which is private in skia. May be we can change
  // that method to also specify the color space?

  /*sk_sp<SkColorSpace> dst_cs = xformer->dst();
  if (SkColorSpace::Equals(dst_cs.get(), color_space_.get())) {
    return this->refMe();
  }

  auto transformed_filter = sk_sp<PaintFilter>(new PaintFilter(record_,
  bounds_)); transformed_filter->color_space_ = std::move(dst_cs); return
  transformed_filter;*/
  return nullptr;
}

PaintFilter::PaintFilter(sk_sp<PaintRecord> record, SkRect bounds)
    : SkImageFilter(nullptr, 0, nullptr),
      record_(std::move(record)),
      bounds_(bounds) {}

#ifndef SK_IGNORE_TO_STRING
void PaintFilter::toString(SkString* str) const {
  str->appendf("PaintFilter: (");
  str->appendf("bounds: (%f,%f,%f,%f) ", bounds_.fLeft, bounds_.fTop,
               bounds_.fRight, bounds_.fBottom);
  str->append(")");
}
#endif

SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_START(PaintFilter)
SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(PaintFilter)
SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_END

}  // namespace cc
