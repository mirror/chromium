// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/cryptauth_token_fetcher_impl.h"

#include "base/macros.h"
#include "base/stl_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace multidevice {

namespace {

const char kAccountId[] = "accountId";
const char kConsumerId[] = "consumerId";
const char kAccessToken[] = "accessToken";

class FakeIdentityManager : public identity::mojom::IdentityManager {
 public:
  FakeIdentityManager() = default;
  ~FakeIdentityManager() override = default;

  void InvokePendingCallbacks(const std::string& access_token) {
    for (auto& callback : pending_callbacks_) {
      std::move(callback).Run(access_token, base::Time(),
                              GoogleServiceAuthError());
    }

    pending_callbacks_.clear();
  }

  // identity::mojom::IdentityManager:
  void GetAccessToken(const std::string& account_id,
                      const identity::ScopeSet& scopes,
                      const std::string& consumer_id,
                      GetAccessTokenCallback callback) override {
    EXPECT_EQ(kAccountId, account_id);
    EXPECT_EQ(kConsumerId, consumer_id);
    EXPECT_TRUE(
        base::ContainsKey(scopes, "https://www.googleapis.com/auth/cryptauth"));
    pending_callbacks_.push_back(std::move(callback));
  }

  void GetPrimaryAccountInfo(GetPrimaryAccountInfoCallback callback) override {}

  void GetPrimaryAccountWhenAvailable(
      GetPrimaryAccountWhenAvailableCallback callback) override {}

  void GetAccountInfoFromGaiaId(
      const std::string& gaia_id,
      GetAccountInfoFromGaiaIdCallback callback) override {}

  void GetAccounts(GetAccountsCallback callback) override {}

 private:
  std::vector<GetAccessTokenCallback> pending_callbacks_;
};

}  // namespace

class MultiDeviceCryptAuthAccessTokenFetcherImplTest : public testing::Test {
 protected:
  MultiDeviceCryptAuthAccessTokenFetcherImplTest() {}

  void SetUp() override {
    fake_identity_manager_ = std::make_unique<FakeIdentityManager>();

    AccountInfo account_info;
    account_info.account_id = kAccountId;
    account_info.given_name = kConsumerId;

    token_fetcher_ = std::make_unique<CryptAuthAccessTokenFetcherImpl>(
        account_info, fake_identity_manager_.get());
  }

  void TearDown() override {
    // Deleting the object should not result in any additional tokens provided.
    token_fetcher_.reset();
    EXPECT_EQ(nullptr, GetTokenAndReset());

    last_access_token_.reset();
  }

  void FetchAccessToken() {
    token_fetcher_->FetchAccessToken(base::Bind(
        &MultiDeviceCryptAuthAccessTokenFetcherImplTest::OnAccessTokenFetched,
        base::Unretained(this)));
  }

  void OnAccessTokenFetched(const std::string& access_token) {
    last_access_token_ = std::make_unique<std::string>(access_token);
  }

  std::unique_ptr<std::string> GetTokenAndReset() {
    return std::move(last_access_token_);
  }

  std::unique_ptr<std::string> last_access_token_;

  std::unique_ptr<FakeIdentityManager> fake_identity_manager_;
  std::unique_ptr<CryptAuthAccessTokenFetcherImpl> token_fetcher_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MultiDeviceCryptAuthAccessTokenFetcherImplTest);
};

TEST_F(MultiDeviceCryptAuthAccessTokenFetcherImplTest, TestSuccess) {
  FetchAccessToken();
  EXPECT_EQ(nullptr, GetTokenAndReset());

  fake_identity_manager_->InvokePendingCallbacks(kAccessToken);
  EXPECT_EQ(kAccessToken, *GetTokenAndReset());
}

TEST_F(MultiDeviceCryptAuthAccessTokenFetcherImplTest, TestError) {
  FetchAccessToken();
  EXPECT_EQ(nullptr, GetTokenAndReset());

  fake_identity_manager_->InvokePendingCallbacks(std::string());
  EXPECT_EQ(std::string(), *GetTokenAndReset());
}

TEST_F(MultiDeviceCryptAuthAccessTokenFetcherImplTest,
       TestDeletedBeforeOperationFinished) {
  FetchAccessToken();
  EXPECT_EQ(nullptr, GetTokenAndReset());

  token_fetcher_.reset();

  // Deleting the object should result in the callback being invoked.
  EXPECT_EQ(std::string(), *GetTokenAndReset());
}

}  // namespace multidevice
