// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PAYMENTS_PAYMENT_APP_INSTALLER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"

class GURL;

namespace content {

// Installs the web payment app with a default payment instrument and returns
// the registration Id throught callback on success.
class PaymentAppInstaller {
 public:
  using InstallPaymentAppCallback =
      base::OnceCallback<void(BrowserContext* browser_context,
                              long registration_id)>;

  // Installs the payment app.
  // |app_name| is the name of the payment app.
  // |sw_url| is the url to get the service worker js script.
  // |scope| is the registration scope.
  // |use_cache| indicates whether use cache.
  // |enabled_methods| are the enabled methods of the app.
  // |callback| to send back registration result.
  static void Install(WebContents* web_contents,
                      std::string& app_name,
                      GURL& sw_url,
                      GURL& scope,
                      bool use_cache,
                      std::vector<std::string>& enabled_methods,
                      InstallPaymentAppCallback callback);

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentAppInstaller);
};

}  // namespace content.

#endif  // CONTENT_BROWSER_PAYMENTS_PAYMENT_APP_INSTALLER_H_