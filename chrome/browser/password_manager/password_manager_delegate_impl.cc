// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_manager_delegate_impl.h"

#include "base/bind_helpers.h"
#include "base/memory/singleton.h"
#include "base/metrics/histogram.h"
#include "chrome/browser/password_manager/password_form_manager.h"
#include "chrome/browser/password_manager/password_manager.h"
#include "chrome/browser/password_manager/password_manager_metrics_util.h"
#include "chrome/browser/password_manager/save_password_infobar_delegate.h"
#include "chrome/browser/profiles/profile.h"
#include "components/autofill/content/browser/autofill_driver_impl.h"
#include "components/autofill/content/common/autofill_messages.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/autofill/core/common/password_form.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/password_authentication_manager.h"
#endif  // OS_ANDROID

DEFINE_WEB_CONTENTS_USER_DATA_KEY(PasswordManagerDelegateImpl);

PasswordManagerDelegateImpl::PasswordManagerDelegateImpl(
    content::WebContents* web_contents)
    : web_contents_(web_contents),
      weak_factory_(this) {
}

PasswordManagerDelegateImpl::~PasswordManagerDelegateImpl() {
}

void PasswordManagerDelegateImpl::FillPasswordForm(
    const autofill::PasswordFormFillData& form_data) {
  web_contents_->GetRenderViewHost()->Send(
      new AutofillMsg_FillPasswordForm(
          web_contents_->GetRenderViewHost()->GetRoutingID(),
          form_data));
}

void PasswordManagerDelegateImpl::AuthenticateAutofillAndFillForm(
      scoped_ptr<autofill::PasswordFormFillData> fill_data) {
#if defined(OS_ANDROID)
  if (PasswordAuthenticationManager
      ::IsAutofillPasswordAuthenticationEnabled()) {
    PasswordAuthenticationManager::AuthenticatePasswordAutofill(
        web_contents_,
        base::Bind(&PasswordManagerDelegateImpl::CommitFillPasswordForm,
                   weak_factory_.GetWeakPtr(),
                   base::Owned(fill_data.release())));
    return;
  }
#endif  // OS_ANDROID

  // Additional authentication is currently only available for Android, so all
  // other plaftorms should just fill the password form directly.
  CommitFillPasswordForm(fill_data.get());
}

void PasswordManagerDelegateImpl::AddSavePasswordInfoBarIfPermitted(
    PasswordFormManager* form_to_save) {
  std::string uma_histogram_suffix(
      password_manager_metrics_util::GroupIdToString(
          password_manager_metrics_util::MonitoredDomainGroupId(
              form_to_save->realm(), GetProfile()->GetPrefs())));
  SavePasswordInfoBarDelegate::Create(
      web_contents_, form_to_save, uma_histogram_suffix);
}

Profile* PasswordManagerDelegateImpl::GetProfile() {
  return Profile::FromBrowserContext(web_contents_->GetBrowserContext());
}

bool PasswordManagerDelegateImpl::DidLastPageLoadEncounterSSLErrors() {
  content::NavigationEntry* entry =
      web_contents_->GetController().GetActiveEntry();
  if (!entry) {
    NOTREACHED();
    return false;
  }

  return net::IsCertStatusError(entry->GetSSL().cert_status);
}

void PasswordManagerDelegateImpl::CommitFillPasswordForm(
    autofill::PasswordFormFillData* data) {
  FillPasswordForm(*data);
}
