// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/test/embedded_test_server_util.h"

#include "base/memory/ptr_util.h"
#include "base/path_service.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const std::string public_directory = "ios/testing/data/http_server_files/";
std::unique_ptr<net::EmbeddedTestServer> test_server_;
}

namespace web {
namespace test {

void RegisterHandlerAndStart(
    const net::EmbeddedTestServer::HandleRequestCallback& handler,
    const char* sourceDirectory) {
  test_server_ = base::MakeUnique<net::EmbeddedTestServer>();
  if (sourceDirectory != NULL) {
    test_server_->ServeFilesFromSourceDirectory(
        base::FilePath(FILE_PATH_LITERAL(sourceDirectory)));
  }

  test_server_->ServeFilesFromSourceDirectory(
      base::FilePath(FILE_PATH_LITERAL(public_directory)));
  test_server_->RegisterRequestHandler(handler);
  CHECK(test_server_->Start());
}

void ShutDownServer() {
  DCHECK(test_server_);
  test_server_.release();
}

GURL GetURL(const std::string& relative_url) {
  DCHECK(test_server_);
  return test_server_->GetURL(relative_url);
}
}
}
