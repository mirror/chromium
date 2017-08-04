// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_ANDROID_STATUS_LISTENER_H_
#define CHROME_BROWSER_NET_ANDROID_STATUS_LISTENER_H_

#include "base/android/application_status_listener.h"
#include "base/synchronization/lock.h"
#include "net/extras/status_listener.h"

namespace chrome_browser_net {

// AndroidStatusListener provides an android-specific implementation of
// net::StatusListener. It listens for the android application entering the
// Stopped state
// (https://developer.android.com/guide/components/activities/activity-lifecycle.html)
// and calls the callback set in SetApplicationStoppedCallback.
class AndroidStatusListener : public net::StatusListener {
 public:
  AndroidStatusListener();
  ~AndroidStatusListener() override;

  void SetApplicationStoppedCallback(base::RepeatingClosure callback) override;

 private:
  void OnApplicationStateChange(base::android::ApplicationState state);

  std::unique_ptr<base::android::ApplicationStatusListener>
      app_status_listener_;

  base::RepeatingClosure stopped_callback_;
};

}  // namespace chrome_browser_net

#endif  // CHROME_BROWSER_NET_ANDROID_STATUS_LISTENER_H_
