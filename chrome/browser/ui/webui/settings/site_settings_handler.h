// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_SITE_SETTINGS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_SITE_SETTINGS_HANDLER_H_

#include <vector>

#include "chrome/browser/storage/storage_info_fetcher.h"
#include "chrome/browser/ui/webui/settings/md_settings_ui.h"

class Profile;

namespace base {
class ListValue;
}

namespace settings {

// Chrome "ContentSettings" settings page UI handler.
class SiteSettingsHandler
     : public SettingsPageUIHandler, StorageInfoFetcher::Observer {
 public:
  explicit SiteSettingsHandler(Profile* profile);
  ~SiteSettingsHandler() override;

  // SettingsPageUIHandler:
  void RegisterMessages() override;

  // StorageInfoFetcher::Observer:
  void OnGetUsageInfo(const storage::UsageInfoEntries& entries) override;

 private:
  // Asynchronously fetches the usage for a given origin. Replies back with
  // OnGetUsageInfo above.
  void HandleFetchUsageTotal(const base::ListValue* args);

  Profile* profile_;

  // The host for which to fetch usage.
  std::string usage_host_;

  // The helper object asynchronously fetching storage info.
  scoped_refptr<StorageInfoFetcher> storage_info_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(SiteSettingsHandler);
};

}  // namespace settings

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_SITE_SETTINGS_HANDLER_H_
