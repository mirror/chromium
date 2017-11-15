// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/android/java_handler_thread_helpers.h"

#include <jni.h>

#include "base/android/java_handler_thread.h"
#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/message_loop/message_loop.h"
#include "base/synchronization/waitable_event.h"
#include "jni/JavaHandlerThreadHelpers_jni.h"

namespace base {
namespace android {

namespace {

static RepeatingCallback<void(bool)> java_task_run_callback;

}  // namespace

static void OnJavaTaskRun(JNIEnv* env, const JavaParamRef<jclass>& clazz) {
  java_task_run_callback.Run(true);
}

// static
std::unique_ptr<JavaHandlerThread> JavaHandlerThreadHelpers::CreateJavaFirst() {
  return std::make_unique<JavaHandlerThread>(
      Java_JavaHandlerThreadHelpers_testAndGetJavaHandlerThread(
          base::android::AttachCurrentThread()));
}

// static
void JavaHandlerThreadHelpers::ThrowExceptionAndAbort(WaitableEvent* event) {
  JNIEnv* env = AttachCurrentThread();
  Java_JavaHandlerThreadHelpers_throwException(env);
  DCHECK(HasException(env));
  base::MessageLoopForUI::current()->Abort();
  event->Signal();
}

// static
bool JavaHandlerThreadHelpers::IsExceptionTestException(
    ScopedJavaLocalRef<jthrowable> exception) {
  JNIEnv* env = AttachCurrentThread();
  return Java_JavaHandlerThreadHelpers_isExceptionTestException(env, exception);
}

// static
void JavaHandlerThreadHelpers::SetOnJavaTaskRunCallback(
    const RepeatingCallback<void(bool)>& callback) {
  java_task_run_callback = callback;
}

// static
void JavaHandlerThreadHelpers::PostJavaTask() {
  JNIEnv* env = AttachCurrentThread();
  Java_JavaHandlerThreadHelpers_postJavaTask(env);
}

}  // namespace android
}  // namespace base
