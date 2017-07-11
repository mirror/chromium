// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_COMMON_IMAGE_CURSORS_HOLDER_H_
#define SERVICES_UI_COMMON_IMAGE_CURSORS_HOLDER_H_

#include <memory>

#include "base/memory/weak_ptr.h"

namespace ui {

class ImageCursors;

// Helper class wrapping an ImageCursors object.
class ImageCursorsHolder {
 public:
  ImageCursorsHolder();
  virtual ~ImageCursorsHolder();

  ImageCursors* GetImageCursors();
  void SetImageCursors(std::unique_ptr<ImageCursors> image_cursors);
  base::WeakPtr<ImageCursorsHolder> GetWeakPtr();

 private:
  std::unique_ptr<ImageCursors> image_cursors_;
  base::WeakPtrFactory<ImageCursorsHolder> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ImageCursorsHolder);
};

}  // namespace ui

#endif  // SERVICES_UI_COMMON_IMAGE_CURSORS_HOLDER_H_
