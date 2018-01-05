// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/webrtc_event_log/webrtc_remote_event_log_manager_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace content {

//BrowserContextKeyedServiceShutdownNotifierFactory

KeyedService* WebRtcRemoteEventLogManagerFactory::BuildServiceInstanceFor(content::BrowserContext* context) const {
  return nullptr;
}

WebRtcRemoteEventLogManager* WebRtcRemoteEventLogManagerFactory::GetForBrowserContext(
    BrowserContext* context) {
  return nullptr;
//  return static_cast<WebRtcRemoteEventLogManager*>(
//      GetInstance()->GetServiceForBrowserContext(context, true));
}

WebRtcRemoteEventLogManagerFactory*
WebRtcRemoteEventLogManagerFactory::GetInstance() {
  return base::Singleton<WebRtcRemoteEventLogManagerFactory>::get();
}

WebRtcRemoteEventLogManagerFactory::WebRtcRemoteEventLogManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "WebRtcRemoteEventLogManager",
          BrowserContextDependencyManager::GetInstance()) {
  // TODO: !!!
}

WebRtcRemoteEventLogManagerFactory::~WebRtcRemoteEventLogManagerFactory() {}

bool WebRtcRemoteEventLogManagerFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace content
