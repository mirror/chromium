// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/interstitials/web_interstitial_impl.h"

#import "ios/web/navigation/navigation_manager_impl.h"
#import "ios/web/public/interstitials/web_interstitial_delegate.h"
#import "ios/web/public/test/fakes/test_navigation_manager.h"
#include "ios/web/public/test/fakes/test_web_state_observer.h"
#import "ios/web/public/test/web_test_with_web_state.h"
#import "ios/web/public/web_state/ui/crw_generic_content_view.h"
#import "ios/web/web_state/web_state_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const char kTestHostName[] = "https://chromium.test/";

// A web interstitial delegate which exposes called delegate methods.
class FakeInterstitialDelegate : public web::WebInterstitialDelegate {
 public:
  ~FakeInterstitialDelegate() override = default;

  // web::WebInterstitialDelegate overrides
  void OnProceed() override { on_proceed_called_ = true; }
  void OnDontProceed() override { on_dont_proceed_called_ = true; }
  void OverrideItem(web::NavigationItem* item) override {}

  // Whether or not |OnProceed()| has been called.
  bool on_proceed_called_ = false;
  // Whether or not |OnDontProceed()| has been called.
  bool on_dont_proceed_called_ = false;
};

// A web interstitial which displays an empty view.
class FakeWebInterstitial : public web::WebInterstitialImpl {
 public:
  FakeWebInterstitial(web::WebStateImpl* web_state,
                      bool new_navigation,
                      const GURL& url,
                      web::WebInterstitialDelegate* delegate)
      : web::WebInterstitialImpl(web_state, new_navigation, url),
        delegate_(delegate) {}

  ~FakeWebInterstitial() override = default;

  CRWContentView* GetContentView() const override {
    return content_view_.get();
  }

 protected:
  void PrepareForDisplay() override {
    if (!content_view_) {
      content_view_.reset(
          [[CRWGenericContentView alloc] initWithView:[[UIView alloc] init]]);
    }
  }

  web::WebInterstitialDelegate* GetDelegate() const override {
    return delegate_;
  }

  void ExecuteJavaScript(
      NSString* script,
      web::JavaScriptResultBlock completion_handler) override {}

 private:
  // The transient content view containing interstitial content.
  base::scoped_nsobject<CRWContentView> content_view_;
  // The delegate for the interstitial.
  web::WebInterstitialDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(FakeWebInterstitial);
};

// Creates and returns a fake interstitial for testing.
FakeWebInterstitial* CreateFakeWebInterstitial(
    web::WebStateImpl* web_state,
    web::WebInterstitialDelegate* delegate) {
  GURL url(kTestHostName);
  return new FakeWebInterstitial(web_state, true, url, delegate);
}

}  // namespace

// Test fixture for WebInterstitialImpl class.
class WebInterstitialImplTest : public web::WebTest {
 protected:
  void SetUp() override {
    WebTest::SetUp();
    web::WebState::CreateParams params(GetBrowserState());
    web_state_ = base::MakeUnique<web::WebStateImpl>(params);
    web_state_->GetNavigationManagerImpl().InitializeSession();

    // Transient item can only be added for pending non-app-specific loads.
    web_state_->GetNavigationManagerImpl().AddPendingItem(
        GURL(kTestHostName), web::Referrer(),
        ui::PageTransition::PAGE_TRANSITION_TYPED,
        web::NavigationInitiationType::USER_INITIATED,
        web::NavigationManager::UserAgentOverrideOption::INHERIT);
  }

  void TearDown() override {
    web_state_.reset();
    WebTest::TearDown();
  }

  std::unique_ptr<web::WebStateImpl> web_state_;
};

TEST_F(WebInterstitialImplTest, Proceed) {
  ASSERT_FALSE(web_state_->IsShowingWebInterstitial());

  std::unique_ptr<FakeInterstitialDelegate> delegate =
      base::MakeUnique<FakeInterstitialDelegate>();
  FakeWebInterstitial* interstitial =
      CreateFakeWebInterstitial(web_state_.get(), delegate.get());
  interstitial->Show();

  ASSERT_TRUE(web_state_->IsShowingWebInterstitial());

  interstitial->Proceed();
  EXPECT_TRUE(delegate->on_proceed_called_);
  EXPECT_FALSE(web_state_->IsShowingWebInterstitial());
}

TEST_F(WebInterstitialImplTest, DontProceed) {
  ASSERT_FALSE(web_state_->IsShowingWebInterstitial());

  std::unique_ptr<FakeInterstitialDelegate> delegate =
      base::MakeUnique<FakeInterstitialDelegate>();
  FakeWebInterstitial* interstitial =
      CreateFakeWebInterstitial(web_state_.get(), delegate.get());

  interstitial->Show();
  ASSERT_TRUE(web_state_->IsShowingWebInterstitial());

  interstitial->DontProceed();
  EXPECT_TRUE(delegate->on_dont_proceed_called_);
  EXPECT_FALSE(web_state_->IsShowingWebInterstitial());
}

TEST_F(WebInterstitialImplTest, VisibleSecurityStateChanged) {
  web::TestWebStateObserver observer(web_state_.get());

  std::unique_ptr<FakeInterstitialDelegate> delegate =
      base::MakeUnique<FakeInterstitialDelegate>();
  FakeWebInterstitial* interstitial =
      CreateFakeWebInterstitial(web_state_.get(), delegate.get());

  interstitial->Show();
  ASSERT_TRUE(observer.did_change_visible_security_state_info());
  EXPECT_TRUE(web_state_->IsShowingWebInterstitial());

  interstitial->DontProceed();
}

TEST_F(WebInterstitialImplTest, WebStateDestroyed) {
  std::unique_ptr<FakeInterstitialDelegate> delegate =
      base::MakeUnique<FakeInterstitialDelegate>();
  FakeWebInterstitial* interstitial =
      CreateFakeWebInterstitial(web_state_.get(), delegate.get());

  interstitial->Show();
  EXPECT_TRUE(web_state_->IsShowingWebInterstitial());

  // Simulate a destroyed web state.
  web_state_.reset();

  // Interstitial should be dismissed if web state is destroyed.
  EXPECT_TRUE(delegate->on_dont_proceed_called_);
}
