// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/webshare/share_service_impl.h"

#include <algorithm>
#include <functional>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/webshare/share_target.h"
#include "chrome/browser/webshare/share_target_pref_helper.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/base/escape.h"

ShareServiceImpl::ShareServiceImpl() : weak_factory_(this) {}
ShareServiceImpl::~ShareServiceImpl() = default;

// static
void ShareServiceImpl::Create(
    const service_manager::BindSourceInfo& source_info,
    blink::mojom::ShareServiceRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<ShareServiceImpl>(),
                          std::move(request));
}

void ShareServiceImpl::ShowPickerDialog(
    std::vector<ShareTarget> targets,
    chrome::WebShareTargetPickerCallback callback) {
  // TODO(mgiuca): Get the browser window as |parent_window|.
  chrome::ShowWebShareTargetPickerDialog(
      nullptr /* parent_window */, std::move(targets), std::move(callback));
}

Browser* ShareServiceImpl::GetBrowser() {
  return BrowserList::GetInstance()->GetLastActive();
}

void ShareServiceImpl::OpenTargetURL(const GURL& target_url) {
  Browser* browser = GetBrowser();
  chrome::AddTabAt(browser, target_url,
                   browser->tab_strip_model()->active_index() + 1, true);
}

PrefService* ShareServiceImpl::GetPrefService() {
  return GetBrowser()->profile()->GetPrefs();
}

bool ShareServiceImpl::HasSufficientEngagement(const GURL& url) {
  // blink::mojom::EngagementLevel::LOW is the minimum required
  // engagement level.
  return GetEngagementLevel(url) >= blink::mojom::EngagementLevel::LOW;
}

blink::mojom::EngagementLevel ShareServiceImpl::GetEngagementLevel(
    const GURL& url) {
  SiteEngagementService* site_engagement_service =
      SiteEngagementService::Get(GetBrowser()->profile());
  return site_engagement_service->GetEngagementLevel(url);
}

void ShareServiceImpl::Share(const std::string& title,
                             const std::string& text,
                             const GURL& share_url,
                             const ShareCallback& callback) {
  std::vector<ShareTarget> share_targets =
      GetShareTargetsInPrefs(GetPrefService());

  std::remove_if(share_targets.begin(), share_targets.end(),
                 [this](const ShareTarget& target) {
                   return !HasSufficientEngagement(target.GetManifestURL());
                 });

  ShowPickerDialog(std::move(share_targets),
                   base::BindOnce(&ShareServiceImpl::OnPickerClosed,
                                  weak_factory_.GetWeakPtr(), title, text,
                                  share_url, callback));
}

void ShareServiceImpl::OnPickerClosed(const std::string& title,
                                      const std::string& text,
                                      const GURL& share_url,
                                      const ShareCallback& callback,
                                      const base::Optional<ShareTarget>& result) {
  if (!result.has_value()) {
    callback.Run(blink::mojom::ShareError::CANCELED);
    return;
  }

  bool replace_succeeded;
  GURL target;
  std::tie(replace_succeeded, target) = result->GetTargetURL(title, text, share_url);

  if (!replace_succeeded) {
    // TODO(mgiuca): This error should not be possible at share time, because
    // targets with invalid templates should not be chooseable. Fix
    // https://crbug.com/694380 and replace this with a DCHECK.
    callback.Run(blink::mojom::ShareError::INTERNAL_ERROR);
    return;
  }

  // User should not be able to cause an invalid target URL. Possibilities are:
  // - The base URL: can't be invalid since it's derived from the manifest URL.
  // - The template: can only be invalid if it contains a NUL character or
  //   invalid UTF-8 sequence (which it can't have).
  // - The replaced pieces: these are escaped.
  // If somehow we slip through this DCHECK, it will just open about:blank.
  DCHECK(target.is_valid());
  OpenTargetURL(target);

  callback.Run(blink::mojom::ShareError::OK);
}
