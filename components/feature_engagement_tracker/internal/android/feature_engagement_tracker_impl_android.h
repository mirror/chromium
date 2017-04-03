// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_ANDROID_FEATURE_ENGAGEMENT_TRACKER_IMPL_ANDROID_H_
#define COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_ANDROID_FEATURE_ENGAGEMENT_TRACKER_IMPL_ANDROID_H_

#include <string>
#include <unordered_map>

#include "base/android/callback_android.h"
#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/supports_user_data.h"
#include "components/feature_engagement_tracker/internal/feature_engagement_tracker_impl.h"

namespace base {
struct Feature;
}  // namespace base

namespace feature_engagement_tracker {

// JNI bridge between FeatureEngagementTrackerImpl in Java and C++. See the
// public API of FeatureEngagementTracker for documentation for all methods.
class FeatureEngagementTrackerImplAndroid
    : public base::SupportsUserData::Data {
 public:
  static bool RegisterJni(JNIEnv* env);
  static FeatureEngagementTrackerImplAndroid* FromJavaObject(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& jobj);

  explicit FeatureEngagementTrackerImplAndroid(
      FeatureEngagementTrackerImpl* feature_engagement_tracker_impl);
  ~FeatureEngagementTrackerImplAndroid() override;

  base::android::ScopedJavaLocalRef<jobject> GetJavaObject();

  FeatureEngagementTrackerImpl* feature_engagement_tracker_impl() {
    return feature_engagement_tracker_impl_;
  }

  // FeatureEngagementTracker JNI bridge implementation.
  virtual void Event(JNIEnv* env,
                     const base::android::JavaRef<jobject>& jobj,
                     const base::android::JavaParamRef<jstring>& jfeature,
                     const base::android::JavaParamRef<jstring>& jprecondition);
  virtual void Used(JNIEnv* env,
                    const base::android::JavaRef<jobject>& jobj,
                    const base::android::JavaParamRef<jstring>& jfeature);
  virtual bool Trigger(JNIEnv* env,
                       const base::android::JavaRef<jobject>& jobj,
                       const base::android::JavaParamRef<jstring>& jfeature);
  virtual void Dismissed(JNIEnv* env,
                         const base::android::JavaRef<jobject>& jobj);
  virtual void AddOnInitializedCallback(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& jobj,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

 private:
  // Uses GetAllFeatures() to set up the |features_| map. This is always called
  // from the constructor.
  void SetUpFeatureMapping();

  // A map from the base::Feature name to the feature, to ensure that the Java
  // version of the API can use the string name. If base::Feature becomes a Java
  // class as well, we should remove this mapping.
  std::unordered_map<std::string, const base::Feature*> features_;

  // The FeatureEngagementTrackerImpl this is a JNI bridge for.
  FeatureEngagementTrackerImpl* feature_engagement_tracker_impl_;

  // The Java-side of this JNI bridge.
  base::android::ScopedJavaGlobalRef<jobject> java_obj_;

  DISALLOW_COPY_AND_ASSIGN(FeatureEngagementTrackerImplAndroid);
};

}  // namespace feature_engagement_tracker

#endif  // COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_ANDROID_FEATURE_ENGAGEMENT_TRACKER_IMPL_ANDROID_H_
