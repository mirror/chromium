// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SAFE_BROWSING_PRIVATE_SAFE_BROWSING_PRIVATE_EVENT_ROUTER_H_
#define CHROME_BROWSER_EXTENSIONS_API_SAFE_BROWSING_PRIVATE_SAFE_BROWSING_PRIVATE_EVENT_ROUTER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/common/extensions/api/safe_browsing_private.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/event_router.h"

namespace content {
class BrowserContext;
}

namespace extensions {

// An event router that observes Safe Browsing events and notifies listeners.
class SafeBrowsingPrivateEventRouter : public KeyedService {
 public:
  static SafeBrowsingPrivateEventRouter* Create(
      content::BrowserContext* browser_context);
  ~SafeBrowsingPrivateEventRouter() override;

  // Notifies listeners of password reuse events.
  void OnProtectedPasswordReuseDetected(const GURL& url,
                                        const std::string& user_name,
                                        bool is_phishing);

  void OnProtectedPasswordChanged(const std::string& user_name);

 protected:
  explicit SafeBrowsingPrivateEventRouter(content::BrowserContext* context);

 private:
  content::BrowserContext* context_;
  EventRouter* event_router_;

  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingPrivateEventRouter);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SAFE_BROWSING_PRIVATE_SAFE_BROWSING_PRIVATE_EVENT_ROUTER_H_
