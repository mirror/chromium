// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/tracing/aw_tracing_controller.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "content/public/browser/tracing_controller.h"
#include "jni/AwTracingController_jni.h"

using base::android::JavaParamRef;

namespace {

base::android::ScopedJavaLocalRef<jbyteArray> StringToJavaBytes(
    JNIEnv* env,
    const std::string& str) {
  return base::android::ToJavaByteArray(
      env, reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

}  // namespace

namespace android_webview {

static jlong JNI_AwTracingController_Init(JNIEnv* env,
                                          const JavaParamRef<jobject>& obj) {
  AwTracingController* controller = new AwTracingController(env, obj);
  return reinterpret_cast<intptr_t>(controller);
}

AwTracingController::AwTracingController(JNIEnv* env, jobject obj)
    : weak_java_object_(env, obj), weak_factory_(this) {}

AwTracingController::~AwTracingController() {}

bool AwTracingController::Start(JNIEnv* env,
                                const JavaParamRef<jobject>& obj,
                                const JavaParamRef<jstring>& jcategories,
                                jint jmode) {
  std::string categories =
      base::android::ConvertJavaStringToUTF8(env, jcategories);
  base::trace_event::TraceConfig trace_config(
      categories, static_cast<base::trace_event::TraceRecordMode>(jmode));
  return content::TracingController::GetInstance()->StartTracing(
      trace_config, content::TracingController::StartTracingDoneCallback());
}

bool AwTracingController::StopAndFlush(JNIEnv* env,
                                       const JavaParamRef<jobject>& obj) {
  // TODO: replace StringEndpoint to with a custom endpoint for better
  // efficiency.
  return content::TracingController::GetInstance()->StopTracing(
      content::TracingController::CreateStringEndpoint(
          base::Bind(&AwTracingController::OnTraceDataReceived,
                     weak_factory_.GetWeakPtr())));
}

void AwTracingController::OnTraceDataReceived(
    std::unique_ptr<const base::DictionaryValue> metadata,
    base::RefCountedString* trace_data) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jbyteArray> java_trace_data =
      StringToJavaBytes(env, trace_data->data());
  base::android::ScopedJavaLocalRef<jobject> obj = weak_java_object_.get(env);
  if (obj.obj()) {
    Java_AwTracingController_onTraceDataChunkReceived(env, obj,
                                                      java_trace_data);
    Java_AwTracingController_onTraceDataComplete(env, obj);
  }
}

bool AwTracingController::IsTracing(JNIEnv* env,
                                    const JavaParamRef<jobject>& obj) {
  return content::TracingController::GetInstance()->IsTracing();
}

}  // namespace android_webview
