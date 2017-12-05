// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_AUTH_ACTIVE_DIRECTORY_SIGNIN_HANDLER_H_
#define CHROME_BROWSER_CHROMEOS_ARC_AUTH_ACTIVE_DIRECTORY_SIGNIN_HANDLER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/arc/arc_support_host.h"
#include "chrome/browser/chromeos/arc/auth/arc_fetcher_base.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"

namespace enterprise_management {
class DeviceManagementResponse;
}  // namespace enterprise_management

namespace policy {
class DeviceManagementRequestJob;
class DMTokenStorage;
}  // namespace policy

namespace arc {

// Signs in an Active Directory user into a shadow Gaia account.
class ActiveDirectoryUserSigninHandler : public ArcFetcherBase,
                                         public ArcSupportHost::AuthDelegate {
 public:
  explicit ActiveDirectoryUserSigninHandler(ArcSupportHost* support_host);
  ~ActiveDirectoryUserSigninHandler() override;

  enum class Status {
    SUCCESS,  // The signin request was successful.
    FAILURE,  // The request failed.
  };

  // Signs in the user into the user's shadow Gaia account. The account is
  // created on the first signin attempt. Calls |callback| when done.
  // |callback| when done. |status| indicates whether the operation was
  // successful.
  // Signin() should be called once per instance, and it is expected that the
  // inflight operation is cancelled without calling the |callback| when the
  // instance is deleted.
  using SigninCallback = base::OnceCallback<void(Status status)>;
  void Signin(SigninCallback callback);

 private:
  // Called when the |dm_token| is retrieved from policy::DMTokenStorage.
  // Triggers SendUserSigninRequest().
  void OnDMTokenAvailable(const std::string& dm_token);

  // ArcSupportHost::AuthDelegate:
  void OnAuthSucceeded() override;
  void OnAuthFailed(const std::string& error_msg) override;
  void OnAuthRetryClicked() override;

  // Sends a request to DM Server to sign in the user.
  void SendUserSigninRequest();

  // Response from DM server. Initiates a SAML flow to authenticate the user.
  void OnUserSignInResponseReceived(
      policy::DeviceManagementStatus dm_status,
      int net_error,
      const enterprise_management::DeviceManagementResponse& response);

  // Sends |auth_redirect_url| to the ArcSupportHost instance, which displays
  // it in a web view and checks whether authentication succeeded. Passes
  // OnSamlFlowFinished as callback for when the SAML flow ends. Calls
  // CancelSamlFlow() if the url is invalid.
  void InitiateSamlFlow(const std::string& auth_redirect_url);

  // Calls callback_ with an error status and resets state.
  void CancelSamlFlow();

  // Callback called from ArcSupportHost when the SAML flow is finished.
  void OnSamlFlowFinished(bool result);

  ArcSupportHost* const support_host_ = nullptr;  // Not owned.

  std::unique_ptr<policy::DeviceManagementRequestJob> fetch_request_job_;
  std::unique_ptr<policy::DMTokenStorage> dm_token_storage_;
  SigninCallback callback_;

  std::string dm_token_;

  base::WeakPtrFactory<ActiveDirectoryUserSigninHandler> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(ActiveDirectoryUserSigninHandler);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_AUTH_ACTIVE_DIRECTORY_SIGNIN_HANDLER_H_
