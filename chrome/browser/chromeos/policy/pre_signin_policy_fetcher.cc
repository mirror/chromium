// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/pre_signin_policy_fetcher.h"

#include <stddef.h>

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/time/time.h"
#include "chromeos/chromeos_paths.h"
#include "chromeos/cryptohome/async_method_caller.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/cryptohome/homedir_methods.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "components/policy/proto/device_management_backend.pb.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "net/url_request/url_request_context_getter.h"

namespace em = enterprise_management;

namespace policy {

namespace {

// We will abort fresh policy fetch after this time and use cached policy.
const int kPolicyFetchTimeoutSecs = 10;

// Traits for the tasks posted in pre-signin policy fetch. As this blocks
// signin, the tasks have user-visible priority.
constexpr base::TaskTraits kTaskTraits = {base::MayBlock(),
                                          base::TaskPriority::USER_VISIBLE};
}  // namespace

PreSigninPolicyFetcher::PreSigninPolicyFetcher(
    chromeos::CryptohomeClient* cryptohome_client,
    chromeos::SessionManagerClient* session_manager_client,
    DeviceManagementService* device_management_service,
    scoped_refptr<net::URLRequestContextGetter> profile_request_context,
    const AccountId& account_id,
    const cryptohome::KeyDefinition& auth_key)
    : cryptohome_client_(cryptohome_client),
      session_manager_client_(session_manager_client),
      device_management_service_(device_management_service),
      profile_request_context_(profile_request_context),
      account_id_(account_id),
      auth_key_(auth_key),
      task_runner_(base::CreateSequencedTaskRunnerWithTraits(kTaskTraits)),
      weak_ptr_factory_(this) {}

PreSigninPolicyFetcher ::~PreSigninPolicyFetcher() {}

void PreSigninPolicyFetcher::FetchPolicy(
    PreSigninPolicyFetcher::PolicyFetchResultCallback callback) {
  DCHECK(callback_.is_null());
  callback_ = std::move(callback);

  VLOG(1) << "Step 1: Attempting temporary user home mount.";

  cryptohome::MountParameters mount(false);
  mount.hidden_mount = true;
  cryptohome::HomedirMethods::GetInstance()->MountEx(
      cryptohome::Identification(account_id_),
      cryptohome::Authorization(auth_key_), mount,
      base::Bind(&PreSigninPolicyFetcher::OnMountTemporaryUserHome,
                 weak_ptr_factory_.GetWeakPtr()));
}

void PreSigninPolicyFetcher::SetCloudPolicyClientForTesting(
    std::unique_ptr<CloudPolicyClient> cloud_policy_client) {
  client_ = std::move(cloud_policy_client);
}

void PreSigninPolicyFetcher::ForceTimeoutForTesting() {
  DCHECK(policy_fetch_timeout_.IsRunning());
  policy_fetch_timeout_.Stop();
  OnPolicyFetchTimeout();
}

void PreSigninPolicyFetcher::OnMountTemporaryUserHome(
    bool success,
    cryptohome::MountError return_code,
    const std::string& mount_hash) {
  if (!success || return_code != cryptohome::MOUNT_ERROR_NONE) {
    LOG(ERROR)
        << "Temporary user home mount failed, no pre-signin policy available.";
    NotifyCallback(PolicyFetchResult::POLICY_FETCH_ERROR, nullptr);
    return;
  }
  VLOG(1) << "Step 2: Reading cached policy from session manager.";
  session_manager_client_->RetrievePolicyForUserWithoutSession(
      cryptohome::Identification(account_id_),
      base::Bind(&PreSigninPolicyFetcher::OnCachedPolicyRetrieved,
                 weak_ptr_factory_.GetWeakPtr()));
}

void PreSigninPolicyFetcher::OnCachedPolicyRetrieved(
    const std::string& policy_blob,
    RetrievePolicyResponseType retrieve_policy_response) {
  // We only need the cached policy key if there was policy.
  if (!policy_blob.empty()) {
    VLOG(1) << "Loading cached policy key.";
    base::FilePath policy_key_dir;
    CHECK(PathService::Get(chromeos::DIR_USER_POLICY_KEYS, &policy_key_dir));
    cached_policy_key_loader_ = base::MakeUnique<CachedPolicyKeyLoaderChromeOS>(
        cryptohome_client_, task_runner_, account_id_, policy_key_dir);
    cached_policy_key_loader_->EnsurePolicyKeyLoaded(base::Bind(
        &PreSigninPolicyFetcher::OnPolicyKeyLoaded,
        weak_ptr_factory_.GetWeakPtr(), policy_blob, retrieve_policy_response));
  } else {
    // Skip and pretend we've loaded policy key. We won't need it anyway,
    // because there is no policy to validate.
    OnPolicyKeyLoaded(policy_blob, retrieve_policy_response);
  }
}

void PreSigninPolicyFetcher::OnPolicyKeyLoaded(
    const std::string& policy_blob,
    RetrievePolicyResponseType retrieve_policy_response) {
  VLOG(1) << "Step 3: Unmounting temporary mount point";
  cryptohome_client_->Unmount(base::Bind(
      &PreSigninPolicyFetcher::OnUnmountTemporaryUserHome,
      weak_ptr_factory_.GetWeakPtr(), policy_blob, retrieve_policy_response));
}

void PreSigninPolicyFetcher::OnUnmountTemporaryUserHome(
    const std::string& policy_blob,
    RetrievePolicyResponseType retrieve_policy_response,
    chromeos::DBusMethodCallStatus unmount_call_status,
    bool unmount_success) {
  if (unmount_call_status != chromeos::DBUS_METHOD_CALL_SUCCESS ||
      !unmount_success) {
    LOG(ERROR) << "Couldn't unmount temporary mount point";
  }

  if (retrieve_policy_response != RetrievePolicyResponseType::SUCCESS) {
    LOG(ERROR) << "Retrieving policy from session manager failed.";
    NotifyCallback(PolicyFetchResult::POLICY_FETCH_ERROR, nullptr);
    return;
  }

  if (policy_blob.empty()) {
    VLOG(1) << "No cached policy.";
    NotifyCallback(PolicyFetchResult::POLICY_FETCH_NO_POLICY, nullptr);
    return;
  }

  // Parse policy
  std::unique_ptr<em::PolicyFetchResponse> policy =
      base::MakeUnique<em::PolicyFetchResponse>();
  if (!policy->ParseFromString(policy_blob)) {
    LOG(ERROR) << "Error parsing cached policy.";
    NotifyCallback(PolicyFetchResult::POLICY_FETCH_ERROR, nullptr);
    return;
  }

  // Before validating, check that we have a cached policy key.
  if (cached_policy_key_loader_->key().empty()) {
    LOG(ERROR) << "No cached policy key provided by session manager";
    NotifyCallback(PolicyFetchResult::POLICY_FETCH_ERROR, nullptr);
    return;
  }

  // Validate policy from session_manager
  UserCloudPolicyValidator::StartValidation(
      CreateValidatorForCachedPolicy(std::move(policy)),
      base::Bind(&PreSigninPolicyFetcher::OnCachedPolicyValidated,
                 weak_ptr_factory_.GetWeakPtr()));
}

void PreSigninPolicyFetcher::OnCachedPolicyValidated(
    UserCloudPolicyValidator* validator) {
  if (!validator->success()) {
    LOG(ERROR) << "Validation of cached policy failed.";
    NotifyCallback(PolicyFetchResult::POLICY_FETCH_ERROR, nullptr);
    return;
  }

  policy_data_ = std::move(validator->policy_data());
  policy_payload_ = std::move(validator->payload());

  if (account_id_.GetAccountType() == AccountType::ACTIVE_DIRECTORY) {
    // For AD, we don't support fresh policy fetch at the moment. Simply exit
    // with cached policy.
    NotifyCallback(PolicyFetchResult::POLICY_FETCH_SUCCESS,
                   std::move(policy_payload_));
  } else {
    // Try to retrieve fresh policy.
    if (!client_) {
      // Only create a CloudPolicyClient if we don't have one (for testing).
      client_ = base::MakeUnique<CloudPolicyClient>(
          std::string() /* machine_id */, std::string() /* machine_model */,
          device_management_service_, profile_request_context_,
          nullptr /* signing_service */);
    }
    client_->SetupRegistration(policy_data_->request_token(),
                               policy_data_->device_id());
    client_->AddPolicyTypeToFetch(dm_protocol::kChromeUserPolicyType,
                                  std::string() /* settings_entity_id */);
    if (policy_data_->has_public_key_version())
      client_->set_public_key_version(policy_data_->public_key_version());
    client_->AddObserver(this);

    // Start a timer that will limit how long we wait for fresh policy.
    policy_fetch_timeout_.Start(
        FROM_HERE, base::TimeDelta::FromSeconds(kPolicyFetchTimeoutSecs),
        base::Bind(&PreSigninPolicyFetcher::OnPolicyFetchTimeout,
                   weak_ptr_factory_.GetWeakPtr()));

    VLOG(1) << "Starting fresh policy fetch.";
    client_->FetchPolicy();
  }
}

void PreSigninPolicyFetcher::OnPolicyFetched(CloudPolicyClient* client) {
  policy_fetch_timeout_.Stop();

  const em::PolicyFetchResponse* fetched_policy =
      client_->GetPolicyFor(dm_protocol::kChromeUserPolicyType,
                            std::string() /* settings_entity_id */);
  if (!fetched_policy || !fetched_policy->has_policy_data()) {
    LOG(ERROR)
        << "Fresh policy could not be parsed, falling back to cached policy.";
    // policy_payload_ still holds caached policy loaded from session_manager.
    NotifyCallback(PolicyFetchResult::POLICY_FETCH_SUCCESS,
                   std::move(policy_payload_));
    return;
  }

  VLOG(0) << "Fresh policy fetch succeeded. Starting validation.";
  // Make a copy because there's currently no way to transfer ownership out of
  // CloudPolicyClient.
  std::unique_ptr<em::PolicyFetchResponse> fetched_policy_copy =
      base::MakeUnique<em::PolicyFetchResponse>(*fetched_policy);

  // Validate fresh policy.
  UserCloudPolicyValidator::StartValidation(
      CreateValidatorForFetchedPolicy(std::move(fetched_policy_copy)),
      base::Bind(&PreSigninPolicyFetcher::OnFetchedPolicyValidated,
                 weak_ptr_factory_.GetWeakPtr()));
}

void PreSigninPolicyFetcher::OnRegistrationStateChanged(
    CloudPolicyClient* client) {
  // Ignored.
}

void PreSigninPolicyFetcher::OnClientError(CloudPolicyClient* client) {
  policy_fetch_timeout_.Stop();
  LOG(ERROR) << "Fresh policy fetch failed, falling back to cached policy.";
  // policy_payload_ still holds caached policy loaded from session_manager.
  NotifyCallback(PolicyFetchResult::POLICY_FETCH_SUCCESS,
                 std::move(policy_payload_));
}

void PreSigninPolicyFetcher::OnPolicyFetchTimeout() {
  LOG(ERROR) << "Fresh policy fetch timed out, falling back to cached policy.";
  // Invalidate all weak pointrs so OnPolicyFetched is not called back anymore.
  weak_ptr_factory_.InvalidateWeakPtrs();
  NotifyCallback(PolicyFetchResult::POLICY_FETCH_SUCCESS,
                 std::move(policy_payload_));
}

void PreSigninPolicyFetcher::OnFetchedPolicyValidated(
    UserCloudPolicyValidator* validator) {
  if (!validator->success()) {
    LOG(ERROR) << "Validation of fetched policy failed, falling back to cached "
                  "policy.";
    NotifyCallback(PolicyFetchResult::POLICY_FETCH_SUCCESS,
                   std::move(policy_payload_));
    return;
  }

  VLOG(1) << "Fresh policy validation succeeded.";
  policy_data_ = std::move(validator->policy_data());
  policy_payload_ = std::move(validator->payload());
  NotifyCallback(PolicyFetchResult::POLICY_FETCH_SUCCESS,
                 std::move(policy_payload_));
}

void PreSigninPolicyFetcher::NotifyCallback(
    PolicyFetchResult result,
    std::unique_ptr<enterprise_management::CloudPolicySettings>
        policy_payload) {
  // Clean up instances created during the policy fetch procedure.
  cached_policy_key_loader_.reset();
  policy_data_.reset();
  if (client_) {
    client_->RemoveObserver(this);
    client_.reset();
  }

  DCHECK(callback_);
  std::move(callback_).Run(result, std::move(policy_payload));
}

std::unique_ptr<UserCloudPolicyValidator>
PreSigninPolicyFetcher::CreateValidatorForCachedPolicy(
    std::unique_ptr<enterprise_management::PolicyFetchResponse> policy) {
  std::unique_ptr<UserCloudPolicyValidator> validator =
      UserCloudPolicyValidator::Create(std::move(policy), task_runner_);

  validator->ValidatePolicyType(dm_protocol::kChromeUserPolicyType);
  validator->ValidatePayload();

  if (account_id_.GetAccountType() != AccountType::ACTIVE_DIRECTORY) {
    // Also validate the user e-mail and the signature (except for authpolicy).
    validator->ValidateUsername(account_id_.GetUserEmail(), true);
    validator->ValidateSignature(cached_policy_key_loader_->key());
  }
  return validator;
}

std::unique_ptr<UserCloudPolicyValidator>
PreSigninPolicyFetcher::CreateValidatorForFetchedPolicy(
    std::unique_ptr<enterprise_management::PolicyFetchResponse> policy) {
  // Configure the validator to validate based on cached policy.
  std::unique_ptr<UserCloudPolicyValidator> validator =
      UserCloudPolicyValidator::Create(std::move(policy), task_runner_);

  validator->ValidatePolicyType(dm_protocol::kChromeUserPolicyType);
  validator->ValidateAgainstCurrentPolicy(
      policy_data_.get(), CloudPolicyValidatorBase::TIMESTAMP_VALIDATED,
      CloudPolicyValidatorBase::DM_TOKEN_REQUIRED,
      CloudPolicyValidatorBase::DEVICE_ID_REQUIRED);
  validator->ValidatePayload();

  if (account_id_.GetAccountType() != AccountType::ACTIVE_DIRECTORY) {
    // Also validate the signature.
    std::string domain = gaia::ExtractDomainName(
        gaia::CanonicalizeEmail(account_id_.GetUserEmail()));
    validator->ValidateSignatureAllowingRotation(
        cached_policy_key_loader_->key(), domain);
  }
  return validator;
}

}  // namespace policy
