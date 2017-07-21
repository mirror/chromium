// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/SharedWorkerContentSettingsProxy.h"

#include <memory>
#include <utility>
#include "platform/weborigin/SecurityOrigin.h"
#include "public/platform/InterfaceProvider.h"
#include "public/platform/Platform.h"
#include "third_party/WebKit/Source/platform/wtf/text/WTFString.h"

namespace blink {

SharedWorkerContentSettingsProxy::SharedWorkerContentSettingsProxy(
    SecurityOrigin* security_origin,
    int route_id)
    : origin_url_(KURL(NullURL(), security_origin->ToString().IsolatedCopy())),
      is_unique_origin_(security_origin->IsUnique()),
      route_id_(route_id) {}
SharedWorkerContentSettingsProxy::~SharedWorkerContentSettingsProxy() = default;

bool SharedWorkerContentSettingsProxy::AllowIndexedDB(
    const WebString& name,
    const WebSecurityOrigin& origin) {
  if (is_unique_origin_)
    return false;
  bool result = false;
  GetService()->AllowIndexedDB(origin_url_.Copy(), name, route_id_, &result);
  return result;
}

bool SharedWorkerContentSettingsProxy::RequestFileSystemAccessSync() {
  if (is_unique_origin_)
    return false;
  bool result = false;
  GetService()->RequestFileSystemAccessSync(origin_url_.Copy(), route_id_,
                                            &result);
  return result;
}

mojom::blink::SharedWorkerContentSettingsProxyPtr&
SharedWorkerContentSettingsProxy::GetService() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      ThreadSpecific<mojom::blink::SharedWorkerContentSettingsProxyPtr>,
      content_setting_instance_host, ());
  if (!content_setting_instance_host.IsSet()) {
    Platform::Current()->GetInterfaceProvider()->GetInterface(
        mojo::MakeRequest(&*content_setting_instance_host));
  }
  return *content_setting_instance_host;
}

}  // namespace blink
