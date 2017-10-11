// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/preferences/pref_change_registrar_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "jni/PrefChangeRegistrar_jni.h"

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;

PrefChangeRegistrarAndroid::PrefChangeRegistrarAndroid(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    bool is_incognito) {
  Profile* last_profile = ProfileManager::GetLastUsedProfile();
  profile_ = is_incognito ? last_profile->GetOffTheRecordProfile()
                          : last_profile->GetOriginalProfile();

  pref_change_registrar_ = base::MakeUnique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(profile_->GetPrefs());

  pref_change_registrar_jobject_.Reset(env, obj);
}

PrefChangeRegistrarAndroid::~PrefChangeRegistrarAndroid() {}

void PrefChangeRegistrarAndroid::Destroy(JNIEnv*,
                                         const JavaParamRef<jobject>&) {
  delete this;
}

void PrefChangeRegistrarAndroid::Add(JNIEnv* env,
                                     const JavaParamRef<jobject>& obj,
                                     const JavaParamRef<jstring>& j_pref_name) {
  std::string pref_name = ConvertJavaStringToUTF8(env, j_pref_name);
  pref_change_registrar_->Add(
      pref_name, base::Bind(&PrefChangeRegistrarAndroid::OnPreferenceChange,
                            base::Unretained(this), pref_name));
}

void PrefChangeRegistrarAndroid::OnPreferenceChange(std::string pref_name) {
  JNIEnv* env = AttachCurrentThread();
  Java_PrefChangeRegistrar_onPreferenceChange(
      env, pref_change_registrar_jobject_,
      ConvertUTF8ToJavaString(env, pref_name));
}

jlong Init(JNIEnv* env,
           const JavaParamRef<jobject>& obj,
           jboolean is_incognito) {
  return reinterpret_cast<intptr_t>(
      new PrefChangeRegistrarAndroid(env, obj, is_incognito));
}
