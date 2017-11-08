// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/dialogs/java_script_dialog_blocking_state.h"

#include "base/memory/ptr_util.h"
#include "ios/web/public/load_committed_details.h"
#include "ios/web/public/navigation_item.h"
#import "ios/web/public/test/fakes/test_navigation_context.h"
#import "ios/web/public/test/fakes/test_navigation_manager.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/web_state/web_state_observer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// TestNavigationContext sublcass that allows setting IsRendererInitiated() and
// IsSameDocument().
class JavaScriptBlockingTestNavigationContext
    : public web::TestNavigationContext {
 public:
  JavaScriptBlockingTestNavigationContext(bool renderer_initiated,
                                          bool same_document)
      : renderer_initiated_(renderer_initiated),
        same_document_(same_document) {}

  bool IsSameDocument() const override { return same_document_; }
  bool IsRendererInitiated() const override { return renderer_initiated_; }

 private:
  bool renderer_initiated_ = false;
  bool same_document_ = false;
};
// TestWebState subclass that allows simulating page loads.
class JavaScriptBlockingTestWebState : public web::TestWebState {
 public:
  JavaScriptBlockingTestWebState() : web::TestWebState() {
    last_committed_item_ = web::NavigationItem::Create();
    std::unique_ptr<web::TestNavigationManager> manager =
        base::MakeUnique<web::TestNavigationManager>();
    manager->SetLastCommittedItem(last_committed_item_.get());
    manager_ = manager.get();
    SetNavigationManager(std::move(manager));
  }
  ~JavaScriptBlockingTestWebState() override {
    observer_->WebStateDestroyed(this);
  }

  // Simulates a navigation by sending a WebStateObserver callback.
  void SimulateNavigationStarted(bool renderer_initiated,
                                 bool same_document,
                                 bool change_last_committed_item) {
    if (change_last_committed_item) {
      last_committed_item_ = web::NavigationItem::Create();
      manager_->SetLastCommittedItem(last_committed_item_.get());
    }
    JavaScriptBlockingTestNavigationContext context(renderer_initiated,
                                                    same_document);
    observer_->DidStartNavigation(this, &context);
  }

 protected:
  // WebState:
  void AddObserver(web::WebStateObserver* observer) override {
    // Only one observer is supported.
    ASSERT_EQ(observer_, nullptr);
    observer_ = observer;
  }

 private:
  web::WebStateObserver* observer_ = nullptr;
  web::TestNavigationManager* manager_ = nullptr;
  std::unique_ptr<web::NavigationItem> last_committed_item_;
};
}  // namespace

// Test fixture for testing JavaScriptDialogBlockingState.
class JavaScriptDialogBlockingStateTest : public PlatformTest {
 protected:
  JavaScriptDialogBlockingStateTest() : PlatformTest() {
    JavaScriptDialogBlockingState::CreateForWebState(&web_state_);
  }

  JavaScriptDialogBlockingState& state() {
    return *JavaScriptDialogBlockingState::FromWebState(&web_state_);
  }
  JavaScriptBlockingTestWebState& web_state() { return web_state_; }

 private:
  JavaScriptBlockingTestWebState web_state_;
};

// Tests that show_blocking_option() returns true after the first call to
// JavaScriptDialogWasShown() for a given presenter.
TEST_F(JavaScriptDialogBlockingStateTest, ShouldShow) {
  EXPECT_FALSE(state().show_blocking_option());
  state().JavaScriptDialogWasShown();
  EXPECT_TRUE(state().show_blocking_option());
}

// Tests that blocked() returns true after a call
// to JavaScriptDialogBlockingOptionSelected() for a given presenter.
TEST_F(JavaScriptDialogBlockingStateTest, BlockingOptionSelected) {
  EXPECT_FALSE(state().blocked());
  state().JavaScriptDialogBlockingOptionSelected();
  EXPECT_TRUE(state().blocked());
}

// Tests that blocked() returns false after a user-initiated or document-
// changing navigation.
TEST_F(JavaScriptDialogBlockingStateTest, StopBlocking) {
  EXPECT_FALSE(state().blocked());
  state().JavaScriptDialogBlockingOptionSelected();
  EXPECT_TRUE(state().blocked());
  web_state().SimulateNavigationStarted(false /* renderer_initiated */,
                                        false /* same_document */,
                                        true /* change_last_committed_item */);
  EXPECT_FALSE(state().blocked());
  state().JavaScriptDialogBlockingOptionSelected();
  EXPECT_TRUE(state().blocked());
  web_state().SimulateNavigationStarted(true /* renderer_initiated */,
                                        false /* same_document */,
                                        true /* change_last_committed_item */);
  EXPECT_FALSE(state().blocked());
  state().JavaScriptDialogBlockingOptionSelected();
  EXPECT_TRUE(state().blocked());
  web_state().SimulateNavigationStarted(false /* renderer_initiated */,
                                        true /* same_document */,
                                        true /* change_last_committed_item */);
  EXPECT_FALSE(state().blocked());
}

// Tests that blocked() returns true after a renderer-initiated, same-document
// navigation that doesn't change the last committed item.
TEST_F(JavaScriptDialogBlockingStateTest, ContinueBlocking) {
  EXPECT_FALSE(state().blocked());
  state().JavaScriptDialogBlockingOptionSelected();
  EXPECT_TRUE(state().blocked());
  web_state().SimulateNavigationStarted(true /* renderer_initiated */,
                                        true /* same_document */,
                                        false /* change_last_committed_item */);
  EXPECT_TRUE(state().blocked());
}
