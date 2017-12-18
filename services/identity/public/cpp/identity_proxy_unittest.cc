// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/public/cpp/identity_proxy.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace identity {
namespace {

#if defined(OS_CHROMEOS)
using SigninManagerForTest = FakeSigninManagerBase;
#else
using SigninManagerForTest = FakeSigninManager;
#endif  // OS_CHROMEOS

const char kTestGaiaId[] = "dummyId";
const char kTestEmail[] = "me@dummy.com";
#if 0
const char kSecondaryTestGaiaId[] = "secondaryDummyId";
const char kSecondaryTestEmail[] = "metoo@dummy.com";
const char kTestRefreshToken[] = "dummy-refresh-token";
const char kTestAccessToken[] = "access_token";
#endif

class TestIdentityProxyObserver : IdentityProxy::Observer {
 public:
  TestIdentityProxyObserver(IdentityProxy* identity_proxy)
      : identity_proxy_(identity_proxy) {
    identity_proxy_->AddObserver(this);
  }
  ~TestIdentityProxyObserver() override {
    identity_proxy_->RemoveObserver(this);
  }

  void set_on_login_callback(base::OnceClosure callback) {
    on_login_callback_ = std::move(callback);
  }
  void set_on_logout_callback(base::OnceClosure callback) {
    on_logout_callback_ = std::move(callback);
  }

  const AccountInfo& account_info_on_login() { return account_info_on_login_; }
  const AccountInfo& account_info_on_logout() {
    return account_info_on_logout_;
  }

 private:
  // IdentityProxy::Observer:
  void OnPrimaryAccountLogin(const AccountInfo& primary_account_info) override {
    account_info_on_login_ = primary_account_info;
    if (on_login_callback_)
      std::move(on_login_callback_).Run();
  }
  void OnPrimaryAccountLogout(
      const AccountInfo& previous_primary_account_info) override {
    account_info_on_logout_ = previous_primary_account_info;
    if (on_logout_callback_)
      std::move(on_logout_callback_).Run();
  }

  IdentityProxy* identity_proxy_;
  base::OnceClosure on_login_callback_;
  base::OnceClosure on_logout_callback_;
  AccountInfo account_info_on_login_;
  AccountInfo account_info_on_logout_;
};

}  // namespace

class IdentityProxyTest : public testing::Test {
 public:
  IdentityProxyTest()
      : signin_client_(&pref_service_),
#if defined(OS_CHROMEOS)
        signin_manager_(&signin_client_, &account_tracker_)
#else
        signin_manager_(&signin_client_,
                        &token_service_,
                        &account_tracker_,
                        nullptr)
#endif
  {
    AccountTrackerService::RegisterPrefs(pref_service_.registry());
    SigninManagerBase::RegisterProfilePrefs(pref_service_.registry());
    SigninManagerBase::RegisterPrefs(pref_service_.registry());
    signin::RegisterAccountConsistencyProfilePrefs(pref_service_.registry());
    signin::SetGaiaOriginIsolatedCallback(
        base::BindRepeating([] { return true; }));

    account_tracker_.Initialize(&signin_client_);

    signin_manager()->SetAuthenticatedAccountInfo(kTestGaiaId, kTestEmail);

    identity_proxy_.reset(new IdentityProxy(&signin_manager_, &token_service_));
    identity_proxy_observer_.reset(
        new TestIdentityProxyObserver(identity_proxy_.get()));
  }

  IdentityProxy* identity_proxy() { return identity_proxy_.get(); }
  TestIdentityProxyObserver* identity_proxy_observer() {
    return identity_proxy_observer_.get();
  }
  AccountTrackerService* account_tracker() { return &account_tracker_; }
  SigninManagerForTest* signin_manager() { return &signin_manager_; }
  FakeProfileOAuth2TokenService* token_service() { return &token_service_; }

 private:
  base::MessageLoop message_loop_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  AccountTrackerService account_tracker_;
  TestSigninClient signin_client_;
  SigninManagerForTest signin_manager_;
  FakeProfileOAuth2TokenService token_service_;
  std::unique_ptr<IdentityProxy> identity_proxy_;
  std::unique_ptr<TestIdentityProxyObserver> identity_proxy_observer_;

  DISALLOW_COPY_AND_ASSIGN(IdentityProxyTest);
};

// Test that IdentityProxy starts off with the information in SigninManager.
TEST_F(IdentityProxyTest, PrimaryAccountInfoAtStartup) {
  AccountInfo primary_account_info = identity_proxy()->GetPrimaryAccountInfo();
  EXPECT_EQ(kTestGaiaId, primary_account_info.gaia);
  EXPECT_EQ(kTestEmail, primary_account_info.email);
}

// Signin/signout tests aren't relevant and cannot build on ChromeOS, which
// doesn't support signin/signout.
#if !defined(OS_CHROMEOS)
// Test that the user signing in results in firing of the IdentityProxy
// observer callback and the IdentityProxy's state being updated.
TEST_F(IdentityProxyTest, PrimaryAccountInfoAfterSignin) {
  base::RunLoop run_loop;
  identity_proxy_observer()->set_on_login_callback(run_loop.QuitClosure());

  signin_manager()->SignIn(kTestGaiaId, kTestEmail, "password");
  run_loop.Run();

  AccountInfo account_info_on_login =
      identity_proxy_observer()->account_info_on_login();
  EXPECT_EQ(kTestGaiaId, account_info_on_login.gaia);
  EXPECT_EQ(kTestEmail, account_info_on_login.email);

  AccountInfo primary_account_info = identity_proxy()->GetPrimaryAccountInfo();
  EXPECT_EQ(kTestGaiaId, primary_account_info.gaia);
  EXPECT_EQ(kTestEmail, primary_account_info.email);
}

// Test that the user signing out results in firing of the IdentityProxy
// observer callback and the IdentityProxy's state being updated.
TEST_F(IdentityProxyTest, PrimaryAccountInfoAfterSigninAndSignout) {
  // First ensure that the user is signed in from the POV of the IdentityProxy.
  base::RunLoop run_loop;
  identity_proxy_observer()->set_on_login_callback(run_loop.QuitClosure());
  signin_manager()->SignIn(kTestGaiaId, kTestEmail, "password");
  run_loop.Run();

  // Sign the user out and check that the IdentityProxy responds appropriately.
  base::RunLoop run_loop2;
  identity_proxy_observer()->set_on_logout_callback(run_loop2.QuitClosure());

  signin_manager()->ForceSignOut();
  run_loop2.Run();

  AccountInfo account_info_on_logout =
      identity_proxy_observer()->account_info_on_logout();
  EXPECT_EQ(kTestGaiaId, account_info_on_logout.gaia);
  EXPECT_EQ(kTestEmail, account_info_on_logout.email);

  AccountInfo primary_account_info = identity_proxy()->GetPrimaryAccountInfo();
  EXPECT_EQ("", primary_account_info.gaia);
  EXPECT_EQ("", primary_account_info.email);
}
#endif  // !defined(OS_CHROMEOS)

TEST_F(IdentityProxyTest, CreateAccessTokenFetcherForPrimaryAccount) {
  std::set<std::string> scopes{"scope"};
  PrimaryAccountTokenFetcher::TokenCallback callback =
      base::BindOnce([](const GoogleServiceAuthError& error,
                        const std::string& access_token) {});
  std::unique_ptr<PrimaryAccountTokenFetcher> token_fetcher =
      identity_proxy()->CreateAccessTokenFetcherForPrimaryAccount(
          "dummy_consumer", scopes, std::move(callback));
  EXPECT_TRUE(token_fetcher);
}

}  // namespace identity
