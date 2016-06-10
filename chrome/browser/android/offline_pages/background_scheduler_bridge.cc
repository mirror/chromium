// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/callback_android.h"
#include "base/android/scoped_java_ref.h"
#include "chrome/browser/android/offline_pages/background_scheduler_bridge.h"
#include "chrome/browser/android/offline_pages/offline_page_model_factory.h"
#include "chrome/browser/android/offline_pages/request_coordinator_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/offline_pages/background/request_coordinator.h"
#include "jni/BackgroundSchedulerBridge_jni.h"

using base::android::ScopedJavaGlobalRef;

namespace offline_pages {
namespace android {

namespace {

// C++ callback that delegates to Java callback.
void ProcessingDoneCallback(
    const ScopedJavaGlobalRef<jobject>& j_callback_obj, bool result) {
  base::android::RunCallbackAndroid(j_callback_obj, result);
}

}  // namespace

// JNI call to start request processing.
static jboolean StartProcessing(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jobject>& j_callback_obj) {
  ScopedJavaGlobalRef<jobject> j_callback_ref;
  j_callback_ref.Reset(env, j_callback_obj);

  // Lookup/create RequestCoordinator KeyedService and call StartProcessing on
  // it with bound j_callback_obj.
  Profile* profile = ProfileManager::GetLastUsedProfile();
  RequestCoordinator* coordinator =
      RequestCoordinatorFactory::GetInstance()->
      GetForBrowserContext(profile);
  DVLOG(2) << "resource_coordinator: " << coordinator;
  coordinator->StartProcessing(
      base::Bind(&ProcessingDoneCallback, j_callback_ref));


  base::Bind(&ProcessingDoneCallback, j_callback_ref);
  // TODO(dougarnett): lookup/create RequestCoordinator KeyedService
  // and call StartProcessing on it with bound j_callback_obj.
  return false;
}

void BackgroundSchedulerBridge::Schedule(
    const TriggerCondition& trigger_condition) {
  JNIEnv* env = base::android::AttachCurrentThread();
  // TODO(dougarnett): pass trigger_condition.
  Java_BackgroundSchedulerBridge_schedule(env);
}

void BackgroundSchedulerBridge::Unschedule() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BackgroundSchedulerBridge_unschedule(env);
}

bool RegisterBackgroundSchedulerBridge(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace android
}  // namespace offline_pages
