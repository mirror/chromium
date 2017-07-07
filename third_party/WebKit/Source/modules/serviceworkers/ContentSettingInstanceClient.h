// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ContentSettingInstanceClient_h
#define ContentSettingInstanceClient_h

#include "modules/serviceworkers/ServiceWorkerRegistration.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/content_setting.mojom-blink.h"
#include "third_party/WebKit/public/web/WebSharedWorkerClient.h"

namespace blink {

// The bridge is responsible for establishing and maintaining the Mojo
// connection to the BackgroundFetchService. It's keyed on an active Service
// Worker Registration.
class ContentSettingInstanceClient : public WebSharedWorkerClient {
 public:
  ContentSettingInstanceClient();
  ~ContentSettingInstanceClient();

 private:
  // Returns an initialized BackgroundFetchServicePtr. A connection will be
  // established after the first call to this method.
  mojom::blink::ContentSettingInstanceHostPtr& GetService();

  mojom::blink::ContentSettingInstanceHostPtr content_setting_instance_host_;
};

}  // namespace blink

#endif  // ContentSettingInstanceClient_h
