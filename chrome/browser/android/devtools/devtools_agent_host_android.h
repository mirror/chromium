// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DEVTOOLS_DEVTOOLS_AGENT_HOST_ANDROID_H_
#define CHROME_BROWSER_ANDROID_DEVTOOLS_DEVTOOLS_AGENT_HOST_ANDROID_H_

#include <string>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/values.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_client.h"

// Native counterpart to DevToolsAgentHostImpl.java.
// Instances of this class are owned by their Java-side object.
class DevToolsAgentHostAndroid : public content::DevToolsAgentHostClient {
 public:
  DevToolsAgentHostAndroid(JNIEnv* env,
                           jobject obj,
                           scoped_refptr<content::DevToolsAgentHost> host);
  ~DevToolsAgentHostAndroid() override;

  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  void SendCommand(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& obj,
                   const base::android::JavaParamRef<jstring>& method,
                   const base::android::JavaParamRef<jstring>& params,
                   const base::android::JavaParamRef<jobject>& callback);

  // DevToolsAgentHostClient interface:

  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               const std::string& message) override;

  void AgentHostClosed(content::DevToolsAgentHost* agent_host) override;

 private:
  JavaObjectWeakGlobalRef weak_java_host_;
  scoped_refptr<content::DevToolsAgentHost> host_;
  int last_request_id_;
  base::flat_map<int, base::android::ScopedJavaGlobalRef<jobject>> pending_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsAgentHostAndroid);
};

#endif  // CHROME_BROWSER_ANDROID_DEVTOOLS_DEVTOOLS_AGENT_HOST_ANDROID_H_
