// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/serviceworkers/ContentSettingInstanceClient.h"

#include <utility>
#include "platform/weborigin/KURL.h"
#include "public/platform/InterfaceProvider.h"
#include "public/platform/Platform.h"
#include "third_party/WebKit/Source/platform/wtf/text/WTFString.h"

namespace blink {

ContentSettingInstanceClient::ContentSettingInstanceClient() {
  bool result = false;
  GetService()->AllowIndexedDB(blink::KURL(), WTF::String(), &result);
  LOG(ERROR) << __PRETTY_FUNCTION__ << result;
}

ContentSettingInstanceClient::~ContentSettingInstanceClient(){};

mojom::blink::ContentSettingInstanceHostPtr&
ContentSettingInstanceClient::GetService() {
  if (!content_setting_instance_host_) {
    Platform::Current()->GetInterfaceProvider()->GetInterface(
        mojo::MakeRequest(&content_setting_instance_host_));
  }
  return content_setting_instance_host_;
}

}  // namespace blink
