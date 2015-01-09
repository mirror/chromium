// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/drawing_display_item.h"

#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkDrawPictureCallback.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/utils/SkPictureUtils.h"

namespace cc {

DrawingDisplayItem::DrawingDisplayItem(skia::RefPtr<SkPicture> picture,
                                       gfx::PointF location)
    : picture_(picture), location_(location) {
}

DrawingDisplayItem::~DrawingDisplayItem() {
}

void DrawingDisplayItem::Raster(SkCanvas* canvas,
                                SkDrawPictureCallback* callback) const {
  canvas->save();
  canvas->translate(location_.x(), location_.y());
  if (callback)
    picture_->playback(canvas, callback);
  else
    canvas->drawPicture(picture_.get());
  canvas->restore();
}

void DrawingDisplayItem::RasterForTracing(SkCanvas* canvas) const {
  canvas->save();
  canvas->translate(location_.x(), location_.y());
  // The picture debugger in about:tracing doesn't drill down into |drawPicture|
  // operations. Calling |playback()| rather than |drawPicture()| causes the
  // skia operations in |picture_| to appear individually in the picture
  // produced for tracing rather than being hidden inside a drawPicture
  // operation.
  picture_->playback(canvas);
  canvas->restore();
}

bool DrawingDisplayItem::IsSuitableForGpuRasterization() const {
  return picture_->suitableForGpuRasterization(NULL);
}

int DrawingDisplayItem::ApproximateOpCount() const {
  return picture_->approximateOpCount() + sizeof(gfx::PointF);
}

size_t DrawingDisplayItem::PictureMemoryUsage() const {
  DCHECK(picture_);
  return SkPictureUtils::ApproximateBytesUsed(picture_.get());
}

}  // namespace cc
