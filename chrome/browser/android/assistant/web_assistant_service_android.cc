// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/assistant/web_assistant_service_android.h"

#include <utility>

#include "base/android/jni_string.h"
#include "base/json/string_escape.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_thread.h"
#include "jni/WebAssistantService_jni.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;

namespace {

std::string MakeRequestBody(const std::string& url,
                            const std::string& version) {
  return base::StrCat(
      {"{\"url\":", base::GetQuotedJSONString(url),
       ",\"chrome_version\":", base::GetQuotedJSONString(version), "}"});
}

}  // namespace

WebAssistantServiceAndroid::WebAssistantServiceAndroid(JNIEnv* env,
                                                       jobject obj,
                                                       std::string service_url)
    : weak_java_host_(env, obj), service_url_(std::move(service_url)) {}

WebAssistantServiceAndroid::~WebAssistantServiceAndroid() {}

void WebAssistantServiceAndroid::Destroy(JNIEnv* env,
                                         const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  delete this;
}

void WebAssistantServiceAndroid::GetActionsForUrl(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& jurl,
    const JavaParamRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  url_ = ConvertJavaStringToUTF8(env, jurl);
  callback_.Reset(env, callback);

  std::string version = version_info::GetVersionNumber();
  std::string body = MakeRequestBody(url_, version);

  Profile* profile = ProfileManager::GetActiveUserProfile();

  // Note that this cancels any pending fetcher.
  fetcher_ = net::URLFetcher::Create(service_url_, net::URLFetcher::POST, this);
  fetcher_->SetRequestContext(profile->GetRequestContext());
  fetcher_->SetUploadData("application/json", body);
  fetcher_->Start();
}

void WebAssistantServiceAndroid::OnURLFetchComplete(
    const net::URLFetcher* fetcher) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::string response;

  if (fetcher != fetcher_.get() || !fetcher->GetStatus().is_success() ||
      fetcher->GetResponseCode() != net::HTTP_OK ||
      !fetcher->GetResponseAsString(&response)) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> jthis = weak_java_host_.get(env);
  ScopedJavaLocalRef<jstring> jurl = ConvertUTF8ToJavaString(env, url_);
  ScopedJavaLocalRef<jstring> jresponse =
      ConvertUTF8ToJavaString(env, response);
  Java_WebAssistantService_onActionsForUrlResponse(env, jthis, jurl, jresponse,
                                                   callback_);

  callback_.Reset();
  fetcher_.reset();
}

// static
static jlong JNI_WebAssistantService_Init(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& jservice_url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::string service_url = ConvertJavaStringToUTF8(env, jservice_url);

  WebAssistantServiceAndroid* service_android =
      new WebAssistantServiceAndroid(env, obj, std::move(service_url));

  return reinterpret_cast<intptr_t>(service_android);
}
