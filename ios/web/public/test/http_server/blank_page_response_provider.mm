// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/test/http_server/blank_page_response_provider.h"

#include <map>

#include "base/memory/ptr_util.h"
#import "ios/web/public/test/http_server/html_response_provider.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "url/gurl.h"

namespace web {
namespace test {

// The URL and HTML used for the blank page ResponseProvider.
const char kBlankTestPageURL[] = "http://blank-page";
const char kBlankTestPageHTML[] = "<!DOCTYPE html><html><body></body></html>";

std::unique_ptr<ResponseProvider> CreateBlankPageResponseProvider() {
  std::map<GURL, std::string> responses;
  responses[web::test::HttpServer::MakeUrl(kBlankTestPageURL)] =
      kBlankTestPageHTML;
  return base::MakeUnique<HtmlResponseProvider>(responses);
}

}  // namespace test
}  // namespace web
