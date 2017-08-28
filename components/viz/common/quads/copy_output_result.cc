// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/quads/copy_output_result.h"

#include "base/logging.h"

namespace viz {

CopyOutputResult::CopyOutputResult(Format format, const gfx::Rect& rect)
    : format_(format), rect_(rect) {
  DCHECK(format_ == Format::RGBA_BITMAP || format_ == Format::RGBA_TEXTURE ||
         format_ == Format::I420_PLANES);
  DCHECK((format_ != Format::I420_PLANES) ||
         ((rect.x() % 2 == 0) && (rect.y() % 2 == 0) &&
          (rect.width() % 2 == 0) && (rect.height() % 2 == 0)));
}

CopyOutputResult::~CopyOutputResult() = default;

bool CopyOutputResult::IsEmpty() const {
  if (rect_.IsEmpty())
    return true;
  switch (format_) {
    case Format::RGBA_BITMAP:
      return false;
    case Format::I420_PLANES:
      return GetYPlane() == nullptr;
    case Format::RGBA_TEXTURE:
      if (auto* mailbox = GetTextureMailbox())
        return !mailbox->IsTexture();
      else
        return true;
  }
  NOTREACHED();
  return true;
}

const SkBitmap& CopyOutputResult::AsSkBitmap() const {
  if (!lazy_bitmap_) {
    lazy_bitmap_.reset(new SkBitmap);
    if (!rect_.IsEmpty())
      PopulateBitmap(lazy_bitmap_.get());
  }
  return *lazy_bitmap_;
}

const TextureMailbox* CopyOutputResult::GetTextureMailbox() const {
  return nullptr;
}

std::unique_ptr<SingleReleaseCallback>
CopyOutputResult::TakeTextureOwnership() {
  return nullptr;
}

const uint8_t* CopyOutputResult::GetYPlane() const {
  DCHECK_EQ(format_, Format::I420_PLANES);
  return nullptr;
}

const uint8_t* CopyOutputResult::GetUPlane() const {
  DCHECK_EQ(format_, Format::I420_PLANES);
  return nullptr;
}

const uint8_t* CopyOutputResult::GetVPlane() const {
  DCHECK_EQ(format_, Format::I420_PLANES);
  return nullptr;
}

size_t CopyOutputResult::GetYPlaneBytesPerRow() const {
  DCHECK_EQ(format_, Format::I420_PLANES);
  return 0;
}

size_t CopyOutputResult::GetUPlaneBytesPerRow() const {
  DCHECK_EQ(format_, Format::I420_PLANES);
  return 0;
}

size_t CopyOutputResult::GetVPlaneBytesPerRow() const {
  DCHECK_EQ(format_, Format::I420_PLANES);
  return 0;
}

void CopyOutputResult::PopulateBitmap(SkBitmap* bitmap) const {
  DCHECK(bitmap);
  switch (format_) {
    case Format::RGBA_BITMAP:
    case Format::I420_PLANES:
      bitmap->allocPixels(SkImageInfo::MakeN32Premul(
          rect_.width(), rect_.height(), SkColorSpace::MakeSRGB()));
      bitmap->eraseColor(SK_ColorBLACK);
      bitmap->setImmutable();
      break;
    case Format::RGBA_TEXTURE:
      // Leave |bitmap| in "null" state.
      break;
  }
}

CopyOutputSkBitmapResult::CopyOutputSkBitmapResult(const gfx::Rect& rect,
                                                   const SkBitmap& bitmap)
    : CopyOutputResult(Format::RGBA_BITMAP, rect), bitmap_(bitmap) {
  DCHECK(rect.IsEmpty() || bitmap_.readyToDraw());
}

CopyOutputSkBitmapResult::~CopyOutputSkBitmapResult() = default;

void CopyOutputSkBitmapResult::PopulateBitmap(SkBitmap* dest) const {
  // If the source bitmap is already in the "native optimized" format, make a
  // light-weight memory reference copy. Otherwise, make a full bitmap copy with
  // format conversion.
  const SkImageInfo image_info = SkImageInfo::MakeN32Premul(
      rect().width(), rect().height(), bitmap_.refColorSpace());
  if (bitmap_.info() == image_info && bitmap_.readyToDraw()) {
    *dest = bitmap_;
  } else {
    dest->allocPixels(image_info);
    dest->eraseColor(SK_ColorBLACK);
    SkPixmap src_pixmap;
    if (bitmap_.peekPixels(&src_pixmap)) {
      // Note: writePixels() can fail, but then the bitmap will be left with
      // solid black due to the eraseColor() call above.
      dest->writePixels(src_pixmap);
    }
  }
  dest->setImmutable();
}

CopyOutputTextureResult::CopyOutputTextureResult(
    const gfx::Rect& rect,
    const TextureMailbox& texture_mailbox,
    std::unique_ptr<SingleReleaseCallback> release_callback)
    : CopyOutputResult(Format::RGBA_TEXTURE, rect),
      texture_mailbox_(texture_mailbox),
      release_callback_(std::move(release_callback)) {
  DCHECK(rect.IsEmpty() || texture_mailbox_.IsTexture());
  DCHECK(release_callback_ || !texture_mailbox_.IsTexture());
}

CopyOutputTextureResult::~CopyOutputTextureResult() {
  if (release_callback_)
    release_callback_->Run(gpu::SyncToken(), false);
}

const TextureMailbox* CopyOutputTextureResult::GetTextureMailbox() const {
  return &texture_mailbox_;
}

std::unique_ptr<SingleReleaseCallback>
CopyOutputTextureResult::TakeTextureOwnership() {
  texture_mailbox_ = TextureMailbox();
  return std::move(release_callback_);
}

}  // namespace viz
