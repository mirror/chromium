// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_ANIMATION_ANIMATION_TEST_API_H_
#define UI_GFX_ANIMATION_ANIMATION_TEST_API_H_

#include <memory>

#include "base/auto_reset.h"
#include "base/macros.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_export.h"

namespace gfx {
class Animation;

namespace test {

class ANIMATION_EXPORT AnimationTestApi {
 public:
  // Sets the rich animation rendering mode. Allows rich animations to be force
  // enabled/disabled during tests.
  static std::unique_ptr<base::AutoReset<Animation::RichAnimationRenderMode>>
  SetRichAnimationRenderMode(Animation::RichAnimationRenderMode mode);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(AnimationTestApi);
};

}  // namespace test
}  // namespace gfx

#endif  // UI_GFX_ANIMATION_ANIMATION_TEST_API_H_
