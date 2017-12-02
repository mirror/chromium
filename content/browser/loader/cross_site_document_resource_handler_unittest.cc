// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/cross_site_document_resource_handler.h"

#include <stdint.h>

#include <memory>
#include <string>

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

enum class OriginHeader { OMIT, INCLUDE };

enum class AccessControlAllowOriginHeader {
  OMIT,
  ALLOW_ANY,
  ALLOW_NULL,
  ALLOW_INITIATOR_ORIGIN,
  ALLOW_EXAMPLE_DOT_COM,
};

enum class Verdict {
  ALLOW_WITHOUT_SNIFFING,
  BLOCK_WITHOUT_SNIFFING,
  BLOCK_AFTER_SNIFFING,
  ALLOW_AFTER_SNIFFING
};

struct TestScenario {
  const char* description;
  int source_line;
  // Attributes of the HTTP Request.
  const char* target_url;
  ResourceType resource_type;
  const char* initiator_origin;
  OriginHeader cors_request;

  // Attributes of the HTTP response.
  const char* response_mime_type;
  CrossSiteDocumentMimeType canonical_mime_type_after_headers;
  bool include_no_sniff_header;
  AccessControlAllowOriginHeader cors_response;
  const char* first_chunk;

  // Expected results.
  CrossSiteDocumentMimeType canonical_mime_type_after_first_chunk;
  Verdict verdict;

  // TODO(nick): Add a stream operator or a PrintTo method here, so that
  // the GetParam() printf shows something other than a stream of bytes.
};

}  // namespace

class CrossSiteDocumentResourceHandlerTest
    : public testing::Test,
      public testing::WithParamInterface<TestScenario> {
 public:
  CrossSiteDocumentResourceHandlerTest()
      : stream_sink_status_(
            net::URLRequestStatus::FromError(net::ERR_IO_PENDING)) {}

  void Initialize(const std::string& target_url,
                  ResourceType resource_type,
                  const std::string& initiator_origin) {
    stream_sink_status_ = net::URLRequestStatus::FromError(net::ERR_IO_PENDING);

    // Initialize |request_| from the parameters.
    request_ = context_.CreateRequest(GURL(target_url), net::DEFAULT_PRIORITY,
                                      nullptr, TRAFFIC_ANNOTATION_FOR_TESTS);
    ResourceRequestInfo::AllocateForTesting(request_.get(), resource_type,
                                            nullptr,       // context
                                            3,             // render_process_id
                                            2,             // render_view_id
                                            1,             // render_frame_id
                                            true,          // is_main_frame
                                            true,          // allow_download
                                            true,          // is_async
                                            PREVIEWS_OFF,  // previews_state
                                            nullptr);      // navigation_ui_data
    request_->set_initiator(url::Origin::Create(GURL(initiator_origin)));

    // Create a sink handler to capture results.
    std::unique_ptr<TestResourceHandler> stream_sink(
        new TestResourceHandler(&stream_sink_status_, &stream_sink_body_));
    stream_sink_ = stream_sink->GetWeakPtr();

    // Create the CrossSiteDocumentResourceHandler.
    document_blocker_ = std::make_unique<CrossSiteDocumentResourceHandler>(
        std::move(stream_sink), request_.get(), RESOURCE_TYPE_XHR);

    // Create a mock loader to drive the CrossSiteDocumentResourceHandler.
    mock_loader_ =
        std::make_unique<MockResourceLoader>(document_blocker_.get());
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;
  net::TestURLRequestContext context_;
  std::unique_ptr<net::URLRequest> request_;

  // |stream_sink_| is the handler that's immediately after
  // |document_blocker_| in the ResourceHandler chain; it records the values
  // passed to it into |stream_sink_status_| and
  // |stream_sink_body_|, which our tests assert against.
  //
  // |stream_sink_| is owned by |document_blocker_|, but we retain a
  // reference to it.
  base::WeakPtr<TestResourceHandler> stream_sink_;
  net::URLRequestStatus stream_sink_status_;
  std::string stream_sink_body_;

  // |document_blocker_| is the CrossSiteDocuemntResourceHandler instance under
  // test.
  std::unique_ptr<CrossSiteDocumentResourceHandler> document_blocker_;

  // |mock_loader_| is the mock loader used to drive |document_blocker_|.
  std::unique_ptr<MockResourceLoader> mock_loader_;
};

TEST_P(CrossSiteDocumentResourceHandlerTest, ResponseBlocking) {
  const TestScenario scenario = GetParam();

  SCOPED_TRACE(testing::Message()
               << "A failure occured in a parameterized test case:\n"
               << __FILE__ << ":" << scenario.source_line
               << ": TestScenario declared here"
               << "\n  scenario.target_url          = " << scenario.target_url
               << "\n  scenario.initiator_origin    = "
               << scenario.initiator_origin
               << "\n  scenario.response_mime_type  = "
               << scenario.response_mime_type
               << "\n  scenario.first_chunk         = " << scenario.first_chunk
               << "\n  scenario.cors_request        = "
               << (scenario.cors_request == OriginHeader::OMIT
                       ? "OriginHeader::OMIT"
                       : "OriginHeader::INCLUDE"));

  // TODO(nick): Set origin header, mime type, ACAO on response.

  Initialize(scenario.target_url, scenario.resource_type,
             scenario.initiator_origin);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));

  // TODO(nick): Should we test what happens if we get OnWillRead prior to
  // OnResponseStarted?
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseStarted(
                base::MakeRefCounted<ResourceResponse>()));

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());

  if (scenario.verdict == Verdict::ALLOW_WITHOUT_SNIFFING) {
    EXPECT_EQ(mock_loader_->io_buffer(), stream_sink_->buffer());
    // TODO(nick): Note that we set canonical_mime_type in the middle of the
    // logic here...
    EXPECT_EQ(document_blocker_->canonical_mime_type_,
              CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS);
    EXPECT_FALSE(document_blocker_->should_block_response_);
  } else {
    EXPECT_EQ(mock_loader_->io_buffer(),
              document_blocker_->local_buffer_.get());
    EXPECT_TRUE(document_blocker_->should_block_response_);
  }
  EXPECT_EQ(document_blocker_->canonical_mime_type_,
            scenario.canonical_mime_type_after_headers);

  // Deliver the first chunk of the response body; this allows sniffing to
  // occur.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(scenario.first_chunk));
  EXPECT_EQ(mock_loader_->io_buffer(), nullptr);

  if (scenario.verdict == Verdict::ALLOW_WITHOUT_SNIFFING ||
      scenario.verdict == Verdict::ALLOW_AFTER_SNIFFING) {
    EXPECT_EQ(stream_sink_body_, scenario.first_chunk);
  } else {
    EXPECT_EQ(stream_sink_body_, "")
        << "Response should not have been delivered to the renderer.";
  }
}

namespace {

const TestScenario kScenarios[] =
    {
        {
            "Cross-site XHR to HTML document without CORS should be blocked",
            __LINE__,
            "http://www.b.com/resource.html",  // target_url
            RESOURCE_TYPE_XHR,                 // resource_type
            "http://www.a.com/",               // initiator_origin
            OriginHeader::OMIT,                // cors_request
            "text/html",                       // response_mime_type
            CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,  // canonical_mime_type_after_headers
            true,                                // include_no_sniff_header
            AccessControlAllowOriginHeader::OMIT,     // cors_response
            "<html><head>this should sniff as HTML",  // first_chunk
            CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS,  // canonical_mime_type_after_first_chunk
            Verdict::ALLOW_WITHOUT_SNIFFING,  // verdict
        },
        {
            "Cross-site XHR to HTML w/ DTD without CORS should be blocked",
            __LINE__,
            "http://www.b.com/resource.html",  // target_url
            RESOURCE_TYPE_XHR,                 // resource_type
            "http://www.a.com/",               // initiator_origin
            OriginHeader::OMIT,                // cors_request
            "text/html",                       // response_mime_type
            CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,  // canonical_mime_type_after_headers
            true,                                // include_no_sniff_header
            AccessControlAllowOriginHeader::OMIT,  // cors_response
            "<!doctype html><html itemscope=\"\" "
            "itemtype=\"http://schema.org/SearchResultsPage\" "
            "lang=\"en\"><head>",                  // first_chunk
            CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS,  // canonical_mime_type_after_first_chunk
            Verdict::ALLOW_WITHOUT_SNIFFING,  // verdict
        },
        {
            "Cross-site <script> inclusion of to HTML w/ DTD without CORS "
            "should "
            "be blocked",
            __LINE__,
            "http://www.b.com/resource.html",  // target_url
            RESOURCE_TYPE_SCRIPT,              // resource_type
            "http://www.a.com/",               // initiator_origin
            OriginHeader::OMIT,                // cors_request
            "text/html",                       // response_mime_type
            CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,  // canonical_mime_type_after_headers
            true,                                // include_no_sniff_header
            AccessControlAllowOriginHeader::OMIT,  // cors_response
            "<!doctype html><html itemscope=\"\" "
            "itemtype=\"http://schema.org/SearchResultsPage\" "
            "lang=\"en\"><head>",                  // first_chunk
            CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS,  // canonical_mime_type_after_first_chunk
            Verdict::ALLOW_WITHOUT_SNIFFING,  // verdict
        },

};

}  // namespace

INSTANTIATE_TEST_CASE_P(,
                        CrossSiteDocumentResourceHandlerTest,
                        ::testing::ValuesIn(kScenarios));

}  // namespace content
