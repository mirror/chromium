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

  void QueueInterventionReport(const KURL& url,
                               const String& body,
                               const String& source_file,
                               int line_number) {
    if (reporting_service_proxy) {
      reporting_service_proxy->QueueInterventionReport(
          url, body ? body : "", source_file ? source_file : "", line_number);
    }
  }

  void QueueDeprecationReport(const KURL& url,
                              const String& body,
                              const String& source_file,
                              int line_number) {
    if (reporting_service_proxy) {
      reporting_service_proxy->QueueDeprecationReport(
          url, body ? body : "", source_file ? source_file : "", line_number);
    }
  }

  void QueueCspViolationReport(const KURL& url,
                               const String& group,
                               const String& body,
                               const String& source_file,
                               int line_number) {
    if (reporting_service_proxy) {
      reporting_service_proxy->QueueCspViolationReport(
          url, group ? group : "", body ? body : "",
          source_file ? source_file : "", line_number);
    }
  }

 private:
  mojom::blink::ReportingServiceProxyPtr reporting_service_proxy;
};

}  // namespace blink

#endif  // ReportingServiceProxyPtrHolder_h
