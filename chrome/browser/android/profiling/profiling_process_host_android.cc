// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/profiling/profiling_process_host_android.h"

#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/profiling_host/profiling_process_host.h"
#include "content/public/common/service_manager_connection.h"
// #include "base/android/jni_android.h"
// #include "base/android/jni_string.h"
// #include "base/json/json_writer.h"
// #include "base/logging.h"
// #include "base/trace_event/trace_event.h"
// #include "content/public/browser/tracing_controller.h"
#include "jni/ProfilingProcessHostAndroid_jni.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

static jlong Init(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  ProfilingProcessHostAndroid* profiler = new ProfilingProcessHostAndroid(env, obj);
  return reinterpret_cast<intptr_t>(profiler);
}

ProfilingProcessHostAndroid::ProfilingProcessHostAndroid(JNIEnv* env, jobject obj)
    : weak_java_object_(env, obj),
      weak_factory_(this) {}

ProfilingProcessHostAndroid::~ProfilingProcessHostAndroid() {}

void ProfilingProcessHostAndroid::Destroy(JNIEnv* env,
                                       const JavaParamRef<jobject>& obj) {
  delete this;
}

  void ProfilingProcessHostAndroid::StartProfilingBrowserProcess(
                    JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj) {
  DCHECK(content::ServiceManagerConnection::GetForProcess());
  profiling::ProfilingProcessHost::Start(content::ServiceManagerConnection::GetForProcess(), profiling::ProfilingProcessHost::Mode::kMinimal);
}

void ProfilingProcessHostAndroid::TakeTrace(
                  JNIEnv* env,
                  const base::android::JavaParamRef<jobject>& obj) {
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(FROM_HERE, base::Bind(&ProfilingProcessHostAndroid::OnTraceFinished, weak_factory_.GetWeakPtr()), base::TimeDelta::FromSeconds(1));
}

void ProfilingProcessHostAndroid::OnTraceFinished() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = weak_java_object_.get(env);
  if (obj.obj())
    Java_ProfilingProcessHostAndroid_onTraceFinished(env, obj, base::android::ConvertUTF8ToJavaString(env, std::string("placeholder")));
}
