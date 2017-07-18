// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/page_info/connection_info_popup_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "chrome/browser/android/resource_mapper.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "chrome/browser/ui/page_info/page_info.h"
#include "components/security_state/core/security_state.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"
#include "jni/ConnectionInfoPopup_jni.h"
#include "net/cert/x509_certificate.h"
#include "ui/base/l10n/l10n_util.h"

using base::android::CheckException;
using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertUTF16ToJavaString;
using base::android::GetClass;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using content::WebContents;

// static
static jlong Init(JNIEnv* env,
                  const JavaParamRef<jclass>& clazz,
                  const JavaParamRef<jobject>& obj,
                  const JavaParamRef<jobject>& java_web_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(java_web_contents);

  return reinterpret_cast<intptr_t>(
      new ConnectionInfoPopupAndroid(env, obj, web_contents));
}

ConnectionInfoPopupAndroid::ConnectionInfoPopupAndroid(
    JNIEnv* env,
    jobject java_page_info_pop,
    WebContents* web_contents) {
  // Important to use GetVisibleEntry to match what's showing in the omnibox.
  content::NavigationEntry* nav_entry =
      web_contents->GetController().GetVisibleEntry();
  if (nav_entry == nullptr)
    return;

  popup_jobject_.Reset(env, java_page_info_pop);

  SecurityStateTabHelper* helper =
      SecurityStateTabHelper::FromWebContents(web_contents);
  DCHECK(helper);

  security_state::SecurityInfo security_info;
  helper->GetSecurityInfo(&security_info);

  presenter_.reset(new PageInfo(
      this, Profile::FromBrowserContext(web_contents->GetBrowserContext()),
      TabSpecificContentSettings::FromWebContents(web_contents), web_contents,
      nav_entry->GetURL(), security_info));
}

ConnectionInfoPopupAndroid::~ConnectionInfoPopupAndroid() {
}

void ConnectionInfoPopupAndroid::Destroy(JNIEnv* env,
                                         const JavaParamRef<jobject>& obj) {
  delete this;
}

void ConnectionInfoPopupAndroid::ResetCertDecisions(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& java_web_contents) {
  presenter_->OnRevokeSSLErrorBypassButtonPressed();
}

void ConnectionInfoPopupAndroid::SetIdentityInfo(
    const IdentityInfo& identity_info) {
  JNIEnv* env = base::android::AttachCurrentThread();

  ConnectionInfoPopupAndroid::AddExplanations(
      env, PageInfo::SITE_IDENTITY_STATUS_CERT,
      identity_info.security_style_explanations.secure_explanations);
  ConnectionInfoPopupAndroid::AddExplanations(
      env, PageInfo::SITE_IDENTITY_STATUS_UNKNOWN,
      identity_info.security_style_explanations.neutral_explanations);
  ConnectionInfoPopupAndroid::AddExplanations(
      env, PageInfo::SITE_IDENTITY_STATUS_ERROR,
      identity_info.security_style_explanations.insecure_explanations);
  ConnectionInfoPopupAndroid::AddExplanations(
      env, PageInfo::SITE_IDENTITY_STATUS_INTERNAL_PAGE,
      identity_info.security_style_explanations.info_explanations);

  // Java_ConnectionInfoPopup_addMoreInfoLink(
  //     env, popup_jobject_,
  //     ConvertUTF8ToJavaString(
  //         env, l10n_util::GetStringUTF8(IDS_PAGE_INFO_HELP_CENTER_LINK)));
  Java_ConnectionInfoPopup_showDialog(env, popup_jobject_);
}

void ConnectionInfoPopupAndroid::SetCookieInfo(
    const CookieInfoList& cookie_info_list) {
  NOTIMPLEMENTED();
}

void ConnectionInfoPopupAndroid::SetPermissionInfo(
    const PermissionInfoList& permission_info_list,
    ChosenObjectInfoList chosen_object_info_list) {
  NOTIMPLEMENTED();
}

void ConnectionInfoPopupAndroid::AddExplanations(
    JNIEnv* env,
    PageInfo::SiteIdentityStatus status,
    const std::vector<content::SecurityStyleExplanation>& explanations) {
  if (explanations.size() == 0) {
    return;
  }

  int icon_id =
      ResourceMapper::MapFromChromiumId(PageInfoUI::GetIdentityIconID(status));

  LOG(ERROR) << "AddExplanations icon_id: " << icon_id;

  for (const auto& it : explanations) {
    LOG(ERROR) << "iteration";
    ScopedJavaLocalRef<jstring> summary =
        ConvertUTF8ToJavaString(env, it.summary);
    ScopedJavaLocalRef<jstring> description =
        ConvertUTF8ToJavaString(env, it.description);
    LOG(ERROR) << "summary: " << it.summary;
    LOG(ERROR) << "description: " << it.description;
    Java_ConnectionInfoPopup_addSection(env, popup_jobject_, icon_id, summary,
                                        description);
  }
  LOG(ERROR) << "end loop";
}
