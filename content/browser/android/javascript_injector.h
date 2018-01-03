// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_JAVASCRIPT_INJECTOR_H_
#define CONTENT_BROWSER_ANDROID_JAVASCRIPT_INJECTOR_H_

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "content/public/browser/web_contents.h"

namespace content {

class GinJavaBridgeDispatcherHost;
class WebContents;

// Native class for JavascriptInjectorImpl. Managed by Java side.
class JavascriptInjector {
 public:
  JavascriptInjector(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& retained_objects,
      WebContents* web_contents);
  ~JavascriptInjector();

  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  void SetAllowInspection(JNIEnv* env,
                          const base::android::JavaParamRef<jobject>& obj,
                          jboolean allow);

  void AddInterface(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& /* obj */,
      const base::android::JavaParamRef<jobject>& object,
      const base::android::JavaParamRef<jstring>& name,
      const base::android::JavaParamRef<jclass>& safe_annotation_clazz);

  void RemoveInterface(JNIEnv* env,
                       const base::android::JavaParamRef<jobject>& /* obj */,
                       const base::android::JavaParamRef<jstring>& name);

 private:
  // Manages injecting Java objects.
  scoped_refptr<GinJavaBridgeDispatcherHost> java_bridge_dispatcher_host_;

  DISALLOW_COPY_AND_ASSIGN(JavascriptInjector);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_JAVASCRIPT_INJECTOR_H_
