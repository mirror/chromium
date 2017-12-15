// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/engagement/app_engagement_recorder.h"

#include "base/metrics/histogram_macros.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/extensions/launch_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/extensions/api/url_handlers/url_handlers_parser.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "url/gurl.h"

namespace {

const char kAppWindowEngagementTypeHistogram[] =
    "PWAWindowEngagement.EngagementType";

// TODO(mgiuca): Copied from bookmark_app_navigation_throttle.cc. DO NOT SUBMIT.
bool IsWindowedBookmarkApp(const extensions::Extension* app,
                           content::BrowserContext* context) {
  if (!app || !app->from_bookmark())
    return false;

  if (GetLaunchContainer(extensions::ExtensionPrefs::Get(context), app) !=
      extensions::LAUNCH_CONTAINER_WINDOW) {
    return false;
  }

  return true;
}

void RecordAppWindowEngagement(SiteEngagementService::EngagementType type) {
  LOG(ERROR) << "PWAWindowEngagement.EngagementType: " << type;
  UMA_HISTOGRAM_ENUMERATION(kAppWindowEngagementTypeHistogram, type,
                            SiteEngagementService::ENGAGEMENT_LAST);
}

}  // namespace

AppEngagementRecorder::AppEngagementRecorder(SiteEngagementService* service)
    : SiteEngagementObserver(service) {}

AppEngagementRecorder::~AppEngagementRecorder() {}

void AppEngagementRecorder::OnEngagementEvent(
    content::WebContents* web_contents,
    const GURL& url,
    double score,
    SiteEngagementService::EngagementType type) {
  if (!web_contents) {
    // Not part of an app, so can't log anything.
    DVLOG(1) << "AppEngagementRecorder::OnEngagementEvent: No WebContents (\""
             << url << "\")";
    return;
  }

  content::BrowserContext* context = web_contents->GetBrowserContext();
  // TODO(mgiuca): This might be very inefficient. DO NOT SUBMIT.
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
  if (!browser) {
    DVLOG(1) << "AppEngagementRecorder: No browser for web contents (\"" << url
             << "\")";
    return;
  }

  if (!browser->is_app()) {
    DVLOG(1) << "AppEngagementRecorder: Browser is not an app (\"" << url
             << "\")";
    return;
  }

  const extensions::Extension* app =
      extensions::ExtensionRegistry::Get(context)->GetExtensionById(
          web_app::GetExtensionIdFromApplicationName(browser->app_name()),
          extensions::ExtensionRegistry::ENABLED);

  if (!IsWindowedBookmarkApp(app, context)) {
    DVLOG(1) << "AppEngagementRecorder: App is not windowed/bookmark (\"" << url
             << "\")";
    return;
  }

  // Bookmark Apps for installable websites have scope.
  // TODO(crbug.com/774918): Replace once there is a more explicit indicator
  // of a Bookmark App for an installable website.
  if (extensions::UrlHandlers::GetUrlHandlers(app) == nullptr) {
    DVLOG(1) << "AppEngagementRecorder: No URL handlers (not a PWA) (\"" << url
             << "\")";
    return;
  }

  DVLOG(1) << "AppEngagementRecorder::OnEngagementEvent(<WebContents>, \""
           << url << "\", " << score << ", " << type << ") <-- IN A PWA WINDOW";

  RecordAppWindowEngagement(type);
}
