// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiling_host/test_android_shim.h"

#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/profiling_host/profiling_process_host.h"
#include "chrome/browser/profiling_host/profiling_test_driver.h"
#include "content/public/common/service_manager_connection.h"
// #include "base/android/jni_android.h"
// #include "base/android/jni_string.h"
// #include "base/json/json_writer.h"
// #include "base/logging.h"
// #include "base/trace_event/trace_event.h"
// #include "content/public/browser/tracing_controller.h"
#include "jni/TestAndroidShim_jni.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

static jlong Init(JNIEnv* env, const JavaParamRef<jobject>& obj) {
	LOG(ERROR) << "TestAndroidShim::Init1";
  TestAndroidShim* profiler = new TestAndroidShim(env, obj);
	LOG(ERROR) << "TestAndroidShim::Init2";
  return reinterpret_cast<intptr_t>(profiler);
}

TestAndroidShim::TestAndroidShim(JNIEnv* env, jobject obj)
    : weak_java_object_(env, obj),
      weak_factory_(this) {}

TestAndroidShim::~TestAndroidShim() {}

void TestAndroidShim::Destroy(JNIEnv* env,
                                       const JavaParamRef<jobject>& obj) {
  delete this;
}

  void TestAndroidShim::StartProfilingBrowserProcess(
                    JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj) {
  profiling::ProfilingTestDriver driver;
  profiling::ProfilingTestDriver::Options options;
  options.mode = profiling::ProfilingProcessHost::Mode::kMinimal;
  options.profiling_already_started = false;
  CHECK(driver.RunTest(options));
}

void TestAndroidShim::TakeTrace(
                  JNIEnv* env,
                  const base::android::JavaParamRef<jobject>& obj) {
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(FROM_HERE, base::Bind(&TestAndroidShim::OnTraceFinished, weak_factory_.GetWeakPtr()), base::TimeDelta::FromSeconds(1));
}

void TestAndroidShim::OnTraceFinished() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = weak_java_object_.get(env);
  if (obj.obj())
    Java_TestAndroidShim_onTraceFinished(env, obj, base::android::ConvertUTF8ToJavaString(env, std::string("placeholder")));
}
