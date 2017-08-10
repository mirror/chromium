// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_raw_request_headers.h"
#include "base/debug/stack_trace.h"
#include "base/strings/stringprintf.h"

namespace net {

HttpRawRequestHeaders::HttpRawRequestHeaders() = default;
HttpRawRequestHeaders::HttpRawRequestHeaders(HttpRawRequestHeaders&&) = default;
HttpRawRequestHeaders& HttpRawRequestHeaders::operator=(
    HttpRawRequestHeaders&&) = default;
HttpRawRequestHeaders::~HttpRawRequestHeaders() = default;

void HttpRawRequestHeaders::Add(base::StringPiece key,
                                base::StringPiece value) {
  headers_.emplace_back(key.as_string(), value.as_string());
}

bool HttpRawRequestHeaders::FindHeader(base::StringPiece key,
                                       std::string* value) const {
  for (const auto& entry : headers_) {
    if (entry.first == key) {
      *value = entry.second;
      return true;
    }
  }
  return false;
}

std::string HttpRawRequestHeaders::AsRequestString() const {
  std::string result = request_line_;
  for (const auto& pair : headers_) {
    if (!pair.second.empty()) {
      base::StringAppendF(&result, "%s: %s\r\n", pair.first.c_str(),
                          pair.second.c_str());
    } else {
      base::StringAppendF(&result, "%s:\r\n", pair.first.c_str());
    }
  }
  result.append("\r\n");
  return result;
}

}  // namespace net
