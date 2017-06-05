// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/android_overlay_provider_impl.h"

#include "jni/AndroidOverlayProviderImpl_jni.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;

namespace content {

// static
AndroidOverlayProvider* AndroidOverlayProvider::GetInstance() {
  static AndroidOverlayProvider* instance = nullptr;
  if (!instance) {
    JNIEnv* env = AttachCurrentThread();
    ScopedJavaLocalRef<jobject> obj =
        Java_AndroidOverlayProviderImpl_createImpl(env);
    instance = new AndroidOverlayProviderImpl(obj);
  }

  return instance;
}

AndroidOverlayProviderImpl::AndroidOverlayProviderImpl(
    const ScopedJavaLocalRef<jobject>& obj) {
  JNIEnv* env = AttachCurrentThread();
  obj_ = JavaObjectWeakGlobalRef(env, obj);
}

AndroidOverlayProviderImpl::~AndroidOverlayProviderImpl() {}

bool AndroidOverlayProviderImpl::AreOverlaysSupported() const {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = obj_.get(env);
  if (obj.is_null())
    return false;

  return Java_AndroidOverlayProviderImpl_areOverlaysSupported(env, obj.obj());
}

}  // namespace content
