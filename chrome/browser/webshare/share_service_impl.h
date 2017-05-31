// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEBSHARE_SHARE_SERVICE_IMPL_H_
#define CHROME_BROWSER_WEBSHARE_SHARE_SERVICE_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "third_party/WebKit/public/platform/modules/webshare/webshare.mojom.h"
#include "third_party/WebKit/public/platform/site_engagement.mojom.h"

class ShareTarget;
class DictionaryValue;
class GURL;

// Desktop implementation of the ShareService Mojo service.
class ShareServiceImpl : public blink::mojom::ShareService {
 public:
  ShareServiceImpl();
  ~ShareServiceImpl() override;

  static void Create(const service_manager::BindSourceInfo& source_info,
                     mojo::InterfaceRequest<ShareService> request);

  // blink::mojom::ShareService overrides:
  void Share(const std::string& title,
             const std::string& text,
             const GURL& share_url,
             const ShareCallback& callback) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(ShareServiceImplUnittest, ReplacePlaceholders);

  Browser* GetBrowser();

  // Returns the URL template of the target identified by |target_url|
  std::string GetTargetTemplate(const std::string& target_url,
                                const base::DictionaryValue& share_targets);

  // Virtual for testing purposes.
  virtual PrefService* GetPrefService();

  // Returns true if the site with |url| has enough engagement to be a Share
  // Target.
  bool HasSufficientEngagement(const GURL& url);

  // Returns the site engagement level of the site, |url|, with the user.
  // Virtual for testing purposes.
  virtual blink::mojom::EngagementLevel GetEngagementLevel(const GURL& url);

  // Shows the share picker dialog with |targets| as the list of applications
  // presented to the user. Passes the result to |callback|. If the user picks a
  // target, the result passed to |callback| is the manifest URL of the chosen
  // target, or is null if the user cancelled the share. Virtual for testing.
  virtual void ShowPickerDialog(
      std::vector<ShareTarget> targets,
      chrome::WebShareTargetPickerCallback callback);

  // Opens a new tab and navigates to |target_url|.
  // Virtual for testing purposes.
  virtual void OpenTargetURL(const GURL& target_url);

  void OnPickerClosed(const std::string& title,
                      const std::string& text,
                      const GURL& share_url,
                      const ShareCallback& callback,
                      const base::Optional<ShareTarget>& result);

  base::WeakPtrFactory<ShareServiceImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShareServiceImpl);
};

#endif  // CHROME_BROWSER_WEBSHARE_SHARE_SERVICE_IMPL_H_
