// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/stereo_background.h"

#include "base/memory/ptr_util.h"

#include "chrome/grit/vr_shell_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/codec/jpeg_codec.h"

#include "chrome/browser/vr/ui_element_renderer.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkImage.h"
#include "ui/gfx/geometry/rect_f.h"

namespace vr {

StereoBackground::StereoBackground() {
  set_fill(Fill::SELF);
  SetVisible(true);
  set_hit_testable(false);
}

StereoBackground::~StereoBackground() = default;

void StereoBackground::Render(UiElementRenderer* renderer,
                              gfx::Transform view_proj_matrix,
                              bool right_eye) const {
  renderer->DrawStereoBackground(texture_handle_, view_proj_matrix, right_eye,
                                 1.0f);
}

void StereoBackground::Initialize() {
  glGenTextures(1, &texture_handle_);

  base::StringPiece data =
      ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_VR_SHELL_STEREO_BACKGROUND);

  std::unique_ptr<SkBitmap> bitmap = gfx::JPEGCodec::Decode(
      reinterpret_cast<const unsigned char*>(data.data()), data.size());

  DCHECK(bitmap);
  DCHECK(bitmap->colorType() == kRGBA_8888_SkColorType);

  auto image = SkImage::MakeFromBitmap(*bitmap);
  SkPixmap pixmap;
  CHECK(image->peekPixels(&pixmap));

  glBindTexture(GL_TEXTURE_2D, texture_handle_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixmap.width(), pixmap.height(), 0,
               GL_RGBA, GL_UNSIGNED_BYTE, pixmap.addr());
}

}  // namespace vr
