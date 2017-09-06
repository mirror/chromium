// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/quads/copy_output_result.h"

#include "base/logging.h"
#include "components/viz/common/quads/texture_mailbox.h"

namespace viz {

CopyOutputResult::CopyOutputResult() {}

CopyOutputResult::CopyOutputResult(std::unique_ptr<SkBitmap> bitmap,
                                   const gfx::ColorSpace& bitmap_color_space)
    : size_(bitmap->width(), bitmap->height()),
      bitmap_(std::move(bitmap)),
      bitmap_color_space_(bitmap_color_space) {
  DCHECK(bitmap_);
  DCHECK(!bitmap_->colorSpace());
}

CopyOutputResult::CopyOutputResult(
    const gfx::Size& size,
    const TextureMailbox& texture_mailbox,
    std::unique_ptr<SingleReleaseCallback> release_callback)
    : size_(size),
      texture_mailbox_(texture_mailbox),
      release_callback_(std::move(release_callback)) {
  DCHECK(texture_mailbox_.IsTexture());
}

CopyOutputResult::~CopyOutputResult() {
  if (release_callback_)
    release_callback_->Run(gpu::SyncToken(), false);
}

std::unique_ptr<SkBitmap> CopyOutputResult::TakeBitmap() {
  return std::move(bitmap_);
}

void CopyOutputResult::TakeTexture(
    TextureMailbox* texture_mailbox,
    std::unique_ptr<SingleReleaseCallback>* release_callback) {
  *texture_mailbox = texture_mailbox_;
  *release_callback = std::move(release_callback_);

  texture_mailbox_ = TextureMailbox();
}

}  // namespace viz
