// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ModuleScriptData_h
#define ModuleScriptData_h

#include "platform/loader/fetch/AccessControlStatus.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebURLRequest.h"

namespace blink {

struct ModuleScriptData {
 public:
  ModuleScriptData(const KURL& response_url,
                   const String& source_text,
                   WebURLRequest::FetchCredentialsMode fetch_credentials_mode,
                   AccessControlStatus access_control_status)
      : response_url(response_url),
        source_text(source_text),
        fetch_credentials_mode(fetch_credentials_mode),
        access_control_status(access_control_status) {}
  ~ModuleScriptData() {}

  KURL response_url;
  String source_text;
  WebURLRequest::FetchCredentialsMode fetch_credentials_mode;
  AccessControlStatus access_control_status;
};

}  // namespace blink

#endif  // ModuleScriptData_h
