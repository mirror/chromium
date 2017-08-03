// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_CREDENTIAL_MANAGER_IMPL_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_CREDENTIAL_MANAGER_IMPL_H_

#include "components/password_manager/core/browser/credential_manager_password_form_manager.h"
#include "components/password_manager/core/browser/credential_manager_pending_prevent_silent_access_task.h"
#include "components/password_manager/core/browser/credential_manager_pending_request_task.h"
#include "components/password_manager/core/common/credential_manager_types.h"
#include "components/prefs/pref_member.h"

namespace password_manager {

class CredentialManagerImplDelegate {
 public:
  virtual GURL GetLastCommittedURL() const = 0;
  virtual GURL GetOrigin() const = 0;
};

using StoreCallback = base::OnceCallback<void()>;
using PreventSilentAccessCallback = base::OnceCallback<void()>;
using GetCallback =
    base::OnceCallback<void(CredentialManagerError,
                            const base::Optional<CredentialInfo>&)>;

class CredentialManagerImpl
    : public CredentialManagerPendingPreventSilentAccessTaskDelegate,
      public CredentialManagerPendingRequestTaskDelegate,
      public CredentialManagerPasswordFormManagerDelegate {
 public:
  CredentialManagerImpl(CredentialManagerImplDelegate* delegate,
                        PasswordManagerClient* client);
  ~CredentialManagerImpl() override;

  void Store(const CredentialInfo& credential, StoreCallback callback);
  void PreventSilentAccess(PreventSilentAccessCallback callback);
  void Get(CredentialMediationRequirement mediation,
           bool include_passwords,
           const std::vector<GURL>& federations,
           GetCallback callback);

  // CredentialManagerPendingRequestTaskDelegate:
  bool IsZeroClickAllowed() const override;
  GURL GetOrigin() const override;
  void SendCredential(const SendCredentialCallback& send_callback,
                      const CredentialInfo& info) override;
  void SendPasswordForm(const SendCredentialCallback& send_callback,
                        CredentialMediationRequirement mediation,
                        const autofill::PasswordForm* form) override;
  PasswordManagerClient* client() const override;

  // CredentialManagerPendingPreventSilentAccessTaskDelegate:
  PasswordStore* GetPasswordStore() override;
  void DoneRequiringUserMediation() override;

  // CredentialManagerPasswordFormManagerDelegate:
  void OnProvisionalSaveComplete() override;

  // Returns FormDigest for the current URL.
  PasswordStore::FormDigest GetSynthesizedFormForOrigin() const;

 private:
  GURL GetLastCommittedURL() const;

  CredentialManagerImplDelegate* delegate_;
  PasswordManagerClient* client_;
  std::unique_ptr<CredentialManagerPasswordFormManager> form_manager_;

  // Set to false to disable automatic signing in.
  BooleanPrefMember auto_signin_enabled_;

  // When 'Get' is called, it in turn calls out to the
  // PasswordStore; we push enough data into Pending*Task objects so that
  // they can properly respond to the request once the PasswordStore gives
  // us data.
  std::unique_ptr<CredentialManagerPendingRequestTask> pending_request_;
  std::unique_ptr<CredentialManagerPendingPreventSilentAccessTask>
      pending_require_user_mediation_;
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_CREDENTIAL_MANAGER_IMPL_H_
