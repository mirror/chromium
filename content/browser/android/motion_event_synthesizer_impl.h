// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_MOTION_EVENT_SYNTHESIZER_IMPL_H_
#define CONTENT_BROWSER_ANDROID_MOTION_EVENT_SYNTHESIZER_IMPL_H_

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "content/public/browser/android/motion_event_synthesizer.h"

namespace ui {
class ViewAndroid;
}

namespace content {

class MotionEventSynthesizerImpl : public MotionEventSynthesizer {
 public:
  MotionEventSynthesizerImpl(Helper* helper, ui::ViewAndroid* view);
  ~MotionEventSynthesizerImpl() override;

  void SetPointer(int index, int x, int y, int id) override;
  void SetScrollDeltas(int x, int y, int dx, int dy) override;
  void Inject(MotionEventAction action,
              int pointer_count,
              int64_t time_ms) override;

 private:
  base::android::ScopedJavaLocalRef<jobject> GetJavaObject();

  // Used to do the conversion to physical pixel before passing paramers
  // to Java layer.
  float scale_factor_;
  Helper* const helper_;  // not owned by this class.

  DISALLOW_COPY_AND_ASSIGN(MotionEventSynthesizerImpl);
};

bool RegisterMotionEventSynthesizer(JNIEnv* env);

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_MOTION_EVENT_SYNTHESIZER_IMPL_H_
