// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/jni/jni_oauth_token_getter.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "jni/JniOAuthTokenGetter_jni.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::JavaParamRef;

namespace remoting {

static void ResolveOAuthTokenCallback(JNIEnv* env,
                                      const JavaParamRef<jclass>& clazz,
                                      jlong callback_ptr,
                                      jint jni_status,
                                      const JavaParamRef<jstring>& user_email,
                                      const JavaParamRef<jstring>& token) {
  auto* callback =
      reinterpret_cast<OAuthTokenGetter::TokenCallback*>(callback_ptr);
  OAuthTokenGetter::Status status;
  switch (static_cast<JniOAuthTokenGetter::JniStatus>(jni_status)) {
    case JniOAuthTokenGetter::JNI_STATUS_SUCCESS:
      status = OAuthTokenGetter::SUCCESS;
      break;
    case JniOAuthTokenGetter::JNI_STATUS_NETWORK_ERROR:
      status = OAuthTokenGetter::NETWORK_ERROR;
      break;
    case JniOAuthTokenGetter::JNI_STATUS_AUTH_ERROR:
      status = OAuthTokenGetter::AUTH_ERROR;
      break;
    default:
      NOTREACHED();
      return;
  }
  callback->Run(status, ConvertJavaStringToUTF8(user_email),
                ConvertJavaStringToUTF8(token));
  delete callback;
}

JniOAuthTokenGetter::JniOAuthTokenGetter() {}

JniOAuthTokenGetter::~JniOAuthTokenGetter() {}

void JniOAuthTokenGetter::CallWithToken(const TokenCallback& on_access_token) {
  JNIEnv* env = base::android::AttachCurrentThread();
  TokenCallback* callback_copy = new TokenCallback(on_access_token);
  Java_JniOAuthTokenGetter_fetchAuthToken(
      env, reinterpret_cast<intptr_t>(callback_copy));
}

void JniOAuthTokenGetter::InvalidateCache() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_JniOAuthTokenGetter_invalidateCache(env);
}

}  // namespace remoting
