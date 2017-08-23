// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ReportingServiceProxyPtrHolder_h
#define ReportingServiceProxyPtrHolder_h

#include "platform/weborigin/KURL.h"
#include "public/platform/InterfaceProvider.h"
#include "public/platform/Platform.h"
#include "public/platform/reporting.mojom-blink.h"

namespace blink {

class ReportingServiceProxyPtrHolder {
 public:
  ReportingServiceProxyPtrHolder() {
    Platform::Current()->GetInterfaceProvider()->GetInterface(
        mojo::MakeRequest(&reporting_service_proxy));
  }
  ~ReportingServiceProxyPtrHolder() {}

  void QueueReport(const KURL& url,
                   const String& group,
                   const String& type,
                   const String& body) {
    if (reporting_service_proxy) {
      reporting_service_proxy->QueueReport(
          url, group, type,
          std::unique_ptr<base::Value>(new base::Value(body.Utf8().data())));
    }
  }

 private:
  mojom::blink::ReportingServiceProxyPtr reporting_service_proxy;
};

}  // namespace blink

#endif  // ReportingServiceProxyPtrHolder_h
