// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_GESTURE_LISTENER_MANAGER_ANDROID_H_
#define CONTENT_BROWSER_ANDROID_GESTURE_LISTENER_MANAGER_ANDROID_H_

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/input_event_ack_state.h"

namespace blink {
class WebGestureEvent;
}

namespace content {

// Native class for GestureListenerManagerImpl.
// Owned by WebContents through WebContentsObserver and destroyed together.
class GestureListenerManagerAndroid final : WebContentsObserver {
 public:
  GestureListenerManagerAndroid(JNIEnv* env,
                                const base::android::JavaParamRef<jobject>& obj,
                                WebContentsImpl* web_contents);

  void GestureEventAck(const blink::WebGestureEvent& event,
                       InputEventAckState ack_result);

  // WebContentsObserver
  void WebContentsDestroyed() override;

 private:
  // A weak reference to the Java GestureListenerManager object.
  JavaObjectWeakGlobalRef java_ref_;

  DISALLOW_COPY_AND_ASSIGN(GestureListenerManagerAndroid);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_GESTURE_LISTENER_MANAGER_ANDROID_H_
