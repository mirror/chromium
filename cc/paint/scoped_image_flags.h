// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_SCOPED_IMAGE_FLAGS_H_
#define CC_PAINT_SCOPED_IMAGE_FLAGS_H_

#include "base/macros.h"
#include "cc/paint/paint_op_buffer.h"

namespace cc {
class ImageProvider;

// A helper class to decode images inside the provided |flags| and provide a
// PaintFlags with the decoded images that can directly be used for
// rasterization.
// This class should only be used if |flags| has any discardable images.
class ScopedImageFlags {
 public:
  ScopedImageFlags(ImageProvider* image_provider,
                   DecodedImageStash* decoded_image_stash,
                   const PaintFlags& flags,
                   const SkMatrix& ctm);
  ~ScopedImageFlags();

  // The usage of these flags should not extend beyond the lifetime of this
  // object.
  PaintFlags* decoded_flags() {
    return decoded_flags_ ? &decoded_flags_.value() : nullptr;
  }

 private:
  void DecodeImageShader(ImageProvider* image_provider,
                         DecodedImageStash* decoded_image_stash,
                         const PaintFlags& flags,
                         const SkMatrix& ctm);

  void DecodeRecordShader(ImageProvider* image_provider,
                          DecodedImageStash* decoded_image_stash,
                          const PaintFlags& flags,
                          const SkMatrix& ctm);

  base::Optional<PaintFlags> decoded_flags_;
  base::Optional<DecodedImageStash> owned_decoded_image_stash_;

  DISALLOW_COPY_AND_ASSIGN(ScopedImageFlags);
};

}  // namespace cc

#endif  // CC_PAINT_SCOPED_IMAGE_FLAGS_H_
