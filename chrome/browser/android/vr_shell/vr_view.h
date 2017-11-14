// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_SHELL_VR_VIEW_H_
#define CHROME_BROWSER_ANDROID_VR_SHELL_VR_VIEW_H_

#include <jni.h>

#include <map>
#include <memory>

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/callback.h"
#include "base/cancelable_callback.h"
#include "base/macros.h"
#include "chrome/browser/vr/content_input_delegate.h"
#include "ui/gfx/geometry/quaternion.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gfx/native_widget_types.h"

using base::android::JavaParamRef;

namespace vr_shell {
class VrView : public vr::ContentInputDelegate {
 public:
  VrView(JNIEnv* env, jobject obj, jlong native_vr_shell_delegate);
  ~VrView() override;

  void CreateVrView(JNIEnv* env, jobject obj);

 private:
  base::android::ScopedJavaGlobalRef<jobject> vr_view_;
  jlong native_vr_shell_delegate_;

  // vr::ContentInputDelegate.
  void OnContentEnter(const gfx::PointF& normalized_hit_point) override;
  void OnContentLeave() override;
  void OnContentMove(const gfx::PointF& normalized_hit_point) override;
  void OnContentDown(const gfx::PointF& normalized_hit_point) override;
  void OnContentUp(const gfx::PointF& normalized_hit_point) override;
  void OnContentFlingStart(std::unique_ptr<blink::WebGestureEvent> gesture,
                           const gfx::PointF& normalized_hit_point) override;
  void OnContentFlingCancel(std::unique_ptr<blink::WebGestureEvent> gesture,
                            const gfx::PointF& normalized_hit_point) override;
  void OnContentScrollBegin(std::unique_ptr<blink::WebGestureEvent> gesture,
                            const gfx::PointF& normalized_hit_point) override;
  void OnContentScrollUpdate(std::unique_ptr<blink::WebGestureEvent> gesture,
                             const gfx::PointF& normalized_hit_point) override;
  void OnContentScrollEnd(std::unique_ptr<blink::WebGestureEvent> gesture,
                          const gfx::PointF& normalized_hit_point) override;
  base::android::ScopedJavaGlobalRef<jobject> java_vr_view_;
  DISALLOW_COPY_AND_ASSIGN(VrView);
};

}  //  namespace vr_shell

#endif  // CHROME_BROWSER_ANDROID_VR_SHELL_VR_VIEW_H_
