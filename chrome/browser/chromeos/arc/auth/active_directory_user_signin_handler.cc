// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/auth/active_directory_user_signin_handler.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/arc/arc_optin_uma.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/dm_token_storage.h"
#include "chrome/browser/chromeos/settings/install_attributes.h"
#include "components/policy/core/common/cloud/device_management_service.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace em = enterprise_management;

namespace {

constexpr char kSamlAuthErrorMessage[] = "SAML authentication failed. ";

policy::BrowserPolicyConnectorChromeOS* GetConnector() {
  return g_browser_process->platform_part()
      ->browser_policy_connector_chromeos();
}

policy::DeviceManagementService* GetDeviceManagementService() {
  return GetConnector()->device_management_service();
}

std::string GetClientId() {
  return GetConnector()->GetInstallAttributes()->GetDeviceId();
}

std::string GetEndpointUrl() {
  return "https://gaia_url_tbd";
}

}  // namespace

namespace arc {

ActiveDirectoryUserSigninHandler::ActiveDirectoryUserSigninHandler(
    ArcSupportHost* support_host)
    : support_host_(support_host), weak_ptr_factory_(this) {
  DCHECK(support_host_);
  support_host_->SetAuthDelegate(this);
}

ActiveDirectoryUserSigninHandler::~ActiveDirectoryUserSigninHandler() {
  support_host_->SetAuthDelegate(nullptr);
}

void ActiveDirectoryUserSigninHandler::Signin(SigninCallback callback) {
  DCHECK(callback_.is_null());
  callback_ = std::move(callback);
  dm_token_storage_ = std::make_unique<policy::DMTokenStorage>(
      g_browser_process->local_state());
  dm_token_storage_->RetrieveDMToken(
      base::BindOnce(&ActiveDirectoryUserSigninHandler::OnDMTokenAvailable,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ActiveDirectoryUserSigninHandler::OnDMTokenAvailable(
    const std::string& dm_token) {
  if (dm_token.empty()) {
    LOG(ERROR) << "Retrieving the DMToken failed.";
    std::move(callback_).Run(Status::FAILURE);
    return;
  }

  DCHECK(dm_token_.empty());
  dm_token_ = dm_token;
  SendUserSigninRequest();
}

void ActiveDirectoryUserSigninHandler::SendUserSigninRequest() {
  DCHECK(!dm_token_.empty());
  DCHECK(!fetch_request_job_);
  VLOG(1) << "Sending user signin request";

  policy::DeviceManagementService* service = GetDeviceManagementService();
  fetch_request_job_.reset(service->CreateJob(
      policy::DeviceManagementRequestJob::TYPE_ACTIVE_DIRECTORY_USER_SIGNIN,
      g_browser_process->system_request_context()));

  fetch_request_job_->SetDMToken(dm_token_);
  fetch_request_job_->SetClientID(GetClientId());
  fetch_request_job_->GetRequest()
      ->mutable_active_directory_user_signin_request();

  fetch_request_job_->Start(base::Bind(
      &ActiveDirectoryUserSigninHandler::OnUserSignInResponseReceived,
      weak_ptr_factory_.GetWeakPtr()));
}

void ActiveDirectoryUserSigninHandler::OnUserSignInResponseReceived(
    policy::DeviceManagementStatus dm_status,
    int net_error,
    const em::DeviceManagementResponse& response) {
  VLOG(1) << "User signin response received. DM Status: " << dm_status;
  fetch_request_job_.reset();

  Status fetch_status;
  switch (dm_status) {
    case policy::DM_STATUS_SUCCESS: {
      if (!response.has_active_directory_user_signin_response()) {
        LOG(WARNING) << "Invalid Active Directory user signin response.";
        fetch_status = Status::FAILURE;
        break;
      }
      const em::ActiveDirectoryUserSigninResponse& signin_response =
          response.active_directory_user_signin_response();

      InitiateSamlFlow(signin_response.auth_redirect_url());

      // The SAML flow eventually calls |callback_| or this function.
      return;
    }
    default: {
      LOG(ERROR) << "Active Directory user signin failed. DM Status: "
                 << dm_status;
      fetch_status = Status::FAILURE;
      break;
    }
  }

  VLOG(1) << "Active Directory user signin finished. Status: "
          << static_cast<int>(fetch_status);
  dm_token_.clear();
  std::move(callback_).Run(fetch_status);
}

void ActiveDirectoryUserSigninHandler::InitiateSamlFlow(
    const std::string& auth_redirect_url) {
  VLOG(1) << "Initiating SAML flow. Auth redirect URL: " << auth_redirect_url;

  // Check if URL is valid.
  const GURL redirect_url(auth_redirect_url);
  if (!redirect_url.is_valid()) {
    LOG(ERROR) << kSamlAuthErrorMessage << "Redirect URL invalid.";
    CancelSamlFlow();
    return;
  }

  // Send the URL to the support host to display it in a web view inside the
  // Active Directory auth page.
  support_host_->ShowActiveDirectoryAuth(redirect_url, GetEndpointUrl());
}

void ActiveDirectoryUserSigninHandler::CancelSamlFlow() {
  VLOG(1) << "Cancelling SAML flow.";
  dm_token_.clear();
  DCHECK(!callback_.is_null());
  std::move(callback_).Run(Status::FAILURE);
}

void ActiveDirectoryUserSigninHandler::OnAuthSucceeded() {
  VLOG(1) << "SAML auth succeeded.";
  dm_token_.clear();
  DCHECK(!callback_.is_null());
  std::move(callback_).Run(Status::SUCCESS);
}

void ActiveDirectoryUserSigninHandler::OnAuthFailed(
    const std::string& error_msg) {
  LOG(ERROR) << "SAML auth failed: " << error_msg;

  // Don't call callback here, allow user to retry.
  support_host_->ShowError(ArcSupportHost::Error::SERVER_COMMUNICATION_ERROR,
                           true /* should_show_send_feedback */);
  UpdateOptInCancelUMA(OptInCancelReason::NETWORK_ERROR);
}

void ActiveDirectoryUserSigninHandler::OnAuthRetryClicked() {
  VLOG(1) << "Retrying user signin.";

  // Retry the full flow (except DM token fetch), not just the SAML part, in
  // case DM server returned bad data.
  DCHECK(!callback_.is_null());
  support_host_->ShowArcLoading();
  SendUserSigninRequest();
}

}  // namespace arc
