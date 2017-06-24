// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_MOTION_EVENT_SYNTHESIZER_IMPL_H_
#define CONTENT_BROWSER_ANDROID_MOTION_EVENT_SYNTHESIZER_IMPL_H_

#include <jni.h>

#include "base/android/jni_weak_ref.h"
#include "base/macros.h"
#include "content/public/browser/android/motion_event_synthesizer.h"

namespace ui {
class ViewAndroid;
}

namespace content {

class MotionEventSynthesizerImpl : public MotionEventSynthesizer {
 public:
  explicit MotionEventSynthesizerImpl(ui::ViewAndroid* view);
  ~MotionEventSynthesizerImpl() override;

  void SetPointer(int index, int x, int y, int id) override;
  void SetScrollDeltas(int x, int y, int dx, int dy) override;
  void Inject(MotionEventAction action,
              int pointer_count,
              int64 time_ms) override;

 private:
  float scale_factor_;
  JavaObjectWeakGlobalRef java_ref_;

  DISALLOW_COPY_AND_ASSIGN(MotionEventSynthesizerImpl);
};

bool RegisterMotionEventSynthesizer(JNIEnv* env);

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_MOTION_EVENT_SYNTHESIZER_IMPL_H_
