// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/WebHTTPHeaderMap.h"

#include "base/memory/ptr_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "platform/network/HTTPHeaderMap.h"
#include "public/platform/WebString.h"

namespace blink {

namespace {
void ToHeaderMap(const net::HttpResponseHeaders* headers, HTTPHeaderMap& map) {
  size_t iter = 0;
  std::string name;
  std::string value;

  while (headers->EnumerateHeaderLines(&iter, &name, &value)) {
    WTF::AtomicString atomic_name =
        WTF::AtomicString::FromUTF8(name.c_str(), name.length());
    WTF::AtomicString atomic_value =
        WTF::AtomicString::FromUTF8(value.c_str(), value.length());

    if (map.Contains(atomic_name))
      map.Set(atomic_name, map.Get(atomic_name) + "," + atomic_value);
    else
      map.Add(atomic_name, atomic_value);
  }
}

void ToHeaderMap(const net::HttpRequestHeaders* headers, HTTPHeaderMap& map) {
  for (net::HttpRequestHeaders::Iterator it(*headers); it.GetNext();) {
    map.Add(
        WTF::AtomicString::FromUTF8(it.name().c_str(), it.name().length()),
        WTF::AtomicString::FromUTF8(it.value().c_str(), it.value().length()));
  }
}

}  // namespace

class WebHTTPHeaderMap::WebHTTPHeaderMapImpl {
 public:
  WebHTTPHeaderMapImpl(const HTTPHeaderMap& map) : map_(map){};

  WebHTTPHeaderMapImpl(const net::HttpRequestHeaders* headers)
      : map_(mutable_map) {
    ToHeaderMap(headers, mutable_map);
  };

  WebHTTPHeaderMapImpl(const net::HttpResponseHeaders* headers)
      : map_(mutable_map) {
    ToHeaderMap(headers, mutable_map);
  };

  HTTPHeaderMap mutable_map;

  const HTTPHeaderMap& map_;
};

WebHTTPHeaderMap::WebHTTPHeaderMap(const HTTPHeaderMap& map) {
  implementation_ = new WebHTTPHeaderMapImpl(map);
}

WebHTTPHeaderMap::WebHTTPHeaderMap(const net::HttpResponseHeaders* headers) {
  implementation_ = new WebHTTPHeaderMapImpl(headers);
}

WebHTTPHeaderMap::WebHTTPHeaderMap(const net::HttpRequestHeaders* headers) {
  implementation_ = new WebHTTPHeaderMapImpl(headers);
}

WebHTTPHeaderMap::~WebHTTPHeaderMap() {
  delete implementation_;
}

const HTTPHeaderMap& WebHTTPHeaderMap::GetHTTPHeaderMap() const {
  return implementation_->map_;
}

WebString WebHTTPHeaderMap::Get(const WebString& name) const {
  return implementation_->map_.Get(name).GetString();
}

}  // namespace blink
