// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/survey_infobar.h"

#include <memory>
#include <utility>

#include "base/android/jni_string.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "components/infobars/core/infobar_delegate.h"
#include "content/public/browser/web_contents.h"
#include "jni/SurveyInfoBar_jni.h"
#include "ui/android/window_android.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

class SurveyInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  ~SurveyInfoBarDelegate() override {}

  SurveyInfoBarDelegate(std::string surveyId, std::string advertisingId)
      : survey_id_(surveyId), advertising_id_(advertisingId) {}

  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override {
    return InfoBarDelegate::InfoBarIdentifier::SURVEY_INFOBAR_ANDROID;
  }

  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override {
    return delegate->GetIdentifier() == GetIdentifier();
  }

  std::string GetSurveyId() { return survey_id_; }

  std::string GetAdvertisingId() { return advertising_id_; }

 private:
  std::string survey_id_;
  std::string advertising_id_;
};

SurveyInfoBar::SurveyInfoBar(std::unique_ptr<SurveyInfoBarDelegate> delegate)
    : InfoBarAndroid(std::move(delegate)) {}

SurveyInfoBar::~SurveyInfoBar() {}

infobars::InfoBarDelegate* SurveyInfoBar::GetDelegate() {
  return delegate();
}

ScopedJavaLocalRef<jobject> SurveyInfoBar::CreateRenderInfoBar(JNIEnv* env) {
  SurveyInfoBarDelegate* survey_delegate =
      static_cast<SurveyInfoBarDelegate*>(delegate());
  return Java_SurveyInfoBar_create(
      env,
      base::android::ConvertUTF8ToJavaString(env,
                                             survey_delegate->GetSurveyId()),
      base::android::ConvertUTF8ToJavaString(
          env, survey_delegate->GetAdvertisingId()));
}

base::android::ScopedJavaLocalRef<jobject> SurveyInfoBar::GetTab(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  content::WebContents* web_contents =
      InfoBarService::WebContentsFromInfoBar(this);
  if (!web_contents)
    return nullptr;

  TabAndroid* tab_android = TabAndroid::FromWebContents(web_contents);
  return tab_android ? tab_android->GetJavaObject() : nullptr;
}

void SurveyInfoBar::ProcessButton(int action) {}

void Create(JNIEnv* env,
            const JavaParamRef<jclass>& j_caller,
            const JavaParamRef<jobject>& j_web_contents,
            const JavaParamRef<jstring>& j_survey_id,
            const JavaParamRef<jstring>& j_advertising_id) {
  InfoBarService* service = InfoBarService::FromWebContents(
      content::WebContents::FromJavaWebContents(j_web_contents));

  service->AddInfoBar(
      base::MakeUnique<SurveyInfoBar>(base::MakeUnique<SurveyInfoBarDelegate>(
          base::android::ConvertJavaStringToUTF8(env, j_survey_id),
          base::android::ConvertJavaStringToUTF8(env, j_advertising_id))));
}
