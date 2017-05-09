// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/auth/arc_active_directory_enrollment_token_fetcher.h"

#include <string>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/dm_token_storage.h"
#include "chrome/browser/chromeos/settings/install_attributes.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "components/policy/core/common/cloud/device_management_service.h"
#include "content/public/browser/navigation_handle.h"
#include "net/url_request/url_request_context_getter.h"

namespace em = enterprise_management;

namespace {

const char kSamlAuthErrorMessage[] = "Saml authentication failed. ";

policy::DeviceManagementService* GetDeviceManagementService() {
  policy::BrowserPolicyConnectorChromeOS* const connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  return connector->device_management_service();
}

std::string GetClientId() {
  policy::BrowserPolicyConnectorChromeOS* const connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  return connector->GetInstallAttributes()->GetDeviceId();
}

}  // namespace

namespace arc {

ArcActiveDirectoryEnrollmentTokenFetcher::
    ArcActiveDirectoryEnrollmentTokenFetcher()
    : weak_ptr_factory_(this) {}

ArcActiveDirectoryEnrollmentTokenFetcher::
    ~ArcActiveDirectoryEnrollmentTokenFetcher() = default;

void ArcActiveDirectoryEnrollmentTokenFetcher::Fetch(
    const FetchCallback& callback) {
  DCHECK(callback_.is_null());
  callback_ = callback;
  dm_token_storage_ = base::MakeUnique<policy::DMTokenStorage>(
      g_browser_process->local_state());
  dm_token_storage_->RetrieveDMToken(
      base::Bind(&ArcActiveDirectoryEnrollmentTokenFetcher::OnDMTokenAvailable,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ArcActiveDirectoryEnrollmentTokenFetcher::OnDMTokenAvailable(
    const std::string& dm_token) {
  if (dm_token.empty()) {
    LOG(ERROR) << "Retrieving the DMToken failed.";
    base::ResetAndReturn(&callback_)
        .Run(Status::FAILURE, std::string(), std::string());
    return;
  }

  DCHECK(dm_token_.empty());
  dm_token_ = dm_token;
  DoFetchEnrollmentToken();
}

void ArcActiveDirectoryEnrollmentTokenFetcher::DoFetchEnrollmentToken() {
  DCHECK(!dm_token_.empty());
  DCHECK(!fetch_request_job_);

  policy::DeviceManagementService* service = GetDeviceManagementService();
  fetch_request_job_.reset(
      service->CreateJob(policy::DeviceManagementRequestJob::
                             TYPE_ACTIVE_DIRECTORY_ENROLL_PLAY_USER,
                         g_browser_process->system_request_context()));

  fetch_request_job_->SetDMToken(dm_token_);
  fetch_request_job_->SetClientID(GetClientId());
  em::ActiveDirectoryEnrollPlayUserRequest* enroll_request =
      fetch_request_job_->GetRequest()
          ->mutable_active_directory_enroll_play_user_request();
  if (!auth_session_id_.empty()) {
    // Happens after going through SAML flow. Call DM server again with the
    // given |auth_session_id_|.
    enroll_request->set_auth_session_id(auth_session_id_);
    auth_session_id_.clear();
  }

  fetch_request_job_->Start(
      base::Bind(&ArcActiveDirectoryEnrollmentTokenFetcher::
                     OnFetchEnrollmentTokenCompleted,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ArcActiveDirectoryEnrollmentTokenFetcher::OnFetchEnrollmentTokenCompleted(
    policy::DeviceManagementStatus dm_status,
    int net_error,
    const em::DeviceManagementResponse& response) {
  fetch_request_job_.reset();

  Status fetch_status;
  std::string enrollment_token;
  std::string user_id;

  switch (dm_status) {
    case policy::DM_STATUS_SUCCESS: {
      if (!response.has_active_directory_enroll_play_user_response()) {
        LOG(WARNING) << "Invalid Active Directory enroll Play user response.";
        fetch_status = Status::FAILURE;
        break;
      }
      const em::ActiveDirectoryEnrollPlayUserResponse& enroll_response =
          response.active_directory_enroll_play_user_response();

      if (enroll_response.has_saml_parameters()) {
        // SAML authentication required.
        DCHECK(!enroll_response.has_enrollment_token());
        const em::SamlParametersProto& saml_params =
            enroll_response.saml_parameters();
        auth_session_id_ = saml_params.auth_session_id();
        InitiateSamlFlow(saml_params.auth_redirect_url());
        return;  // Saml flow eventually calls |callback_| or this function.
      }

      DCHECK(enroll_response.has_enrollment_token());
      fetch_status = Status::SUCCESS;
      enrollment_token = enroll_response.enrollment_token();
      user_id = enroll_response.user_id();
      break;
    }
    case policy::DM_STATUS_SERVICE_ARC_DISABLED:
      fetch_status = Status::ARC_DISABLED;
      break;
    default:  // All other error cases
      LOG(ERROR) << "Fetching an enrollment token failed. DM Status: "
                 << dm_status;
      fetch_status = Status::FAILURE;
      break;
  }

  dm_token_.clear();
  base::ResetAndReturn(&callback_).Run(fetch_status, enrollment_token, user_id);
}

void ArcActiveDirectoryEnrollmentTokenFetcher::InitiateSamlFlow(
    const std::string& auth_redirect_url) {
  // LOG(ERROR) << "Initiating SAML authentication. Redirect URL: '"
  //           << auth_redirect_url << "'.";

  // Check if URL is valid.
  GURL url(auth_redirect_url);
  if (!url.is_valid()) {
    LOG(ERROR) << kSamlAuthErrorMessage << "Redirect URL invalid.";
    CancelSamlFlow();
    return;
  }

  // Open a browser window and navigate to the URL.
  Profile* const profile = ArcSessionManager::Get()->profile();
  if (!profile) {
    LOG(ERROR) << kSamlAuthErrorMessage << "No user profile found.";
    CancelSamlFlow();
    return;
  }
  chrome::ScopedTabbedBrowserDisplayer displayer(profile);
  CHECK(displayer.browser());
  content::WebContents* web_contents = chrome::AddSelectedTabWithURL(
      displayer.browser(), url, ui::PAGE_TRANSITION_LINK);

  // Since the ScopedTabbedBrowserDisplayer does not guarantee that the
  // browser will be shown on the active desktop, we ensure the visibility.
  multi_user_util::MoveWindowToCurrentDesktop(
      displayer.browser()->window()->GetNativeWindow());

  // Observe web contents to receive the DidFinishNavigation() event.
  Observe(web_contents);
}

void ArcActiveDirectoryEnrollmentTokenFetcher::CancelSamlFlow() {
  dm_token_.clear();
  auth_session_id_.clear();
  DCHECK(!callback_.is_null());
  base::ResetAndReturn(&callback_)
      .Run(Status::FAILURE, std::string(), std::string());
}

void ArcActiveDirectoryEnrollmentTokenFetcher::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    LOG(ERROR) << kSamlAuthErrorMessage << "Unexpected nagivation state.";
    CancelSamlFlow();
    return;
  }

  if (navigation_handle->GetNetErrorCode() != net::OK) {
    LOG(ERROR) << kSamlAuthErrorMessage << "Got net error code '"
               << navigation_handle->GetNetErrorCode() << "'.";
    CancelSamlFlow();
    return;
  }

  policy::DeviceManagementService* service = GetDeviceManagementService();
  GURL server_url(service->GetServerUrl());
  const GURL& url = navigation_handle->GetURL();

  // LOG(ERROR) << "SAML authentication checkpoint '" << url << "'.";

  if (url.scheme() == server_url.scheme() && url.host() == server_url.host() &&
      url.path() == server_url.path()) {
    // Fetch enrollment token again, this time with non-empty session id.
    // LOG(ERROR) << "SAML authentication succeeded. Fetching token again.";
    DCHECK(!auth_session_id_.empty());
    DoFetchEnrollmentToken();
  }
}

}  // namespace arc
