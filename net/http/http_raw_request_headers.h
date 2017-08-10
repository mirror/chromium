// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_RAW_REQUEST_HEADERS_H_
#define NET_HTTP_HTTP_RAW_REQUEST_HEADERS_H_

#include "base/callback.h"
#include "base/strings/string_piece.h"
#include "net/base/net_export.h"

#include <string>
#include <utility>
#include <vector>

namespace net {

class NET_EXPORT HttpRawRequestHeaders {
 public:
  using HeaderPair = std::pair<std::string, std::string>;
  using HeaderVector = std::vector<HeaderPair>;

  HttpRawRequestHeaders();
  HttpRawRequestHeaders(HttpRawRequestHeaders&&);
  HttpRawRequestHeaders& operator=(HttpRawRequestHeaders&&);
  ~HttpRawRequestHeaders();

  void Assign(HttpRawRequestHeaders other) { *this = std::move(other); }
  const HeaderVector& headers() const { return headers_; }
  const std::string& request_line() const { return request_line_; }
  void set_request_line(base::StringPiece line) {
    request_line_ = line.as_string();
  }
  void Add(base::StringPiece key, base::StringPiece value);
  bool FindHeader(base::StringPiece key, std::string* value) const;
  std::string AsRequestString() const;

 private:
  DISALLOW_COPY_AND_ASSIGN(HttpRawRequestHeaders);

  HeaderVector headers_;
  std::string request_line_;
};

using RequestHeadersCallback = base::Callback<void(HttpRawRequestHeaders)>;

}  // namespace net

#endif  // NET_HTTP_HTTP_RAW_REQUEST_HEADERS_H_
