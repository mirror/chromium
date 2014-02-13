// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_DELEGATE_IMPL_H_
#define CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_DELEGATE_IMPL_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/password_manager/password_manager_delegate.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

class PasswordManagerDelegateImpl
    : public PasswordManagerDelegate,
      public content::WebContentsUserData<PasswordManagerDelegateImpl> {
 public:
  virtual ~PasswordManagerDelegateImpl();

  // PasswordManagerDelegate implementation.
  virtual void FillPasswordForm(
      const autofill::PasswordFormFillData& form_data) OVERRIDE;
  virtual void AuthenticateAutofillAndFillForm(
      scoped_ptr<autofill::PasswordFormFillData> fill_data) OVERRIDE;
  virtual void AddSavePasswordInfoBarIfPermitted(
      PasswordFormManager* form_to_save) OVERRIDE;
  virtual Profile* GetProfile() OVERRIDE;
  virtual bool DidLastPageLoadEncounterSSLErrors() OVERRIDE;

 private:
  explicit PasswordManagerDelegateImpl(content::WebContents* web_contents);
  friend class content::WebContentsUserData<PasswordManagerDelegateImpl>;

  // Callback method to be triggered when authentication is successful for a
  // given password authentication request.  If authentication is disabled or
  // not supported, this will be triggered directly.
  void CommitFillPasswordForm(autofill::PasswordFormFillData* fill_data);

  content::WebContents* web_contents_;

  // Allows authentication callbacks to be destroyed when this client is gone.
  base::WeakPtrFactory<PasswordManagerDelegateImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerDelegateImpl);
};

#endif  // CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_DELEGATE_IMPL_H_
