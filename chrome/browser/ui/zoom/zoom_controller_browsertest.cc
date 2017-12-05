// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zoom/zoom_controller.h"

#include "base/macros.h"
#include "base/process/kill.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/signin/login_ui_test_utils.h"
#include "chrome/browser/ui/zoom/chrome_zoom_level_prefs.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/metrics/metrics_switches.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/zoom/metrics/zoom_metrics_cache.h"
#include "components/zoom/test/zoom_test_utils.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_type.h"
#include "content/public/common/page_zoom.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/metrics_proto/zoom_event.pb.h"

using zoom::ZoomChangedWatcher;
using zoom::ZoomController;
using zoom::ZoomObserver;

class ZoomControllerBrowserTest : public InProcessBrowserTest {
 public:
  ZoomControllerBrowserTest() {}
  ~ZoomControllerBrowserTest() override {}

  void TestResetOnNavigation(ZoomController::ZoomMode zoom_mode) {
    DCHECK(zoom_mode == ZoomController::ZOOM_MODE_ISOLATED ||
           zoom_mode == ZoomController::ZOOM_MODE_MANUAL);
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(
        browser(), GURL("about:blank"), 1);
    ZoomController* zoom_controller =
        ZoomController::FromWebContents(web_contents);
    double zoom_level = zoom_controller->GetDefaultZoomLevel();
    zoom_controller->SetZoomMode(zoom_mode);

    // When the navigation occurs, the zoom_mode will be reset to
    // ZOOM_MODE_DEFAULT, and this will be reflected in the event that
    // is generated.
    ZoomController::ZoomChangedEventData zoom_change_data(
        web_contents, zoom_level, zoom_level, ZoomController::ZOOM_MODE_DEFAULT,
        false);
    ZoomChangedWatcher zoom_change_watcher(web_contents, zoom_change_data);

    ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUISettingsURL));
    zoom_change_watcher.Wait();
  }
};  // ZoomControllerBrowserTest

#if defined(OS_ANDROID)
#define MAYBE_CrashedTabsDoNotChangeZoom DISABLED_CrashedTabsDoNotChangeZoom
#else
#define MAYBE_CrashedTabsDoNotChangeZoom CrashedTabsDoNotChangeZoom
#endif
IN_PROC_BROWSER_TEST_F(ZoomControllerBrowserTest,
                       MAYBE_CrashedTabsDoNotChangeZoom) {
  // At the start of the test we are at a tab displaying about:blank.
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  ZoomController* zoom_controller =
      ZoomController::FromWebContents(web_contents);

  double old_zoom_level = zoom_controller->GetZoomLevel();
  double new_zoom_level = old_zoom_level + 0.5;

  content::RenderProcessHost* host = web_contents->GetMainFrame()->GetProcess();
  {
    content::RenderProcessHostWatcher crash_observer(
        host, content::RenderProcessHostWatcher::WATCH_FOR_PROCESS_EXIT);
    host->Shutdown(0, false);
    crash_observer.Wait();
  }
  EXPECT_FALSE(web_contents->GetRenderViewHost()->IsRenderViewLive());

  // The following attempt to change the zoom level for a crashed tab should
  // fail.
  zoom_controller->SetZoomLevel(new_zoom_level);
  EXPECT_FLOAT_EQ(old_zoom_level, zoom_controller->GetZoomLevel());
}

IN_PROC_BROWSER_TEST_F(ZoomControllerBrowserTest, OnPreferenceChanged) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  double new_default_zoom_level = 1.0;
  // Since this page uses the default zoom level, the changes to the default
  // zoom level will change the zoom level for this web_contents.
  ZoomController::ZoomChangedEventData zoom_change_data(
      web_contents,
      new_default_zoom_level,
      new_default_zoom_level,
      ZoomController::ZOOM_MODE_DEFAULT,
      false);
  ZoomChangedWatcher zoom_change_watcher(web_contents, zoom_change_data);
  // TODO(wjmaclean): Convert this to call partition-specific zoom level prefs
  // when they become available.
  browser()->profile()->GetZoomLevelPrefs()->SetDefaultZoomLevelPref(
      new_default_zoom_level);
  // Because this test relies on a round-trip IPC to/from the renderer process,
  // we need to wait for it to propagate.
  zoom_change_watcher.Wait();
}

IN_PROC_BROWSER_TEST_F(ZoomControllerBrowserTest, ErrorPagesCanZoom) {
  ui_test_utils::NavigateToURL(browser(), GURL("http://kjfhkjsdf.com"));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  ZoomController* zoom_controller =
      ZoomController::FromWebContents(web_contents);
  EXPECT_EQ(
      content::PAGE_TYPE_ERROR,
      web_contents->GetController().GetLastCommittedEntry()->GetPageType());

  double old_zoom_level = zoom_controller->GetZoomLevel();
  double new_zoom_level = old_zoom_level + 0.5;

  // The following attempt to change the zoom level for an error page should
  // fail.
  zoom_controller->SetZoomLevel(new_zoom_level);
  EXPECT_FLOAT_EQ(new_zoom_level, zoom_controller->GetZoomLevel());
}

IN_PROC_BROWSER_TEST_F(ZoomControllerBrowserTest,
                       ErrorPagesCanZoomAfterTabRestore) {
  // This url is meant to cause a network error page to be loaded.
  // Tests can't reach the network, so this test should continue
  // to work even if the domain listed is someday registered.
  GURL url("http://kjfhkjsdf.com");

  TabStripModel* tab_strip = browser()->tab_strip_model();
  ASSERT_TRUE(tab_strip);

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  {
    content::WebContents* web_contents = tab_strip->GetActiveWebContents();

    EXPECT_EQ(
        content::PAGE_TYPE_ERROR,
        web_contents->GetController().GetLastCommittedEntry()->GetPageType());

    content::WebContentsDestroyedWatcher destroyed_watcher(web_contents);
    tab_strip->CloseWebContentsAt(tab_strip->active_index(),
                                  TabStripModel::CLOSE_CREATE_HISTORICAL_TAB);
    destroyed_watcher.Wait();
  }
  EXPECT_EQ(1, tab_strip->count());

  content::WebContentsAddedObserver new_web_contents_observer;
  chrome::RestoreTab(browser());
  content::WebContents* web_contents =
      new_web_contents_observer.GetWebContents();
  content::WaitForLoadStop(web_contents);

  EXPECT_EQ(2, tab_strip->count());

  EXPECT_EQ(
      content::PAGE_TYPE_ERROR,
      web_contents->GetController().GetLastCommittedEntry()->GetPageType());

  ZoomController* zoom_controller =
      ZoomController::FromWebContents(web_contents);

  double old_zoom_level = zoom_controller->GetZoomLevel();
  double new_zoom_level = old_zoom_level + 0.5;

  // The following attempt to change the zoom level for an error page should
  // fail.
  zoom_controller->SetZoomLevel(new_zoom_level);
  EXPECT_FLOAT_EQ(new_zoom_level, zoom_controller->GetZoomLevel());
}

IN_PROC_BROWSER_TEST_F(ZoomControllerBrowserTest, Observe) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  double new_zoom_level = 1.0;
  // When the event is initiated from HostZoomMap, the old zoom level is not
  // available.
  ZoomController::ZoomChangedEventData zoom_change_data(
      web_contents,
      new_zoom_level,
      new_zoom_level,
      ZoomController::ZOOM_MODE_DEFAULT,
      false);  // The ZoomController did not initiate, so this will be 'false'.
  ZoomChangedWatcher zoom_change_watcher(web_contents, zoom_change_data);

  content::HostZoomMap* host_zoom_map =
      content::HostZoomMap::GetDefaultForBrowserContext(
          web_contents->GetBrowserContext());

  host_zoom_map->SetZoomLevelForHost("about:blank", new_zoom_level);
  zoom_change_watcher.Wait();
}

IN_PROC_BROWSER_TEST_F(ZoomControllerBrowserTest, ObserveDisabledModeEvent) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  ZoomController* zoom_controller =
      ZoomController::FromWebContents(web_contents);

  double default_zoom_level = zoom_controller->GetDefaultZoomLevel();
  double new_zoom_level = default_zoom_level + 1.0;
  zoom_controller->SetZoomLevel(new_zoom_level);

  ZoomController::ZoomChangedEventData zoom_change_data(
      web_contents,
      new_zoom_level,
      default_zoom_level,
      ZoomController::ZOOM_MODE_DISABLED,
      true);
  ZoomChangedWatcher zoom_change_watcher(web_contents, zoom_change_data);
  zoom_controller->SetZoomMode(ZoomController::ZOOM_MODE_DISABLED);
  zoom_change_watcher.Wait();
}

IN_PROC_BROWSER_TEST_F(ZoomControllerBrowserTest, PerTabModeResetSendsEvent) {
  TestResetOnNavigation(ZoomController::ZOOM_MODE_ISOLATED);
}

// Regression test: crbug.com/450909.
IN_PROC_BROWSER_TEST_F(ZoomControllerBrowserTest, NavigationResetsManualMode) {
  TestResetOnNavigation(ZoomController::ZOOM_MODE_MANUAL);
}

#if !defined(OS_CHROMEOS)
// Regression test: crbug.com/438979.
IN_PROC_BROWSER_TEST_F(ZoomControllerBrowserTest,
                       SettingsZoomAfterSigninWorks) {
  GURL signin_url(std::string(chrome::kChromeUIChromeSigninURL)
                      .append("?access_point=0&reason=0"));
  // We open the signin page in a new tab so that the ZoomController is
  // created against the HostZoomMap of the special StoragePartition that
  // backs the signin page. When we subsequently navigate away from the
  // signin page, the HostZoomMap changes, and we need to test that the
  // ZoomController correctly detects this.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), signin_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  login_ui_test_utils::WaitUntilUIReady(browser());
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_NE(
      content::PAGE_TYPE_ERROR,
      web_contents->GetController().GetLastCommittedEntry()->GetPageType());

  EXPECT_EQ(signin_url, web_contents->GetLastCommittedURL());
  ZoomController* zoom_controller =
      ZoomController::FromWebContents(web_contents);

  GURL settings_url(chrome::kChromeUISettingsURL);
  ui_test_utils::NavigateToURL(browser(), settings_url);
  EXPECT_NE(
      content::PAGE_TYPE_ERROR,
      web_contents->GetController().GetLastCommittedEntry()->GetPageType());

  // Verify new tab was created.
  EXPECT_EQ(2, browser()->tab_strip_model()->count());
  // Verify that the settings page is using the same WebContents.
  EXPECT_EQ(web_contents, browser()->tab_strip_model()->GetActiveWebContents());
  // TODO(wjmaclean): figure out why this next line fails, i.e. why does this
  // test not properly trigger a navigation to the settings page.
  EXPECT_EQ(settings_url, web_contents->GetLastCommittedURL());
  EXPECT_EQ(zoom_controller, ZoomController::FromWebContents(web_contents));

  // If we zoom the new page, it should still generate a ZoomController event.
  double old_zoom_level = zoom_controller->GetZoomLevel();
  double new_zoom_level = old_zoom_level + 0.5;

  ZoomController::ZoomChangedEventData zoom_change_data(
      web_contents,
      old_zoom_level,
      new_zoom_level,
      ZoomController::ZOOM_MODE_DEFAULT,
      true);  // We have a non-empty host, so this will be 'true'.
  ZoomChangedWatcher zoom_change_watcher(web_contents, zoom_change_data);
  zoom_controller->SetZoomLevel(new_zoom_level);
  zoom_change_watcher.Wait();
}
#endif  // !defined(OS_CHROMEOS)

// Used to test that when a user zooms, information about the action along with
// style information from the page are offered to the metrics service.
class ZoomMetricsBrowserTest : public InProcessBrowserTest {
 public:
  ZoomMetricsBrowserTest() {}
  ~ZoomMetricsBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(metrics::switches::kMetricsRecordingOnly);
  }

  // Perform a zoom on |page| to trigger collection of zoom metrics.
  void CollectMetricsForPage(const base::FilePath& page) {
    GURL url = ui_test_utils::GetTestUrl(base::FilePath(), page);
    ui_test_utils::NavigateToURL(browser(), url);

    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    ZoomController* zoom_controller =
        ZoomController::FromWebContents(web_contents);

    zoom_controller->GetDefaultZoomLevel();
    ASSERT_TRUE(content::ZoomValuesEqual(zoom_controller->GetDefaultZoomLevel(),
                                         content::ZoomFactorToZoomLevel(1.0)));

    ASSERT_TRUE(
        zoom::metrics::ZoomMetricsCache::GetInstance()->IsRecordingEnabled());
    zoom_controller->SkipMetricsDelaysForTesting();

    ZoomController::ZoomChangedEventData zoom_change_data(
        web_contents, content::ZoomFactorToZoomLevel(1.0),
        content::ZoomFactorToZoomLevel(1.1), ZoomController::ZOOM_MODE_DEFAULT,
        true);
    ZoomChangedWatcher zoom_change_watcher(web_contents, zoom_change_data);
    base::RunLoop wait_for_collection;
    zoom_controller->CollectionDoneNotificationForTesting(
        wait_for_collection.QuitClosure());

    chrome::Zoom(browser(), content::PAGE_ZOOM_IN);

    // Zooming will trigger the collection.
    zoom_change_watcher.Wait();
    wait_for_collection.Run();

    // The data is not made into an event for metrics until navigation away
    // from the zoomed page.
    ui_test_utils::NavigateToURL(browser(), GURL(url::kAboutBlankURL));
  }
};

IN_PROC_BROWSER_TEST_F(ZoomMetricsBrowserTest, CollectMetricsOnZoom) {
  CollectMetricsForPage(base::FilePath(FILE_PATH_LITERAL("title1.html")));

  std::vector<::metrics::ZoomEventProto> zoom_events;
  zoom::metrics::ZoomMetricsCache::GetInstance()->FlushZoomEvents(&zoom_events);
  ASSERT_EQ(1U, zoom_events.size());

  const ::metrics::ZoomEventProto& zoom_proto = zoom_events.at(0);

  // Several of these fields will vary by testing environment, so we just do
  // sanity checks for the values.

  ASSERT_TRUE(zoom_proto.has_zoom_factor());
  EXPECT_TRUE(content::ZoomValuesEqual(1.1, zoom_proto.zoom_factor()));
  ASSERT_TRUE(zoom_proto.has_default_zoom_factor());
  EXPECT_TRUE(content::ZoomValuesEqual(1.0, zoom_proto.default_zoom_factor()));

  EXPECT_TRUE(zoom_proto.has_visible_content_width());
  EXPECT_TRUE(zoom_proto.has_visible_content_height());
  EXPECT_TRUE(zoom_proto.has_contents_width());
  EXPECT_TRUE(zoom_proto.has_contents_height());
  EXPECT_TRUE(zoom_proto.has_preferred_width());

  ASSERT_EQ(1, zoom_proto.font_size_distribution_size());
  EXPECT_FLOAT_EQ(1.0, zoom_proto.font_size_distribution().at(16));

  EXPECT_FALSE(zoom_proto.has_largest_image_width());
  EXPECT_FALSE(zoom_proto.has_largest_image_height());
  EXPECT_FALSE(zoom_proto.has_largest_object_width());
  EXPECT_FALSE(zoom_proto.has_largest_object_height());

  ASSERT_TRUE(zoom_proto.has_text_area());
  EXPECT_GT(zoom_proto.text_area(), 0U);
  ASSERT_TRUE(zoom_proto.has_image_area());
  EXPECT_EQ(0U, zoom_proto.image_area());
  ASSERT_TRUE(zoom_proto.has_object_area());
  EXPECT_EQ(0U, zoom_proto.object_area());
}

IN_PROC_BROWSER_TEST_F(ZoomMetricsBrowserTest, IncludeIFrameContent) {
  CollectMetricsForPage(base::FilePath(FILE_PATH_LITERAL("iframe.html")));

  std::vector<::metrics::ZoomEventProto> zoom_events;
  zoom::metrics::ZoomMetricsCache::GetInstance()->FlushZoomEvents(&zoom_events);
  ASSERT_EQ(1U, zoom_events.size());

  const ::metrics::ZoomEventProto& zoom_proto = zoom_events.at(0);

  ASSERT_EQ(1, zoom_proto.font_size_distribution_size());
  EXPECT_FLOAT_EQ(1.0, zoom_proto.font_size_distribution().at(16));

  ASSERT_TRUE(zoom_proto.has_text_area());
  EXPECT_GT(zoom_proto.text_area(), 0U);
}

IN_PROC_BROWSER_TEST_F(ZoomMetricsBrowserTest, NontrivialPage) {
  CollectMetricsForPage(
      base::FilePath(FILE_PATH_LITERAL("google/google.html")));

  std::vector<::metrics::ZoomEventProto> zoom_events;
  zoom::metrics::ZoomMetricsCache::GetInstance()->FlushZoomEvents(&zoom_events);
  ASSERT_EQ(1U, zoom_events.size());

  const ::metrics::ZoomEventProto& zoom_proto = zoom_events.at(0);

  // Character counts from manual inspection of the test page.
  const int num_10px = 52;
#if defined(OS_MACOSX)
  // Mac uses a different font size for the buttons.
  const int num_11px = 30;
  const int num_13px = 213;
  const int total_chars = num_10px + num_11px + num_13px;
  ASSERT_EQ(3, zoom_proto.font_size_distribution_size());
  EXPECT_FLOAT_EQ(static_cast<float>(num_11px) / total_chars,
                  zoom_proto.font_size_distribution().at(11));
#else
  const int num_13px = 243;
  const int total_chars = num_10px + num_13px;
  ASSERT_EQ(2, zoom_proto.font_size_distribution_size());
#endif
  EXPECT_FLOAT_EQ(static_cast<float>(num_10px) / total_chars,
                  zoom_proto.font_size_distribution().at(10));
  EXPECT_FLOAT_EQ(static_cast<float>(num_13px) / total_chars,
                  zoom_proto.font_size_distribution().at(13));

  ASSERT_TRUE(zoom_proto.has_largest_image_width());
  EXPECT_GT(zoom_proto.largest_image_width(), 0U);
  ASSERT_TRUE(zoom_proto.has_largest_image_height());
  EXPECT_GT(zoom_proto.largest_image_height(), 0U);

  ASSERT_TRUE(zoom_proto.has_text_area());
  EXPECT_GT(zoom_proto.text_area(), 0U);
  ASSERT_TRUE(zoom_proto.has_image_area());
  EXPECT_GT(zoom_proto.image_area(), 0U);
}
