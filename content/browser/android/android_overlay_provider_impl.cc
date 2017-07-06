// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/android_overlay_provider_impl.h"

#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "jni/AndroidOverlayProviderImpl_jni.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;

namespace content {

// static
bool AndroidOverlayProviderImpl::RegisterAndroidOverlayProviderImpl(
    JNIEnv* env) {
  return RegisterNativesImpl(env);
}

static jlong GetNativeInstance(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  return reinterpret_cast<jlong>(AndroidOverlayProvider::GetInstance());
}

// static
AndroidOverlayProvider* AndroidOverlayProvider::GetInstance() {
  static AndroidOverlayProvider* instance = nullptr;
  if (!instance)
    instance = new AndroidOverlayProviderImpl();

  return instance;
}

AndroidOverlayProviderImpl::AndroidOverlayProviderImpl() {}

AndroidOverlayProviderImpl::~AndroidOverlayProviderImpl() {}

bool AndroidOverlayProviderImpl::AreOverlaysSupported() const {
  JNIEnv* env = AttachCurrentThread();

  return Java_AndroidOverlayProviderImpl_areOverlaysSupported(env);
}

bool AndroidOverlayProviderImpl::IsFrameValidAndVisible(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    jlong token_high,
    jlong token_low) {
  RenderFrameHostImpl* rfhi =
      content::RenderFrameHostImpl::FromOverlayRoutingToken(
          base::UnguessableToken::Deserialize(token_high, token_low));

  if (!rfhi)
    return false;

  WebContentsImpl* web_contents_impl = static_cast<WebContentsImpl*>(
      content::WebContents::FromRenderFrameHost(rfhi));

  return rfhi->IsCurrent() && !web_contents_impl->IsHidden();
}

}  // namespace content
