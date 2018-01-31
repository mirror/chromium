// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/safe_browsing_private/safe_browsing_private_event_router.h"

#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "url/gurl.h"

namespace extensions {

SafeBrowsingPrivateEventRouter::SafeBrowsingPrivateEventRouter(
    content::BrowserContext* context)
    : context_(context), event_router_(nullptr) {
  event_router_ = EventRouter::Get(context_);
}

SafeBrowsingPrivateEventRouter::~SafeBrowsingPrivateEventRouter() {}

void SafeBrowsingPrivateEventRouter::OnProtectedPasswordReuseDetected(
    const GURL& url,
    const std::string& user_name,
    bool is_phishing) {
  api::safe_browsing_private::ProtectedPasswordReuse params;
  params.url = url.spec();
  params.user_name = user_name;
  params.is_phishing = is_phishing;

  auto event_value = std::make_unique<base::ListValue>();
  event_value->Append(params.ToValue());

  auto extension_event = std::make_unique<Event>(
      events::SAFE_BROWSING_PRIVATE_ON_PROTECTED_PASSWORD_REUSE_DETECTED,
      api::safe_browsing_private::OnProtectedPasswordReuseDetected::kEventName,
      std::move(event_value));
  event_router_->BroadcastEvent(std::move(extension_event));
}

void SafeBrowsingPrivateEventRouter::OnProtectedPasswordChanged(
    const std::string& user_name) {
  auto event_value = std::make_unique<base::ListValue>();
  event_value->Append(std::make_unique<base::Value>(user_name));
  auto extension_event = std::make_unique<Event>(
      events::SAFE_BROWSING_PRIVATE_ON_PROTECTED_PASSWORD_CHANGED,
      api::safe_browsing_private::OnProtectedPasswordChanged::kEventName,
      std::move(event_value));
  event_router_->BroadcastEvent(std::move(extension_event));
}

SafeBrowsingPrivateEventRouter* SafeBrowsingPrivateEventRouter::Create(
    content::BrowserContext* context) {
  return new SafeBrowsingPrivateEventRouter(context);
}

}  // namespace extensions
