// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/native/pac_processor.h"

#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/memory/ptr_util.h"
#include "base/logging.h"
#include "jni/PacProcessor_jni.h"
#include "net/dns/host_resolver.h"
#include "net/proxy/proxy_info.h"
#include "net/proxy/proxy_resolver_script_data.h"
#include "net/proxy/proxy_resolver_v8.h"
#include "net/proxy/proxy_resolver_error_observer.h"

#include "net/proxy/proxy_resolver_v8_tracing.h"
#include "net/proxy/proxy_resolver_v8_tracing_wrapper.h"

#include "content/public/common/content_descriptors.h"
#include "gin/v8_initializer.h"
#include "base/android/apk_assets.h"

#include "url/gurl.h"

using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertJavaStringToUTF8;
using base::android::JavaRef;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using net::ProxyResolverFactoryV8TracingWrapper;
using net::ProxyResolverErrorObserver;

namespace {

class ErrorObserver : public ProxyResolverErrorObserver {
 public:
  void OnPACScriptError(int line_number, const base::string16& error) override {
    LOG(ERROR) << "PAC ERROR line " << line_number << ": " << error;
  }
};

std::unique_ptr<ProxyResolverErrorObserver> GetErrorObserver() {
  LOG(ERROR) << "GetErrorObserver";
  return std::unique_ptr<ErrorObserver>(new ErrorObserver());
}

void CompletionCallback(int result) {
  LOG(ERROR) << "proxy create complete: " << result;
}
}

namespace android_webview {

namespace pac_service {

PacProcessor::PacProcessor()
  : host_resolver_(net::HostResolver::CreateDefaultResolver(nullptr)),
    create_request_(new net::ProxyResolverFactory::Request()) {
}

PacProcessor::~PacProcessor() {
}

void PacProcessor::Start(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  LOG(ERROR) << "Native Start!";

  java_ref_ = JavaObjectWeakGlobalRef(env, obj);

  message_loop_.reset(new base::MessageLoop(base::MessageLoop::TYPE_UI));
  static_cast<base::MessageLoopForUI*>(message_loop_.get())->Start();
  proxy_factory_.reset(new ProxyResolverFactoryV8TracingWrapper(
      host_resolver_.get(),
      nullptr,
      base::Bind(&GetErrorObserver)));
}

void PacProcessor::Stop(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  LOG(ERROR) << "Native Stop!";
  static_cast<base::MessageLoopForUI*>(message_loop_.get())->QuitWhenIdle();
}

void PacProcessor::Resolve(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& jurl) {
  GURL url(ConvertJavaStringToUTF8(jurl));
  LOG(ERROR) << "Native resolve url: " << url.spec();

  resolve_results_.reset(new net::ProxyInfo());

  start_time_ = base::Time::Now();
  proxy_resolver_->GetProxyForURL(url,
                                  resolve_results_.get(),
                                  base::Bind(&PacProcessor::ResolveCallback, base::Unretained(this)),
                                  &resolve_request_,
                                  net_log_);
}

void PacProcessor::SetPacScript(JNIEnv* env,
                                const JavaParamRef<jobject>& obj,
                                const JavaParamRef<jstring>& content) {
  LOG(ERROR) << "Native setPacScript";

  if (base::MessageLoop::current() == nullptr) {
    LOG(ERROR) << "no message loop!";
  }

  LOG(ERROR) << "content: " << ConvertJavaStringToUTF8(env, content);

  std::unique_ptr<net::ProxyResolverFactory::Request> request;

  proxy_factory_->CreateProxyResolver(
      net::ProxyResolverScriptData::FromUTF8(ConvertJavaStringToUTF8(env, content)),
      &proxy_resolver_,
      base::Bind(&CompletionCallback),
      &create_request_);
}

void PacProcessor::ResolveCallback(int result) {
  LOG(ERROR) << "resolve complete, result: " << result;
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  ScopedJavaLocalRef<jstring> result_jstring =
      ConvertUTF8ToJavaString(env, resolve_results_->ToPacString());
  if (!obj.is_null()) {
    LOG(ERROR) << "Resolve took: " << (base::Time::Now() - start_time_).InMilliseconds() << "ms.";
    Java_PacProcessor_OnResolveResult(env, obj, result_jstring);
  }

}

bool RegisterPacProcessor(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

// static
jlong Init(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  return reinterpret_cast<jlong>(new PacProcessor());
}

}  // pac_service

}  // android_webview
