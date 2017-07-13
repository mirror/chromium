// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ContentSettingInstanceClient_h
#define ContentSettingInstanceClient_h

#include "third_party/WebKit/public/platform/WebContentSettingsClient.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/content_setting.mojom-blink.h"

namespace blink {

class ContentSettingInstanceClient : public WebContentSettingsClient {
 public:
  ContentSettingInstanceClient(const KURL& origin_url,
                               bool is_unique_origin,
                               int route_id);
  ~ContentSettingInstanceClient() override;

  bool AllowIndexedDB(const WebString& name, const WebSecurityOrigin&) override;
  bool RequestFileSystemAccessSync() override;

 private:
  const KURL origin_url_;
  const bool is_unique_origin_;
  const int route_id_;

  // Returns an initialized ContentSettingInstanceHostPtr.
  // A connection will be established on each call to this method.
  mojom::blink::ContentSettingInstanceHostPtr GetService();
};

}  // namespace blink

#endif  // ContentSettingInstanceClient_h
