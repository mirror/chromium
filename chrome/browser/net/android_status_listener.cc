// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/android_status_listener.h"

AndroidStatusListener::~AndroidStatusListener() {
  base::AutoLock lock(callback_lock_);
}

AndroidStatusListener::AndroidStatusListener() {
  app_status_listener_.reset(new base::android::ApplicationStatusListener(
      base::Bind(&::AndroidStatusListener::OnApplicationStateChange,
                 base::Unretained(this))));
}

void AndroidStatusListener::SetApplicationStoppedCallback(
    base::RepeatingClosure callback) {
  base::AutoLock lock(callback_lock_);
  stopped_callback_ = callback;
}

void AndroidStatusListener::OnApplicationStateChange(
    base::android::ApplicationState state) {
  base::AutoLock lock(callback_lock_);
  if (state == base::android::APPLICATION_STATE_HAS_STOPPED_ACTIVITIES &&
      !stopped_callback_.is_null()) {
    stopped_callback_.Run();
  }
}
