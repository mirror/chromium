// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/webrtc_event_log/webrtc_remote_event_log_manager_factory.h"

//#include "chrome/browser/profiles/incognito_helpers.h"
//#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace content {

WebRtcRemoteEventLogManager* WebRtcRemoteEventLogManagerFactory::GetForProfile(
    Profile* profile) {
//  return static_cast<WebRtcRemoteEventLogManager*>(
//      GetInstance()->GetServiceForBrowserContext(profile, true));
  return nullptr;
}

// static
WebRtcRemoteEventLogManagerFactory*
WebRtcRemoteEventLogManagerFactory::GetInstance() {
  return base::Singleton<WebRtcRemoteEventLogManagerFactory>::get();
}

WebRtcRemoteEventLogManagerFactory::WebRtcRemoteEventLogManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "WebRtcRemoteEventLogManager",
          BrowserContextDependencyManager::GetInstance()) {}

WebRtcRemoteEventLogManagerFactory::~WebRtcRemoteEventLogManagerFactory() {}

KeyedService* WebRtcRemoteEventLogManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
//  return new WebRtcRemoteEventLogManager(Profile::FromBrowserContext(context));
  return nullptr;
}

// TODO: !!! ?
//content::BrowserContext*
//WebRtcRemoteEventLogManagerFactory::GetBrowserContextToUse(
//    content::BrowserContext* context) const {
//  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
//}

}  // namespace content
