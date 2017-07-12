// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/breaking_news/subscription_manager.h"

#include "base/message_loop/message_loop.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/ntp_snippets/remote/test_utils.h"
#include "components/prefs/testing_pref_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "google_apis/gaia/fake_oauth2_token_service_delegate.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_snippets {

const char kTestEmail[] = "test@email.com";
const std::string kSubscriptionUrl = "http://valid-url.test/subscribe";
const std::string kUnsubscriptionUrl = "http://valid-url.test/unsubscribe";

class SubscriptionManagerTest : public testing::Test {
 public:
  SubscriptionManagerTest()
      : request_context_getter_(
            new net::TestURLRequestContextGetter(message_loop_.task_runner())),
        fake_token_service_(base::MakeUnique<FakeProfileOAuth2TokenService>(
            base::MakeUnique<FakeOAuth2TokenServiceDelegate>(
                request_context_getter_.get()))) {}

  void SetUp() override {
    SubscriptionManager::RegisterProfilePrefs(
        utils_.pref_service()->registry());
  }

  scoped_refptr<net::URLRequestContextGetter> GetRequestContext() {
    return request_context_getter_.get();
  }

  PrefService* GetPrefService() { return utils_.pref_service(); }

  OAuth2TokenService* GetOAuth2TokenService() {
    return fake_token_service_.get();
  }

  SigninManagerBase* GetSigninManager() { return utils_.fake_signin_manager(); }

  net::TestURLFetcher* GetRunningFetcher() {
    // All created TestURLFetchers have ID 0 by default.
    net::TestURLFetcher* url_fetcher = url_fetcher_factory_.GetFetcherByID(0);
    DCHECK(url_fetcher);
    return url_fetcher;
  }

  void RespondToSubscriptionRequestSuccessfully() {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    ASSERT_EQ(GURL(kSubscriptionUrl), url_fetcher->GetOriginalURL());
    RespondSuccessfully();
  }

  void RespondToUnsubscriptionRequestSuccessfully() {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    ASSERT_EQ(GURL(kUnsubscriptionUrl), url_fetcher->GetOriginalURL());
    RespondSuccessfully();
  }

  void RespondToSubscriptionWithError(int error_code) {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    ASSERT_EQ(GURL(kSubscriptionUrl), url_fetcher->GetOriginalURL());
    RespondWithError(error_code);
  }

  void RespondToUnsubscriptionWithError(int error_code) {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    ASSERT_EQ(GURL(kUnsubscriptionUrl), url_fetcher->GetOriginalURL());
    RespondWithError(error_code);
  }

  void SignIn() {
    utils_.fake_signin_manager()->SignIn(kTestEmail, "user", "pass");
  }

  void SignOut() { utils_.fake_signin_manager()->ForceSignOut(); }
  void IssueRefreshToken() {
    fake_token_service_->GetDelegate()->UpdateCredentials(kTestEmail, "token");
  }

  void IssueAccessToken() {
    fake_token_service_->IssueAllTokensForAccount(kTestEmail, "access_token",
                                                  base::Time::Max());
  }

  void CancelAccessTokenRequests() {
    fake_token_service_->IssueErrorForAllPendingRequestsForAccount(
        kTestEmail, GoogleServiceAuthError(
                        GoogleServiceAuthError::State::REQUEST_CANCELED));
  }

 private:
  void RespondSuccessfully() {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    url_fetcher->set_status(net::URLRequestStatus());
    url_fetcher->set_response_code(net::HTTP_OK);
    url_fetcher->SetResponseString(std::string());
    // Call the URLFetcher delegate to continue the test.
    url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
  }

  void RespondWithError(int error_code) {
    net::TestURLFetcher* url_fetcher = GetRunningFetcher();
    url_fetcher->set_status(net::URLRequestStatus::FromError(error_code));
    url_fetcher->SetResponseString(std::string());
    // Call the URLFetcher delegate to continue the test.
    url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
  }


  base::MessageLoop message_loop_;
  test::RemoteSuggestionsTestUtils utils_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
  net::TestURLFetcherFactory url_fetcher_factory_;
  std::unique_ptr<FakeProfileOAuth2TokenService> fake_token_service_;
};

TEST_F(SubscriptionManagerTest, SubscribeSuccessfully) {
  std::string token = "1234567890";
  SubscriptionManager manager(GetRequestContext(), GetPrefService(),
                              GetSigninManager(), GetOAuth2TokenService(),
                              GURL(kSubscriptionUrl), GURL(kUnsubscriptionUrl));
  manager.Subscribe(token);
  RespondToSubscriptionRequestSuccessfully();
  ASSERT_TRUE(manager.IsSubscribed());
  EXPECT_EQ(token, GetPrefService()->GetString(
                       prefs::kBreakingNewsSubscriptionDataToken));
  EXPECT_FALSE(GetPrefService()->GetBoolean(
      prefs::kBreakingNewsSubscriptionDataIsAuthenticated));
}

TEST_F(SubscriptionManagerTest,
       ShouldSubscribeWithAuthenticationWhenAuthenticated) {
  SignIn();
  IssueRefreshToken();

  std::string token = "1234567890";
  SubscriptionManager manager(GetRequestContext(), GetPrefService(),
                              GetSigninManager(), GetOAuth2TokenService(),
                              GURL(kSubscriptionUrl), GURL(kUnsubscriptionUrl));
  manager.Subscribe(token);
  EXPECT_FALSE(manager.IsSubscribed());
  IssueAccessToken();
  EXPECT_FALSE(manager.IsSubscribed());
  RespondToSubscriptionRequestSuccessfully();
  EXPECT_TRUE(manager.IsSubscribed());
  EXPECT_EQ(
      GetPrefService()->GetString(prefs::kBreakingNewsSubscriptionDataToken),
      token);
  EXPECT_TRUE(GetPrefService()->GetBoolean(
      prefs::kBreakingNewsSubscriptionDataIsAuthenticated));
}

TEST_F(SubscriptionManagerTest, ShouldNotSubscribeIfError) {
  std::string token = "1234567890";
  SubscriptionManager manager(GetRequestContext(), GetPrefService(),
                              GetSigninManager(), GetOAuth2TokenService(),
                              GURL(kSubscriptionUrl), GURL(kUnsubscriptionUrl));

  manager.Subscribe(token);
  RespondToSubscriptionWithError(net::ERR_TIMED_OUT);
  EXPECT_FALSE(manager.IsSubscribed());
}

TEST_F(SubscriptionManagerTest, UnsubscribeSuccessfully) {
  std::string token = "1234567890";
  SubscriptionManager manager(GetRequestContext(), GetPrefService(),
                              GetSigninManager(), GetOAuth2TokenService(),
                              GURL(kSubscriptionUrl), GURL(kUnsubscriptionUrl));
  manager.Subscribe(token);
  RespondToSubscriptionRequestSuccessfully();
  ASSERT_TRUE(manager.IsSubscribed());
  manager.Unsubscribe(token);
  RespondToUnsubscriptionRequestSuccessfully();
  EXPECT_FALSE(manager.IsSubscribed());
  EXPECT_FALSE(
      GetPrefService()->HasPrefPath(prefs::kBreakingNewsSubscriptionDataToken));
}

TEST_F(SubscriptionManagerTest,
       ShouldRemainSubscribedIfErrorDuringUnsubscribe) {
  std::string token = "1234567890";
  SubscriptionManager manager(GetRequestContext(), GetPrefService(),
                              GetSigninManager(), GetOAuth2TokenService(),
                              GURL(kSubscriptionUrl), GURL(kUnsubscriptionUrl));
  manager.Subscribe(token);
  RespondToSubscriptionRequestSuccessfully();
  ASSERT_TRUE(manager.IsSubscribed());
  manager.Unsubscribe(token);
  RespondToUnsubscriptionWithError(net::ERR_TIMED_OUT);
  ASSERT_TRUE(manager.IsSubscribed());
  EXPECT_EQ(token, GetPrefService()->GetString(
                       prefs::kBreakingNewsSubscriptionDataToken));
}

TEST_F(SubscriptionManagerTest, ShouldResubscribeIfSignInAfterSubscription) {
  std::string token = "1234567890";
  SubscriptionManager manager(GetRequestContext(), GetPrefService(),
                              GetSigninManager(), GetOAuth2TokenService(),
                              GURL(kSubscriptionUrl), GURL(kUnsubscriptionUrl));
  manager.Subscribe(token);
  RespondToSubscriptionRequestSuccessfully();
  EXPECT_FALSE(manager.NeedsToResubscribe());
  SignIn();
  IssueRefreshToken();
  EXPECT_TRUE(manager.NeedsToResubscribe());
  RespondToUnsubscriptionRequestSuccessfully();
  IssueAccessToken();
  RespondToSubscriptionRequestSuccessfully();
  EXPECT_TRUE(GetPrefService()->GetBoolean(
      prefs::kBreakingNewsSubscriptionDataIsAuthenticated));
}

TEST_F(SubscriptionManagerTest, ShouldResubscribeIfSignOutAfterSubscription) {
  SignIn();
  IssueRefreshToken();

  std::string token = "1234567890";
  SubscriptionManager manager(GetRequestContext(), GetPrefService(),
                              GetSigninManager(), GetOAuth2TokenService(),
                              GURL(kSubscriptionUrl), GURL(kUnsubscriptionUrl));
  manager.Subscribe(token);
  IssueAccessToken();
  RespondToSubscriptionRequestSuccessfully();
  SignOut();
  EXPECT_TRUE(manager.NeedsToResubscribe());
  RespondToUnsubscriptionRequestSuccessfully();
  RespondToSubscriptionRequestSuccessfully();
  EXPECT_FALSE(GetPrefService()->GetBoolean(
      prefs::kBreakingNewsSubscriptionDataIsAuthenticated));
}

}  // namespace ntp_snippets
