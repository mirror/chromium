// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/pre_signin_policy_fetcher.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/chromeos_paths.h"
#include "chromeos/cryptohome/mock_homedir_methods.h"
#include "chromeos/dbus/mock_cryptohome_client.h"
#include "chromeos/dbus/mock_session_manager_client.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "components/policy/core/common/cloud/mock_cloud_policy_client.h"
#include "components/policy/core/common/cloud/policy_builder.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/policy/proto/cloud_policy.pb.h"
#include "crypto/rsa_private_key.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Invoke;
using ::testing::Return;
using ::testing::WithArg;
using ::testing::_;

namespace em = enterprise_management;

using RetrievePolicyResponseType =
    chromeos::SessionManagerClient::RetrievePolicyResponseType;

using testing::AllOf;
using testing::AnyNumber;
using testing::Eq;
using testing::Mock;
using testing::Property;
using testing::Return;
using testing::SaveArg;
using testing::SetArgPointee;
using testing::_;

namespace policy {

namespace {

const char kSanitizedUsername[] =
    "0123456789ABCDEF0123456789ABCDEF012345678@example.com";
// This path does not actually need to exist - PreSigninPolicyFetcher should
// just pass it from cryptohome to session_manager, so it is not a real path in
// our test.
const char kCachedHomepage[] = "http://cached.org";
const char kFreshHomepage[] = "http://fresh.org";

ACTION_P2(SendSanitizedUsername, call_status, sanitized_username) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(arg1, call_status, sanitized_username));
}

class PreSigninPolicyFetcherTest : public testing::Test {
 protected:
  PreSigninPolicyFetcherTest() = default;

  void SetUp() override {
    // Setup mock HomedirMethods - this is used by PreSigninPolicyFetcher to
    // perform the temporary cryptohome mount.
    // Ownership of mock_homedir_methods_ is passsed to
    // HomedirMethods::InitializeForTesting.
    mock_homedir_methods_ = new cryptohome::MockHomedirMethods;
    cryptohome::HomedirMethods::InitializeForTesting(mock_homedir_methods_);

    // Setup cryptohome_client_'s GetSanitizedUsername handling. We don't check
    // each call explicitly.
    EXPECT_CALL(cryptohome_client_, GetSanitizedUsername(cryptohome_id_, _))
        .Times(AnyNumber())
        .WillRepeatedly(SendSanitizedUsername(
            chromeos::DBUS_METHOD_CALL_SUCCESS, kSanitizedUsername));

    // Create a temporary directory where the user policy keys will live (these
    // are shared between session_manager and chrome through files) and set it
    // into PathService, so PreSigninPolicyFetcher will use it.
    ASSERT_TRUE(tmp_dir_.CreateUniqueTempDir());
    PathService::Override(chromeos::DIR_USER_POLICY_KEYS,
                          user_policy_keys_dir());

    auto cloud_policy_client = base::MakeUnique<MockCloudPolicyClient>();
    cloud_policy_client_ = cloud_policy_client.get();
    pre_signin_policy_fetcher_ =
        base::MakeUnique<policy::PreSigninPolicyFetcher>(
            &cryptohome_client_, &session_manager_client_,
            std::move(cloud_policy_client), account_id_, cryptohome_key_);
    cached_policy_.payload().mutable_homepagelocation()->set_value(
        kCachedHomepage);
    cached_policy_.Build();

    fresh_policy_.payload().mutable_homepagelocation()->set_value(
        kFreshHomepage);
    fresh_policy_.Build();
  }

  void TearDown() override {
    base::RunLoop().RunUntilIdle();
    cryptohome::HomedirMethods::Shutdown();
    mock_homedir_methods_ = nullptr;
  }

  void InstallPublicKey(const UserPolicyBuilder& policy) {
    const std::string public_key = policy.GetPublicSigningKeyAsString();
    ASSERT_FALSE(public_key.empty());
    StoreUserPolicyKey(public_key);
  }

  void StoreUserPolicyKey(const std::string& public_key) {
    ASSERT_TRUE(base::CreateDirectory(user_policy_key_file().DirName()));
    ASSERT_EQ(static_cast<int>(public_key.size()),
              base::WriteFile(user_policy_key_file(), public_key.data(),
                              public_key.size()));
  }

  base::FilePath user_policy_keys_dir() const {
    return tmp_dir_.GetPath().AppendASCII("var_run_user_policy");
  }

  base::FilePath user_policy_key_file() const {
    return user_policy_keys_dir()
        .AppendASCII(kSanitizedUsername)
        .AppendASCII("policy.pub");
  }

  // Expect that the hidden cryptohome mount will be attempted, and return
  // the passed |mount_error|.
  void ExpectTemporaryCryptohomeMount(cryptohome::MountError mount_error) {
    cryptohome::MountParameters expected_mount_params(false);
    expected_mount_params.hidden_mount = true;
    EXPECT_CALL(*mock_homedir_methods_,
                MountEx(cryptohome::Identification(account_id_),
                        cryptohome::Authorization(cryptohome_key_),
                        expected_mount_params, _))
        .WillOnce(WithArg<3>(Invoke(
            [mount_error](cryptohome::HomedirMethods::MountCallback callback) {
              callback.Run(true /* success */, mount_error,
                           std::string() /* mount_hash */);
            })));
  }

  // Expect that the temporary cryptohome mount will be attempted, and return
  // success.
  void ExpectTemporaryCryptohomeMount() {
    ExpectTemporaryCryptohomeMount(cryptohome::MOUNT_ERROR_NONE);
  }

  void ExpectRetrievePolicyForUserWithoutSession(
      const std::string& policy_blob) {
    EXPECT_CALL(session_manager_client_,
                RetrievePolicyForUserWithoutSession(cryptohome_id_, _))
        .WillOnce(WithArg<1>(Invoke(
            [policy_blob](chromeos::SessionManagerClient::RetrievePolicyCallback
                              callback) {
              callback.Run(policy_blob, RetrievePolicyResponseType::SUCCESS);
            })));
  }

  void PrepareMockCloudPolicyClient(const std::string& dm_token,
                                    const std::string& client_id) {
    EXPECT_CALL(*cloud_policy_client_, SetupRegistration(dm_token, client_id));
    EXPECT_CALL(*cloud_policy_client_, FetchPolicy());
  }

  void VerifyMockCloudPolicyClientExpectations() {
    // Verify that the expected method calls happened (see
    // PrepareMockCloudPolicyClient).
    Mock::VerifyAndClearExpectations(cloud_policy_client_);
    // Verify that the public key version from the cached policy has been passed
    // to CloudPolicyClient for the fresh request.
    EXPECT_TRUE(cloud_policy_client_->public_key_version_valid_);
    EXPECT_EQ(cloud_policy_client_->public_key_version_,
              cached_policy_.policy_data().public_key_version());
  }

  void OnPolicyRetrieved(
      PreSigninPolicyFetcher::PolicyFetchResult result,
      std::unique_ptr<enterprise_management::CloudPolicySettings>
          policy_payload) {
    ASSERT_FALSE(policy_retrieved_called_);
    policy_retrieved_called_ = true;
    obtained_policy_fetch_result_ = result;
    obtained_policy_payload_ = std::move(policy_payload);
  }

  void ExpectTemporaryCryptohomeUnmount() {
    EXPECT_CALL(cryptohome_client_, Unmount(_))
        .WillOnce(
            WithArg<0>(Invoke([](chromeos::BoolDBusMethodCallback callback) {
              callback.Run(
                  chromeos::DBusMethodCallStatus::DBUS_METHOD_CALL_SUCCESS,
                  true);
            })));
  }

  void ExecuteFetchPolicy() {
    pre_signin_policy_fetcher_->FetchPolicy(
        base::Bind(&PreSigninPolicyFetcherTest::OnPolicyRetrieved,
                   base::Unretained(this)));
    scoped_task_environment_.RunUntilIdle();
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_ = {
      base::test::ScopedTaskEnvironment::MainThreadType::UI};
  cryptohome::MockHomedirMethods* mock_homedir_methods_ = nullptr;
  chromeos::MockCryptohomeClient cryptohome_client_;
  chromeos::MockSessionManagerClient session_manager_client_;
  UserPolicyBuilder cached_policy_;
  UserPolicyBuilder fresh_policy_;
  const AccountId account_id_ =
      AccountId::FromUserEmail(PolicyBuilder::kFakeUsername);
  const cryptohome::Identification cryptohome_id_ =
      cryptohome::Identification(account_id_);
  const cryptohome::KeyDefinition cryptohome_key_ =
      cryptohome::KeyDefinition("secret",
                                std::string() /* label */,
                                cryptohome::PRIV_DEFAULT);

  MockCloudPolicyClient* cloud_policy_client_ = nullptr;
  std::unique_ptr<PreSigninPolicyFetcher> pre_signin_policy_fetcher_;

  bool policy_retrieved_called_ = false;
  PreSigninPolicyFetcher::PolicyFetchResult obtained_policy_fetch_result_;
  std::unique_ptr<enterprise_management::CloudPolicySettings>
      obtained_policy_payload_;

 private:
  base::ScopedTempDir tmp_dir_;

  DISALLOW_COPY_AND_ASSIGN(PreSigninPolicyFetcherTest);
};

// Test that we successfully determine that the user has no policy (unmanaged
// user). The cached policy fetch succeeds with NO_POLICY.
// PreSigninPolicyFetcher does not attempt to fetch fresh policy.
TEST_F(PreSigninPolicyFetcherTest, NoPolicy) {
  ExpectTemporaryCryptohomeMount();
  // session_manager's RetrievePolicy* methods signal that there is no policy by
  // passing an empty string as policy blob.
  ExpectRetrievePolicyForUserWithoutSession(std::string());
  ExpectTemporaryCryptohomeUnmount();

  ExecuteFetchPolicy();

  EXPECT_TRUE(policy_retrieved_called_);
  EXPECT_EQ(obtained_policy_fetch_result_,
            PreSigninPolicyFetcher::PolicyFetchResult::NO_POLICY);
  EXPECT_FALSE(obtained_policy_payload_);
}

// Test that PreSigninPolicyFetcher signals an error when the temporary
// cryptohome mount fails.
TEST_F(PreSigninPolicyFetcherTest, CryptohomeTemporaryMountError) {
  ExpectTemporaryCryptohomeMount(
      cryptohome::MountError::MOUNT_ERROR_KEY_FAILURE);

  ExecuteFetchPolicy();

  EXPECT_TRUE(policy_retrieved_called_);
  EXPECT_EQ(obtained_policy_fetch_result_,
            PreSigninPolicyFetcher::PolicyFetchResult::ERROR);
  EXPECT_FALSE(obtained_policy_payload_);
}

// Break the signature of cached policy. We expect that the cached policy
// fails to validate as a consequence and thus we get a
// PolicyFetchResult::ERROR as response. PreSigninPolicyFetcher will not
// attempt to fetch fresh policy in this case.
TEST_F(PreSigninPolicyFetcherTest, CachedPolicyFailsToValidate) {
  cached_policy_.policy().mutable_policy_data_signature()->append("garbage");

  StoreUserPolicyKey(cached_policy_.GetPublicSigningKeyAsString());

  ExpectTemporaryCryptohomeMount();
  ExpectRetrievePolicyForUserWithoutSession(cached_policy_.GetBlob());
  ExpectTemporaryCryptohomeUnmount();

  ExecuteFetchPolicy();

  EXPECT_TRUE(policy_retrieved_called_);
  EXPECT_EQ(obtained_policy_fetch_result_,
            PreSigninPolicyFetcher::PolicyFetchResult::ERROR);
  EXPECT_FALSE(obtained_policy_payload_);
}

// Don't call StoreUserPolicyKey - chrome won't find a cached policy key. We
// expect that the cached policy fails to validate and thus we get a
// PolicyFetchResult::ERROR as response. PreSigninPolicyFetcher will not
// attempt to fetch fresh policy in this case.
TEST_F(PreSigninPolicyFetcherTest, NoCachedPolicyKeyAccessible) {
  ExpectTemporaryCryptohomeMount();
  ExpectRetrievePolicyForUserWithoutSession(cached_policy_.GetBlob());
  ExpectTemporaryCryptohomeUnmount();

  ExecuteFetchPolicy();

  EXPECT_TRUE(policy_retrieved_called_);
  EXPECT_EQ(obtained_policy_fetch_result_,
            PreSigninPolicyFetcher::PolicyFetchResult::ERROR);
  EXPECT_FALSE(obtained_policy_payload_);
}

// Cached policy is available and validates. However, fresh policy fetch fails
// with a CloudPolicyClient error. Expect that PreSigninPolicyFetcher will
// report a PolicyFetchResult::SUCCESS and pass the cached policy to the
// callback.
TEST_F(PreSigninPolicyFetcherTest, FreshPolicyFetchFails) {
  StoreUserPolicyKey(cached_policy_.GetPublicSigningKeyAsString());

  ExpectTemporaryCryptohomeMount();
  ExpectRetrievePolicyForUserWithoutSession(cached_policy_.GetBlob());
  ExpectTemporaryCryptohomeUnmount();

  PrepareMockCloudPolicyClient(PolicyBuilder::kFakeToken,
                               PolicyBuilder::kFakeDeviceId);
  ExecuteFetchPolicy();

  VerifyMockCloudPolicyClientExpectations();

  // Fresh policy fetch fails with a CloudPolicyClient error.
  cloud_policy_client_->NotifyClientError();

  // Expect that we still get PolicyFetchResult::SUCCESS with the cached policy.
  EXPECT_TRUE(policy_retrieved_called_);
  EXPECT_EQ(obtained_policy_fetch_result_,
            PreSigninPolicyFetcher::PolicyFetchResult::SUCCESS);
  EXPECT_TRUE(obtained_policy_payload_);
  EXPECT_EQ(obtained_policy_payload_->homepagelocation().value(),
            kCachedHomepage);
}

// Cached policy is available and validates. However, fresh policy fetch fails
// with timeout. Expect that PreSigninPolicyFetcher will report a
// PolicyFetchResult::SUCCESS and pass the cached policy to the callback.
TEST_F(PreSigninPolicyFetcherTest, FreshPolicyFetchTimeout) {
  StoreUserPolicyKey(cached_policy_.GetPublicSigningKeyAsString());

  ExpectTemporaryCryptohomeMount();
  ExpectRetrievePolicyForUserWithoutSession(cached_policy_.GetBlob());
  ExpectTemporaryCryptohomeUnmount();

  PrepareMockCloudPolicyClient(PolicyBuilder::kFakeToken,
                               PolicyBuilder::kFakeDeviceId);
  ExecuteFetchPolicy();

  VerifyMockCloudPolicyClientExpectations();

  // Fresh policy fetch times out.
  pre_signin_policy_fetcher_->ForceTimeoutForTesting();

  // Expect that we still get PolicyFetchResult::SUCCESS with the cached policy.
  EXPECT_TRUE(policy_retrieved_called_);
  EXPECT_EQ(obtained_policy_fetch_result_,
            PreSigninPolicyFetcher::PolicyFetchResult::SUCCESS);
  EXPECT_TRUE(obtained_policy_payload_);
  EXPECT_EQ(obtained_policy_payload_->homepagelocation().value(),
            kCachedHomepage);
}

// Cached policy is available and validates. Fresh policy fetch is also
// successful, but the fresh policy fails to validate. Expect that
// PreSigninPolicyFetcher will report a PolicyFetchResult::SUCCESS and pass
// the cached policy to the callback.
TEST_F(PreSigninPolicyFetcherTest, FreshPolicyFetchFailsToValidate) {
  StoreUserPolicyKey(cached_policy_.GetPublicSigningKeyAsString());

  ExpectTemporaryCryptohomeMount();
  ExpectRetrievePolicyForUserWithoutSession(cached_policy_.GetBlob());
  ExpectTemporaryCryptohomeUnmount();

  PrepareMockCloudPolicyClient(PolicyBuilder::kFakeToken,
                               PolicyBuilder::kFakeDeviceId);
  ExecuteFetchPolicy();

  VerifyMockCloudPolicyClientExpectations();

  // Fresh policy fetch is successful but returns a policy blob with a broken
  // signature, so the fresh policy fails to validate.
  fresh_policy_.policy().mutable_policy_data_signature()->append("garbage");
  cloud_policy_client_->SetPolicy(dm_protocol::kChromeUserPolicyType,
                                  std::string() /* settings_entity_id */,
                                  fresh_policy_.policy());
  cloud_policy_client_->NotifyPolicyFetched();
  scoped_task_environment_.RunUntilIdle();

  // Expect that we still get a PolicyFetchResult::SUCCESS with cached policy.
  EXPECT_TRUE(policy_retrieved_called_);
  EXPECT_EQ(obtained_policy_fetch_result_,
            PreSigninPolicyFetcher::PolicyFetchResult::SUCCESS);
  EXPECT_TRUE(obtained_policy_payload_);
  EXPECT_EQ(obtained_policy_payload_->homepagelocation().value(),
            kCachedHomepage);
}

// Cached policy is available and validates. Fresh policy fetch is also
// successful and the fresh policy validates. Expect that
// PreSigninPolicyFetcher will report a PolicyFetchResult::SUCCESS and pass
// the fresh policy to the callback.
TEST_F(PreSigninPolicyFetcherTest, FreshPolicyFetchSuccess) {
  StoreUserPolicyKey(cached_policy_.GetPublicSigningKeyAsString());

  ExpectTemporaryCryptohomeMount();
  ExpectRetrievePolicyForUserWithoutSession(cached_policy_.GetBlob());
  ExpectTemporaryCryptohomeUnmount();

  PrepareMockCloudPolicyClient(PolicyBuilder::kFakeToken,
                               PolicyBuilder::kFakeDeviceId);
  ExecuteFetchPolicy();

  VerifyMockCloudPolicyClientExpectations();

  // Fresh policy fetch is successful and validates.
  cloud_policy_client_->SetPolicy(dm_protocol::kChromeUserPolicyType,
                                  std::string() /* settings_entity_id */,
                                  fresh_policy_.policy());
  cloud_policy_client_->NotifyPolicyFetched();
  scoped_task_environment_.RunUntilIdle();

  // Expect that we get PolicyFetchResult::SUCCESS with fresh policy.
  EXPECT_TRUE(policy_retrieved_called_);
  EXPECT_EQ(obtained_policy_fetch_result_,
            PreSigninPolicyFetcher::PolicyFetchResult::SUCCESS);
  EXPECT_TRUE(obtained_policy_payload_);
  EXPECT_EQ(obtained_policy_payload_->homepagelocation().value(),
            kFreshHomepage);
}

}  // namespace

}  // namespace policy
