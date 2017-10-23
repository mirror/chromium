// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/background_sync_launcher_android.h"

#include "content/public/browser/browser_thread.h"
#include "jni/BackgroundSyncLauncher_jni.h"

using content::BrowserThread;

namespace {
base::LazyInstance<BackgroundSyncLauncherAndroid>::DestructorAtExit
    g_background_sync_launcher = LAZY_INSTANCE_INITIALIZER;
}  // namespace

// static
BackgroundSyncLauncherAndroid* BackgroundSyncLauncherAndroid::Get() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  return g_background_sync_launcher.Pointer();
}

// static
void BackgroundSyncLauncherAndroid::LaunchBrowserIfStopped(
    bool launch_when_next_online,
    int64_t min_delay_ms) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Get()->LaunchBrowserIfStoppedImpl(launch_when_next_online, min_delay_ms);
}

void BackgroundSyncLauncherAndroid::LaunchBrowserIfStoppedImpl(
    bool launch_when_next_online,
    int64_t min_delay_ms) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BackgroundSyncLauncher_launchBrowserIfStopped(
      env, java_launcher_, launch_when_next_online, min_delay_ms);
}

// static
bool BackgroundSyncLauncherAndroid::ShouldDisableBackgroundSync() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return Java_BackgroundSyncLauncher_shouldDisableBackgroundSync(
      base::android::AttachCurrentThread());
}

BackgroundSyncLauncherAndroid::BackgroundSyncLauncherAndroid() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  JNIEnv* env = base::android::AttachCurrentThread();
  java_launcher_.Reset(Java_BackgroundSyncLauncher_create(env));
}

BackgroundSyncLauncherAndroid::~BackgroundSyncLauncherAndroid() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BackgroundSyncLauncher_destroy(env, java_launcher_);
}
