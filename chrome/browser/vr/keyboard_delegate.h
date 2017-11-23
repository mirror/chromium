// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_KEYBOARD_DELEGATE_H_
#define CHROME_BROWSER_VR_KEYBOARD_DELEGATE_H_

#include "base/memory/weak_ptr.h"

namespace gfx {
class Point3F;
class Transform;
class Vector3dF;
}  // namespace gfx

namespace vr {

struct CameraModel;

class KeyboardDelegate {
 public:
  virtual ~KeyboardDelegate() {}
  virtual void ShowKeyboard() = 0;
  virtual void HideKeyboard() = 0;
  virtual void SetTransform(gfx::Transform) = 0;
  virtual bool HitTest(const gfx::Point3F& ray_origin,
                       const gfx::Point3F& ray_target,
                       gfx::Point3F* hit_position) = 0;

  virtual void OnBeginFrame() {}
  virtual void Draw(const CameraModel& model) = 0;

  // virtual base::WeakPtr<KeyboardRenderer> GetKeyboardRenderer() = 0;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_KEYBOARD_DELEGATE_H_
