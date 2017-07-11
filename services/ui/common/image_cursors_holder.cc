// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/common/image_cursors_holder.h"

#include "ui/base/cursor/image_cursors.h"

namespace ui {

ImageCursorsHolder::ImageCursorsHolder() : weak_ptr_factory_(this) {}

ImageCursorsHolder::~ImageCursorsHolder() {}

ImageCursors* ImageCursorsHolder::GetImageCursors() {
  DCHECK(image_cursors_);
  return image_cursors_.get();
}

void ImageCursorsHolder::SetImageCursors(
    std::unique_ptr<ImageCursors> image_cursors) {
  DCHECK(!image_cursors_);
  image_cursors_ = std::move(image_cursors);
}

base::WeakPtr<ImageCursorsHolder> ImageCursorsHolder::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace ui
