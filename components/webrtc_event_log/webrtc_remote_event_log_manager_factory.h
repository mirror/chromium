// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEBRTC_EVENT_LOG_WEBRTC_REMOTE_EVENT_LOG_MANAGER_FACTORY_H_
#define COMPONENTS_WEBRTC_EVENT_LOG_WEBRTC_REMOTE_EVENT_LOG_MANAGER_FACTORY_H_

//#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
//#include "content/browser/webrtc/webrtc_remote_event_log_manager.h"

class Profile;

namespace content {

class WebRtcRemoteEventLogManager;

class WebRtcRemoteEventLogManagerFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static WebRtcRemoteEventLogManager* GetForProfile(Profile* profile);
  static WebRtcRemoteEventLogManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<WebRtcRemoteEventLogManagerFactory>;

  WebRtcRemoteEventLogManagerFactory();
  ~WebRtcRemoteEventLogManagerFactory() override;

  // BrowserContextKeyedBaseFactory methods:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  // TODO: !!! ?
//  content::BrowserContext* GetBrowserContextToUse(
//      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(WebRtcRemoteEventLogManagerFactory);
};

}  // namespace content

#endif  // COMPONENTS_WEBRTC_EVENT_LOG_WEBRTC_REMOTE_EVENT_LOG_MANAGER_FACTORY_H_
