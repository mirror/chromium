// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_VAAPI_VAAPI_PICTURE_FACTORY_H_
#define MEDIA_GPU_VAAPI_VAAPI_PICTURE_FACTORY_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/ref_counted.h"
#include "media/gpu/vaapi/vaapi_picture.h"
#include "ui/gfx/geometry/size.h"

namespace media {

class VaapiWrapper;

// Factory of platform dependent VaapiPictures.
class VaapiPictureFactory {
 public:
  VaapiPictureFactory();
  ~VaapiPictureFactory();

  // Creates a VaapiPicture of |size| associated with |picture_buffer_id|. If
  // provided, bind it to |texture_id|, as well as to |client_texture_id| using
  // |bind_image_cb|.
  linked_ptr<VaapiPicture> Create(
      const scoped_refptr<VaapiWrapper>& vaapi_wrapper,
      const MakeGLContextCurrentCallback& make_context_current_cb,
      const BindGLImageCallback& bind_image_cb,
      int32_t picture_buffer_id,
      const gfx::Size& size,
      uint32_t texture_id,
      uint32_t client_texture_id);

  // Gets the texture target used to bind EGLImages (either GL_TEXTURE_2D on X11
  // or GL_TEXTURE_EXTERNAL_OES on DRM).
  uint32_t GetGLTextureTarget();

 private:
  DISALLOW_COPY_AND_ASSIGN(VaapiPictureFactory);
};

}  // namespace media

#endif  // MEDIA_GPU_VAAPI_VAAPI_PICTURE_FACTORY_H_
