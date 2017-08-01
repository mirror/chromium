// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker.h"

#include "base/run_loop.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/new_tab_promo.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/test/ui_controls.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace {

MATCHER_P(IsFeature, feature, "") {
  return arg.name == feature.name;
}

class FakeNewTabTracker : public feature_engagement::NewTabTracker {
 public:
  explicit FakeNewTabTracker(feature_engagement::Tracker* feature_tracker)
      : feature_tracker_(feature_tracker),
        pref_service_(
            base::MakeUnique<sync_preferences::TestingPrefServiceSyncable>()) {
    feature_engagement::NewTabTracker::RegisterProfilePrefs(
        pref_service_->registry());
  }

  // feature_engagement::NewTabTracker::
  feature_engagement::Tracker* GetFeatureTracker() override {
    return feature_tracker_;
  }

  PrefService* GetPrefs() override { return pref_service_.get(); }

 private:
  feature_engagement::Tracker* const feature_tracker_;
  const std::unique_ptr<sync_preferences::TestingPrefServiceSyncable>
      pref_service_;
};

class MockTracker : public feature_engagement::Tracker {
 public:
  MockTracker() = default;
  MOCK_METHOD1(NotifyEvent, void(const std::string& event));
  MOCK_METHOD1(ShouldTriggerHelpUI, bool(const base::Feature& feature));
  MOCK_METHOD1(GetTriggerState,
               Tracker::TriggerState(const base::Feature& feature));
  MOCK_METHOD1(Dismissed, void(const base::Feature& feature));
  MOCK_METHOD0(IsInitialized, bool());
  MOCK_METHOD1(AddOnInitializedCallback, void(OnInitializedCallback callback));
};

class NewTabTrackerBrowserTest : public InProcessBrowserTest {
 public:
  NewTabTrackerBrowserTest() = default;
  ~NewTabTrackerBrowserTest() override = default;

  void SetUpOnMainThread() override {
    feature_tracker_ = base::MakeUnique<testing::StrictMock<MockTracker>>();

    new_tab_tracker_ =
        base::MakeUnique<FakeNewTabTracker>(feature_tracker_.get());
  }

  GURL GetGoogleURL() { return GURL("http://www.google.com/"); }

  static void GetOmniboxViewForBrowser(const Browser* browser,
                                       OmniboxView** omnibox_view) {
    BrowserWindow* window = browser->window();
    ASSERT_TRUE(window);
    LocationBar* location_bar = window->GetLocationBar();
    ASSERT_TRUE(location_bar);
    *omnibox_view = location_bar->GetOmniboxView();
    ASSERT_TRUE(*omnibox_view);
  }

 protected:
  std::unique_ptr<FakeNewTabTracker> new_tab_tracker_;
  std::unique_ptr<MockTracker> feature_tracker_;
};

}  // namespace

IN_PROC_BROWSER_TEST_F(NewTabTrackerBrowserTest, TestShowPromo) {
  // Navigate in the omnibox.
  ui_test_utils::NavigateToURL(browser(), GetGoogleURL());
  OmniboxView* view = NULL;
  ASSERT_NO_FATAL_FAILURE(GetOmniboxViewForBrowser(browser(), &view));
  OmniboxViewViews* omnibox_view = static_cast<OmniboxViewViews*>(view);
  ui::ScopedClipboardWriter(ui::CLIPBOARD_TYPE_COPY_PASTE)
      .WriteText(base::ASCIIToUTF16("http://maps.google.com/"));
  omnibox_view->ExecuteCommand(IDS_PASTE_AND_GO, ui::EF_NONE);

  // Bypassing the 2 hour active session time requirement.
  EXPECT_CALL(*feature_tracker_,
              NotifyEvent(feature_engagement::events::kSessionTime));
  new_tab_tracker_->OnSessionTimeMet();

  // Focus on the omnibox.
  chrome::FocusLocationBar(browser());

  EXPECT_TRUE(BrowserView::GetBrowserViewForBrowser(browser())
                  ->tabstrip()
                  ->new_tab_button()
                  ->new_tab_promo()
                  ->GetWidget()
                  ->IsVisible());

  EXPECT_CALL(*feature_tracker_,
              Dismissed(IsFeature(feature_engagement::kIPHNewTabFeature)));

  BrowserView::GetBrowserViewForBrowser(browser())
      ->tabstrip()
      ->new_tab_button()
      ->new_tab_promo()
      ->GetWidget()
      ->Close();
}
