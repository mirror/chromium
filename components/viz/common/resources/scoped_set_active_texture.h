// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_SCOPED_SET_ACTIVE_TEXTURE_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_SCOPED_SET_ACTIVE_TEXTURE_H_

#include "base/macros.h"
#include "components/viz/common/viz_common_export.h"
#include "third_party/khronos/GLES2/gl2.h"

namespace gpu {
namespace gles2 {
class GLES2Interface;
}
}  // namespace gpu

namespace viz {

class VIZ_COMMON_EXPORT ScopedSetActiveTexture {
 public:
  explicit ScopedSetActiveTexture(gpu::gles2::GLES2Interface* gl, GLenum unit);
  ~ScopedSetActiveTexture();

 private:
  gpu::gles2::GLES2Interface* gl_;
  GLenum unit_;
  DISALLOW_COPY_AND_ASSIGN(ScopedSetActiveTexture);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_SCOPED_SET_ACTIVE_TEXTURE_H_
