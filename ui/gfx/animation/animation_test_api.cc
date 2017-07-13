// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/animation/animation_test_api.h"

#include "base/memory/ptr_util.h"

namespace gfx {
namespace test {

std::unique_ptr<base::AutoReset<Animation::RichAnimationRenderMode>>
AnimationTestApi::SetRichAnimationRenderMode(
    Animation::RichAnimationRenderMode mode) {
  DCHECK(Animation::rich_animation_rendering_mode_ ==
         Animation::RichAnimationRenderMode::PLATFORM);
  return base::MakeUnique<base::AutoReset<Animation::RichAnimationRenderMode>>(
      &Animation::rich_animation_rendering_mode_, mode);
}

}  // namespace test
}  // namespace gfx
