// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_AUTH_SCHEME_SET_H_
#define NET_HTTP_HTTP_AUTH_SCHEME_SET_H_

#include <set>
#include <string>

#include "net/base/net_export.h"

namespace net {

// A set of HTTP auth schemes.
class NET_EXPORT HttpAuthSchemeSet {
 public:
  HttpAuthSchemeSet();
  ~HttpAuthSchemeSet();

  // |scheme| needs to be normalized.
  void Add(const std::string& scheme);

  // |scheme| needs to be normalized.
  bool Contains(const std::string& scheme) const;

 private:
  std::set<std::string> schemes_;
};

}  // namespace net

#endif  // NET_HTTP_HTTP_AUTH_SCHEME_SET_H_
