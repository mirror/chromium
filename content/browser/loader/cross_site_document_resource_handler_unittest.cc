// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/cross_site_document_resource_handler.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/loader/mock_resource_loader.h"
#include "content/browser/loader/resource_controller.h"
#include "content/browser/loader/test_resource_handler.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/resource_response.h"
#include "content/public/common/resource_type.h"
#include "content/public/common/webplugininfo.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

namespace {

// TODO(creis, nick): Expand and add tests.
class CrossSiteDocumentResourceHandlerTest : public testing::Test {
 public:
  CrossSiteDocumentResourceHandlerTest()
      : request_(context_.CreateRequest(GURL("http://www.google.com"),
                                        net::DEFAULT_PRIORITY,
                                        nullptr,
                                        TRAFFIC_ANNOTATION_FOR_TESTS)),
        old_handler_status_(
            net::URLRequestStatus::FromError(net::ERR_IO_PENDING)) {
    ResourceRequestInfo::AllocateForTesting(request_.get(),
                                            RESOURCE_TYPE_MAIN_FRAME,
                                            nullptr,       // context
                                            3,             // render_process_id
                                            2,             // render_view_id
                                            1,             // render_frame_id
                                            true,          // is_main_frame
                                            true,          // allow_download
                                            true,          // is_async
                                            PREVIEWS_OFF,  // previews_state
                                            nullptr);      // navigation_ui_data

    std::unique_ptr<TestResourceHandler> old_handler(
        new TestResourceHandler(&old_handler_status_, &old_handler_body_));
    old_handler_ = old_handler->GetWeakPtr();
    document_handler_ = std::make_unique<CrossSiteDocumentResourceHandler>(
        std::move(old_handler), request_.get(), RESOURCE_TYPE_XHR);
    mock_loader_ =
        std::make_unique<MockResourceLoader>(document_handler_.get());
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;
  net::TestURLRequestContext context_;
  std::unique_ptr<net::URLRequest> request_;

  net::URLRequestStatus old_handler_status_;
  std::string old_handler_body_;
  base::WeakPtr<TestResourceHandler> old_handler_;

  std::unique_ptr<CrossSiteDocumentResourceHandler> document_handler_;
  std::unique_ptr<MockResourceLoader> mock_loader_;
};

// TODO(creis): Add tests.
TEST_F(CrossSiteDocumentResourceHandlerTest, BlockHTML) {}

}  // namespace

}  // namespace content
