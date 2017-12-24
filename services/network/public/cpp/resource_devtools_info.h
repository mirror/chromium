// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_RESOURCE_DEVTOOLS_INFO_H_
#define SERVICES_NETWORK_PUBLIC_CPP_RESOURCE_DEVTOOLS_INFO_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/strings/string_split.h"

namespace network {

// Note: when modifying this structure, also update DeepCopy in
// resource_devtools_info.cc.
struct ResourceDevToolsInfo : base::RefCounted<ResourceDevToolsInfo> {
  typedef base::StringPairs HeadersVector;

  ResourceDevToolsInfo();

  scoped_refptr<ResourceDevToolsInfo> DeepCopy() const;

  int32_t http_status_code;
  std::string http_status_text;
  HeadersVector request_headers;
  HeadersVector response_headers;
  std::string request_headers_text;
  std::string response_headers_text;

 private:
  friend class base::RefCounted<ResourceDevToolsInfo>;
  ~ResourceDevToolsInfo();
};

}  // namespace network

#endif  // SERVICES_NETWORK_PUBLIC_CPP_RESOURCE_DEVTOOLS_INFO_H_
