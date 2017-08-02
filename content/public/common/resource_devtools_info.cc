// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/resource_devtools_info.h"

#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request.h"

namespace content {

ResourceDevToolsInfo::ResourceDevToolsInfo()
    : http_status_code(0) {
}

ResourceDevToolsInfo::~ResourceDevToolsInfo() {
}

scoped_refptr<ResourceDevToolsInfo> ResourceDevToolsInfo::DeepCopy() const {
  scoped_refptr<ResourceDevToolsInfo> new_info = new ResourceDevToolsInfo();
  new_info->http_status_code = http_status_code;
  new_info->http_status_text = http_status_text;
  new_info->request_headers = request_headers;
  new_info->response_headers = response_headers;
  new_info->request_headers_text = request_headers_text;
  new_info->response_headers_text = response_headers_text;
  return new_info;
}

// static
scoped_refptr<ResourceDevToolsInfo> ResourceDevToolsInfo::FromURLRequest(
    net::URLRequest* url_request) {
  scoped_refptr<ResourceDevToolsInfo> info;

  if (!(url_request->load_flags() & net::LOAD_REPORT_WIRE_REQUEST_HEADERS))
    return info;
  const net::HttpResponseHeaders* response_headers =
      url_request->response_headers();
  if (!response_headers)
    return info;

  info = new ResourceDevToolsInfo();

  const net::HttpResponseInfo& response_info = url_request->response_info();
  // Unparsed headers only make sense if they were sent as text, i.e. HTTP 1.x.
  const bool report_headers_text =
      !response_info.DidUseQuic() && !response_info.was_fetched_via_spdy;
  net::HttpRequestHeaders request_headers;
  std::string request_line;

  url_request->GetWireRequestHeaders(&request_headers, &request_line);
  for (net::HttpRequestHeaders::Iterator it(request_headers); it.GetNext();)
    info->request_headers.push_back(std::make_pair(it.name(), it.value()));
  if (report_headers_text)
    info->request_headers_text = request_line + request_headers.ToString();

  info->http_status_code = response_headers->response_code();
  info->http_status_text = response_headers->GetStatusText();

  std::string name, value;
  for (size_t it = 0;
       response_headers->EnumerateHeaderLines(&it, &name, &value);) {
    info->response_headers.push_back(std::make_pair(name, value));
  }
  if (report_headers_text) {
    info->response_headers_text =
        net::HttpUtil::ConvertHeadersBackToHTTPResponse(
            response_headers->raw_headers());
  }
  return info;
}

}  // namespace content
