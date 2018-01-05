// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_ASSISTANT_WEB_ASSISTANT_SERVICE_ANDROID_H_
#define CHROME_BROWSER_ANDROID_ASSISTANT_WEB_ASSISTANT_SERVICE_ANDROID_H_

#include <memory>
#include <string>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

// Native counterpart to WebAssistantService.java.
// Instances of this class are owned by their Java-side object.
class WebAssistantServiceAndroid : public net::URLFetcherDelegate {
 public:
  WebAssistantServiceAndroid(JNIEnv* env, jobject obj, std::string service_url);
  ~WebAssistantServiceAndroid() override;

  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  void GetActionsForUrl(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj,
                        const base::android::JavaParamRef<jstring>& url,
                        const base::android::JavaParamRef<jobject>& callback);

 private:
  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  JavaObjectWeakGlobalRef weak_java_host_;
  const GURL service_url_;
  std::unique_ptr<net::URLFetcher> fetcher_;
  std::string url_;
  base::android::ScopedJavaGlobalRef<jobject> callback_;

  DISALLOW_COPY_AND_ASSIGN(WebAssistantServiceAndroid);
};

#endif  // CHROME_BROWSER_ANDROID_ASSISTANT_WEB_ASSISTANT_SERVICE_ANDROID_H_
