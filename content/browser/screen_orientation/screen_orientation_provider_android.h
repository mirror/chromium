// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_PROVIDER_ANDROID_H_
#define CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_PROVIDER_ANDROID_H_

#include <jni.h>

#include "base/compiler_specific.h"
#include "content/public/browser/screen_orientation_provider.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class ScreenOrientationDispatcherHost;
class WebContentsImpl;

class ScreenOrientationProviderAndroid : public ScreenOrientationProvider,
                                         public WebContentsObserver {
 public:
  explicit ScreenOrientationProviderAndroid(
      ScreenOrientationDispatcherHost* dispatcher,
      WebContents* web_contents);

  static bool Register(JNIEnv* env);

  // ScreenOrientationProvider
  virtual void LockOrientation(int request_id,
                               blink::WebScreenOrientationLockType) OVERRIDE;
  virtual void UnlockOrientation() OVERRIDE;
  virtual void OnOrientationChange() OVERRIDE;

  // WebContentsObserver
  virtual void DidToggleFullscreenModeForTab(bool entered_fullscreen) OVERRIDE;

  // Ask the ScreenOrientationListener (Java) to start accurately listening to
  // the screen orientation. It keep track of the number of start request if it
  // is already running an accurate listening.
  static void StartAccurateListening();

  // Ask the ScreenOrientationListener (Java) to stop accurately listening to
  // the screen orientation. It will actually stop only if the number of stop
  // requests matches the number of start requests.
  static void StopAccurateListening();

 private:
  WebContentsImpl* web_contents_impl();

  // Whether the passed |lock| matches the current orientation. In other words,
  // whether the orientation will need to change to match the |lock|.
  bool LockMatchesCurrentOrientation(blink::WebScreenOrientationLockType lock);

  // Returns the lock type that should be associated with 'natural' lock.
  // Returns WebScreenOrientationLockDefault if the natural lock type can't be
  // found.
  blink::WebScreenOrientationLockType GetNaturalLockType() const;

  virtual ~ScreenOrientationProviderAndroid();

  // ScreenOrientationDispatcherHost owns ScreenOrientationProvider so
  // dispatcher_ should not point to an invalid memory.
  ScreenOrientationDispatcherHost* dispatcher_;

  // Whether the ScreenOrientationProvider currently has a lock applied.
  bool lock_applied_;

  struct LockInformation {
    LockInformation(int request_id, blink::WebScreenOrientationLockType lock);
    int request_id;
    blink::WebScreenOrientationLockType lock;
  };
  LockInformation* pending_lock_;

  DISALLOW_COPY_AND_ASSIGN(ScreenOrientationProviderAndroid);
};

} // namespace content

#endif // CONTENT_BROWSER_SCREEN_ORIENTATION_SCREEN_ORIENTATION_PROVIDER_ANDROID_H_
