// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CLIENT_HINTS_CLIENT_HINTS_OBSERVER_H_
#define CHROME_BROWSER_CLIENT_HINTS_CLIENT_HINTS_OBSERVER_H_

#include "base/macros.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings.mojom.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_binding_set.h"
#include "content/public/browser/web_contents_user_data.h"

// ClientHintsObserver updates the browser of the lifetime of
// client hints for different origins based on IPC messages received from
// renderer::ContentSettingsObserver.
class ClientHintsObserver
    : public content_settings::mojom::ClientHints,
      public content::WebContentsUserData<ClientHintsObserver> {
 public:
  explicit ClientHintsObserver(content::WebContents* tab);
  ~ClientHintsObserver() override;

 private:
  friend class content::WebContentsUserData<ClientHintsObserver>;

  // mojom::ContentSettings implementation.
  void PersistClientHints(
      const GURL& primary_origin,
      std::unique_ptr<base::DictionaryValue> expiration_times) override;

  content::WebContentsFrameBindingSet<content_settings::mojom::ClientHints>
      binding_;

  DISALLOW_COPY_AND_ASSIGN(ClientHintsObserver);
};

#endif  // CHROME_BROWSER_CLIENT_HINTS_CLIENT_HINTS_OBSERVER_H_
