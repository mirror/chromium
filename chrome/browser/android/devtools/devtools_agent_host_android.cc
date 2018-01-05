// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/devtools/devtools_agent_host_android.h"

#include <memory>
#include <string>

#include "base/android/jni_string.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "jni/DevToolsAgentHostImpl_jni.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;

DevToolsAgentHostAndroid::DevToolsAgentHostAndroid(
    JNIEnv* env,
    jobject obj,
    scoped_refptr<content::DevToolsAgentHost> host)
    : weak_java_host_(env, obj), host_(host), last_request_id_(0) {
  host->AttachClient(this);
}

DevToolsAgentHostAndroid::~DevToolsAgentHostAndroid() {
  if (host_) {
    host_->DetachClient(this);
    host_ = nullptr;
  }
}

void DevToolsAgentHostAndroid::Destroy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  delete this;
}

void DevToolsAgentHostAndroid::SendCommand(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& jmethod,
    const base::android::JavaParamRef<jstring>& jparams,
    const base::android::JavaParamRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!host_) {
    ScopedJavaLocalRef<jstring> error =
        ConvertUTF8ToJavaString(env, "devtools disconnected");
    Java_DevToolsAgentHostImpl_onCommandResult(env, callback, nullptr, error);
    return;
  }

  const int id = ++last_request_id_;
  pending_[id].Reset(env, callback);

  const std::string idstr = base::NumberToString(id);
  const std::string method = ConvertJavaStringToUTF8(env, jmethod);
  const std::string params =
      jparams ? ConvertJavaStringToUTF8(env, jparams) : "{}";
  const std::string message =
      base::StrCat({"{\"id\":", idstr, ",\"method\":\"", method,
                    "\",\"params\":", params, "}"});
  host_->DispatchProtocolMessage(this, message);
}

void DevToolsAgentHostAndroid::DispatchProtocolMessage(
    content::DevToolsAgentHost* agent_host,
    const std::string& message) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (agent_host != host_) {
    LOG(ERROR) << "Unexpected message for a different devtools host";
    return;
  }

  std::unique_ptr<base::Value> json = base::JSONReader::Read(message);
  if (!json || !json->is_dict()) {
    LOG(ERROR) << "Unexpected non-JSON object message from devtools: "
               << message;
    return;
  }
  base::DictionaryValue* dict = static_cast<base::DictionaryValue*>(json.get());

  int id;
  std::string method;
  if (dict->GetInteger("id", &id)) {
    // This is a reply to a previous command.
    auto it = pending_.find(id);
    if (it == pending_.end()) {
      LOG(ERROR) << "Unexpected reply to an unmapped command ID: " << message;
      return;
    }
    ScopedJavaGlobalRef<jobject> jcallback = it->second;
    pending_.erase(it);

    JNIEnv* env = base::android::AttachCurrentThread();
    ScopedJavaLocalRef<jstring> jresult;
    ScopedJavaLocalRef<jstring> jerror;

    const base::Value* value = dict->FindKey("result");
    if (value) {
      std::string json;
      if (base::JSONWriter::Write(*value, &json)) {
        jresult = ConvertUTF8ToJavaString(env, json);
      } else {
        jerror = ConvertUTF8ToJavaString(env, "JSONWriter failed");
      }
    } else {
      jresult = ConvertUTF8ToJavaString(env, "{}");
    }

    Java_DevToolsAgentHostImpl_onCommandResult(env, jcallback, jresult, jerror);
  } else if (dict->GetString("method", &method)) {
    // This is an async event notification from the devtools.
    std::string params;
    const base::Value* value = dict->FindKey("params");
    if (!value || !base::JSONWriter::Write(*value, &params)) {
      params = "{}";
    }
    JNIEnv* env = base::android::AttachCurrentThread();
    ScopedJavaLocalRef<jobject> jthis = weak_java_host_.get(env);
    ScopedJavaLocalRef<jstring> jmethod = ConvertUTF8ToJavaString(env, method);
    ScopedJavaLocalRef<jstring> jparams = ConvertUTF8ToJavaString(env, params);
    Java_DevToolsAgentHostImpl_onEvent(env, jthis, jmethod, jparams);
  } else if (dict->FindKey("error")) {
    LOG(ERROR) << "Error from devtools: " << message;
  } else {
    LOG(ERROR) << "Unrecognized message from devtools: " << message;
  }
}

void DevToolsAgentHostAndroid::AgentHostClosed(
    content::DevToolsAgentHost* agent_host) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (agent_host == host_) {
    // Note that this DevToolsAgentHostClient is already detached when
    // AgentHostClosed is invoked.
    host_ = nullptr;
  }
}

// static
static jlong JNI_DevToolsAgentHostImpl_Init(JNIEnv* env,
                                            const JavaParamRef<jobject>& obj,
                                            jlong native_host) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  scoped_refptr<content::DevToolsAgentHost> host(
      reinterpret_cast<content::DevToolsAgentHost*>(native_host));
  // Balance the AddRef() from TabAndroid::GetOrCreateDevToolsAgentHost.
  host.get()->Release();

  DevToolsAgentHostAndroid* host_android =
      new DevToolsAgentHostAndroid(env, obj, host);

  return reinterpret_cast<intptr_t>(host_android);
}
