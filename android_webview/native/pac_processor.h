// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_LIB_PACSERVICE_PAC_PROCESSOR__H_
#define ANDROID_WEBVIEW_LIB_PACSERVICE_PAC_PROCESSOR__H_

#include <jni.h>

#include <memory>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/time/time.h"
#include "net/log/net_log_with_source.h"
#include "net/proxy/proxy_resolver.h"
#include "net/proxy/proxy_resolver_factory.h"

namespace net {
class HostResolver;
class ProxyInfo;
class ProxyResolverErrorObserver;
class ProxyResolverFactoryV8TracingWrapper;
}  //namespace net

namespace android_webview {

namespace pac_service {

class PacProcessor {

 public:
  PacProcessor();
  ~PacProcessor();

  void Start(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void Stop(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void Resolve(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& url);
  void SetPacScript(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj,
                    const base::android::JavaParamRef<jstring>& content);
  void ResolveCallback(int error);

 private:
  DISALLOW_COPY_AND_ASSIGN(PacProcessor);

  JavaObjectWeakGlobalRef java_ref_;

  std::unique_ptr<net::ProxyResolver> proxy_resolver_;
  std::unique_ptr<net::HostResolver> host_resolver_;
  std::unique_ptr<net::ProxyResolverFactoryV8TracingWrapper> proxy_factory_;
  std::unique_ptr<base::MessageLoop> message_loop_;
  std::unique_ptr<net::ProxyResolverFactory::Request> create_request_;
  std::unique_ptr<net::ProxyResolver::Request> resolve_request_;
  std::unique_ptr<net::ProxyInfo> resolve_results_;
  net::NetLogWithSource net_log_;
  base::Time start_time_;
};

bool RegisterPacProcessor(JNIEnv* env);

}  // namespace pac_service

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_LIB_PACSERVICE_PAC_PROCESSOR__HH
