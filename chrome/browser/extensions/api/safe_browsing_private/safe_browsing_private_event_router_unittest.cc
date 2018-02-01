// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/extensions/api/safe_browsing_private/safe_browsing_private_event_router.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/mock_callback.h"
#include "base/values.h"
#include "chrome/browser/extensions/api/safe_browsing_private/safe_browsing_private_event_router_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/test_event_router.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

class SafeBrowsingEventObserver
    : public extensions::TestEventRouter::EventObserver {
 public:
  // The observer will only listen to events with the |event_name|.
  explicit SafeBrowsingEventObserver(const std::string& event_name)
      : event_name_(event_name) {}

  ~SafeBrowsingEventObserver() override = default;

  // Removes |event_args_| from |*this| and returns them.
  base::Value PassEventArgs() { return std::move(event_args_); }

  // extensions::TestEventRouter::EventObserver:
  void OnBroadcastEvent(const extensions::Event& event) override {
    if (event.event_name == event_name_) {
      event_args_ = event.event_args->Clone();
    }
  }

 private:
  // The name of the observed event.
  const std::string event_name_;

  // The arguments passed for the last observed event.
  base::Value event_args_;

  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingEventObserver);
};

std::unique_ptr<KeyedService> BuildSafeBrowsingPrivateEventRouter(
    content::BrowserContext* context) {
  return std::unique_ptr<KeyedService>(
      SafeBrowsingPrivateEventRouter::Create(context));
}

class SafeBrowsingPrivateEventRouterTest : public testing::Test {
 public:
  SafeBrowsingPrivateEventRouterTest() = default;
  ~SafeBrowsingPrivateEventRouterTest() override = default;

  void TriggerOnPolicySpecifiedPasswordReuseDetectedEvent() {
    SafeBrowsingPrivateEventRouterFactory::GetForProfile(&profile_)
        ->OnPolicySpecifiedPasswordReuseDetected(
            GURL("https://phishing.com/"), "user_name_1", /*is_phishing*/ true);
  }

  void TriggerOnPolicySpecifiedPasswordChangedEvent() {
    SafeBrowsingPrivateEventRouterFactory::GetForProfile(&profile_)
        ->OnPolicySpecifiedPasswordChanged("user_name_2");
  }

  void TriggerOnDangerousDownloadAllowedEvent() {
    SafeBrowsingPrivateEventRouterFactory::GetForProfile(&profile_)
        ->OnDangerousDownloadAllowed(GURL("https://evil.com/malware.exe"),
                                     "/path/to/malware.exe",
                                     "sha256_or_malware_exe", "user_name");
  }

  void TriggerOnDangerousDownloadOpenedEvent() {
    SafeBrowsingPrivateEventRouterFactory::GetForProfile(&profile_)
        ->OnDangerousDownloadOpened(GURL("https://evil.com/malware.exe"),
                                    "/path/to/malware.exe",
                                    "sha256_or_malware_exe", "user_name");
  }

  void TriggerOnSecurityInterstitialProceededEvent() {
    SafeBrowsingPrivateEventRouterFactory::GetForProfile(&profile_)
        ->OnSecurityInterstitialProceeded(GURL("https://phishing.com/"),
                                          "PHISHING", -1, "user_name");
  }

  void SetUpRouters() {
    event_router_ = extensions::CreateAndUseTestEventRouter(&profile_);
    SafeBrowsingPrivateEventRouterFactory::GetInstance()->SetTestingFactory(
        &profile_, BuildSafeBrowsingPrivateEventRouter);
  }

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  extensions::TestEventRouter* event_router_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingPrivateEventRouterTest);
};

TEST_F(SafeBrowsingPrivateEventRouterTest, TestOnReuseDetected) {
  SetUpRouters();
  SafeBrowsingEventObserver event_observer(
      api::safe_browsing_private::OnPolicySpecifiedPasswordReuseDetected::
          kEventName);
  event_router_->AddEventObserver(&event_observer);

  TriggerOnPolicySpecifiedPasswordReuseDetectedEvent();
  base::RunLoop().RunUntilIdle();

  auto captured_args = event_observer.PassEventArgs().GetList()[0].Clone();
  EXPECT_EQ("https://phishing.com/", captured_args.FindKey("url")->GetString());
  EXPECT_EQ("user_name_1", captured_args.FindKey("userName")->GetString());
  EXPECT_TRUE(captured_args.FindKey("sha256")->GetBool());
}

TEST_F(SafeBrowsingPrivateEventRouterTest, TestOnPasswordChanged) {
  SetUpRouters();
  SafeBrowsingEventObserver event_observer(
      api::safe_browsing_private::OnPolicySpecifiedPasswordChanged::kEventName);
  event_router_->AddEventObserver(&event_observer);

  TriggerOnPolicySpecifiedPasswordChangedEvent();
  base::RunLoop().RunUntilIdle();

  auto captured_args = event_observer.PassEventArgs().GetList()[0].Clone();
  EXPECT_EQ("user_name_2", captured_args.GetString());
}

TEST_F(SafeBrowsingPrivateEventRouterTest, TestOnDangerousDownloadAllowed) {
  SetUpRouters();
  SafeBrowsingEventObserver event_observer(
      api::safe_browsing_private::OnDangerousDownloadAllowed::kEventName);
  event_router_->AddEventObserver(&event_observer);

  TriggerOnDangerousDownloadAllowedEvent();
  base::RunLoop().RunUntilIdle();

  auto captured_args = event_observer.PassEventArgs().GetList()[0].Clone();
  EXPECT_EQ("https://evil.com/malware.exe",
            captured_args.FindKey("url")->GetString());
  EXPECT_EQ("/path/to/malware.exe",
            captured_args.FindKey("fileName")->GetString());
  EXPECT_EQ("user_name", captured_args.FindKey("userName")->GetString());
  EXPECT_EQ("sha256_or_malware_exe",
            captured_args.FindKey("sha256")->GetString());
}

TEST_F(SafeBrowsingPrivateEventRouterTest, TestOnDangerousDownloadOpened) {
  SetUpRouters();
  SafeBrowsingEventObserver event_observer(
      api::safe_browsing_private::OnDangerousDownloadOpened::kEventName);
  event_router_->AddEventObserver(&event_observer);

  TriggerOnDangerousDownloadOpenedEvent();
  base::RunLoop().RunUntilIdle();

  auto captured_args = event_observer.PassEventArgs().GetList()[0].Clone();
  EXPECT_EQ("https://evil.com/malware.exe",
            captured_args.FindKey("url")->GetString());
  EXPECT_EQ("/path/to/malware.exe",
            captured_args.FindKey("fileName")->GetString());
  EXPECT_EQ("user_name", captured_args.FindKey("userName")->GetString());
  EXPECT_EQ("sha256_or_malware_exe",
            captured_args.FindKey("sha256")->GetString());
}

TEST_F(SafeBrowsingPrivateEventRouterTest,
       TestOnSecurityInterstitialProceeded) {
  SetUpRouters();
  SafeBrowsingEventObserver event_observer(
      api::safe_browsing_private::OnSecurityInterstitialProceeded::kEventName);
  event_router_->AddEventObserver(&event_observer);

  TriggerOnSecurityInterstitialProceededEvent();
  base::RunLoop().RunUntilIdle();

  auto captured_args = event_observer.PassEventArgs().GetList()[0].Clone();
  EXPECT_EQ("https://phishing.com/", captured_args.FindKey("url")->GetString());
  EXPECT_EQ("PHISHING", captured_args.FindKey("reason")->GetString());
  EXPECT_EQ(-1, captured_args.FindKey("netErrorCode")->GetInt());
  EXPECT_EQ("user_name", captured_args.FindKey("userName")->GetString());
}

}  // namespace extensions
