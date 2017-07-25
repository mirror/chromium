// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_UPDATE_OBSERVER_H_
#define CHROME_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_UPDATE_OBSERVER_H_

#include "base/macros.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings.mojom.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_binding_set.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content_settings {

// ContentSettingsUpdateObserver updates the browser of the lifetime of
// client hints for different origins based on IPC messages received from
// renderer::ContentSettingsObserver.
class ContentSettingsUpdateObserver
    : public content_settings::mojom::ContentSettings,
      public content::WebContentsUserData<ContentSettingsUpdateObserver> {
 public:
  explicit ContentSettingsUpdateObserver(content::WebContents* tab);
  ~ContentSettingsUpdateObserver() override;

 private:
  friend class content::WebContentsUserData<ContentSettingsUpdateObserver>;

  // mojom::ContentSettings implementation.
  void UpdateContentSetting(
      const ContentSettingsPattern& primary_pattern,
      std::unique_ptr<base::Value> expiration_times) override;

  content::WebContentsFrameBindingSet<content_settings::mojom::ContentSettings>
      binding_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingsUpdateObserver);
};

}  // namespace content_settings

#endif  // CHROME_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_UPDATE_OBSERVER_H_
