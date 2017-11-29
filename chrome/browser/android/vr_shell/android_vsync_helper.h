// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_SHELL_ANDROID_VSYNC_HELPER_H_
#define CHROME_BROWSER_ANDROID_VR_SHELL_ANDROID_VSYNC_HELPER_H_

#include <jni.h>

#include "base/android/jni_weak_ref.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/browser/vr/fps_meter.h"

namespace vr {
class HeuristicVSyncAverage;
}

namespace vr_shell {

class AndroidVSyncHelper {
 public:
  AndroidVSyncHelper();
  ~AndroidVSyncHelper();
  void OnVSync(JNIEnv* env,
               const base::android::JavaParamRef<jobject>& obj,
               jlong time_nanos);

  void RequestVSync(const base::Callback<void(base::TimeTicks)>& callback);
  void CancelVSyncRequest();
  // The last interval may be a multiple of the actual refresh interval, use
  // with care.
  base::TimeDelta LastVSyncInterval() { return last_interval_; }
  // Heuristically estimated average VSync interval.
  base::TimeDelta AverageVSyncInterval();

 private:
  base::TimeTicks last_vsync_;
  base::TimeDelta last_interval_;
  std::unique_ptr<vr::HeuristicVSyncAverage> intervals_;
  base::Callback<void(base::TimeTicks)> callback_;

  base::android::ScopedJavaGlobalRef<jobject> j_object_;

  DISALLOW_COPY_AND_ASSIGN(AndroidVSyncHelper);
};

}  // namespace vr_shell

#endif  // CHROME_BROWSER_ANDROID_VR_SHELL_ANDROID_VSYNC_HELPER_H_
