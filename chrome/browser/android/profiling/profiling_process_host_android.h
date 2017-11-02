// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_PROFILING_PROCESS_HOST_ANDROID_H_
#define CHROME_BROWSER_ANDROID_PROFILING_PROCESS_HOST_ANDROID_H_

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

// This class implements the native methods of ProfilingProcessHostAndroid.java,
// and acts as a bridge to ProfilingProcessHost. Note that this class is only
// used for testing.
class ProfilingProcessHostAndroid {
 public:
  ProfilingProcessHostAndroid(JNIEnv* env, jobject obj);
  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  void StartProfilingBrowserProcess(
                    JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj);

  void TakeTrace(
                    JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj);

 private:
  ~ProfilingProcessHostAndroid();

  void OnTraceFinished();

  JavaObjectWeakGlobalRef weak_java_object_;
  base::WeakPtrFactory<ProfilingProcessHostAndroid> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProfilingProcessHostAndroid);
};

#endif  // CHROME_BROWSER_ANDROID_PROFILING_PROCESS_HOST_ANDROID_H_
