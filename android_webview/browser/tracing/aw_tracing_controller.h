// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_TRACING_AW_TRACING_CONTROLLER_H_
#define ANDROID_WEBVIEW_BROWSER_TRACING_AW_TRACING_CONTROLLER_H_

#include "base/android/jni_weak_ref.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

#include "content/public/browser/tracing_controller.h"

namespace android_webview {

class AwTracingController {
 public:
  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.android_webview
  // TODO(timvolodine): consider elimintating this enum, see crbug.com/797302.
  enum AwTracingMode {
    RECORD_UNTIL_FULL = 0,
    RECORD_CONTINUOSLY = 1,
    RECORD_AS_MUCH_AS_POSSIBLE = 2,
    RECORD_TO_CONSOLE = 3,
  };

  AwTracingController(JNIEnv* env, jobject obj);

  bool Start(JNIEnv* env,
             const base::android::JavaParamRef<jobject>& obj,
             const base::android::JavaParamRef<jstring>& categories,
             jint mode);
  bool StopAndFlush(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj);
  bool IsTracing(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

 private:
  ~AwTracingController();

  void OnTraceDataReceived(
      std::unique_ptr<const base::DictionaryValue> metadata,
      base::RefCountedString* trace_data);

  JavaObjectWeakGlobalRef weak_java_object_;
  base::WeakPtrFactory<AwTracingController> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AwTracingController);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_TRACING_AW_TRACING_CONTROLLER_H_
