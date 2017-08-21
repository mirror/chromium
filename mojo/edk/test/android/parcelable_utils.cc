// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/test/android/parcelable_utils.h"

#include "base/android/jni_android.h"

namespace mojo {
namespace edk {
namespace test {

base::android::ScopedJavaLocalRef<jobject> CreateJavaPoint(jint x, jint y) {
  JNIEnv* env = base::android::AttachCurrentThread();

  base::android::ScopedJavaLocalRef<jclass> point_jclass =
      base::android::GetClass(env, "android/graphics/Point");

  jmethodID point_constructor =
      base::android::MethodID::Get<base::android::MethodID::TYPE_INSTANCE>(
          env, point_jclass.obj(), "<init>", "(II)V");

  jobject point_obj =
      env->NewObject(point_jclass.obj(), point_constructor, x, y);
  return base::android::ScopedJavaLocalRef<jobject>(env, point_obj);
}

bool AreJavaObjectsEqual(jobject object1, jobject object2) {
  if (object1 == nullptr || object2 == nullptr)
    return object1 == object2;

  JNIEnv* env = base::android::AttachCurrentThread();

  base::android::ScopedJavaLocalRef<jclass> object_jclass =
      base::android::GetClass(env, "java/lang/Object");

  jmethodID equal_method_id =
      base::android::MethodID::Get<base::android::MethodID::TYPE_INSTANCE>(
          env, object_jclass.obj(), "equals", "(Ljava/lang/Object;)Z");

  bool result = env->CallBooleanMethod(object1, equal_method_id, object2);
  base::android::CheckException(env);
  return result;
}

}  // namespace test
}  // namespace edk
}  // namespace mojo