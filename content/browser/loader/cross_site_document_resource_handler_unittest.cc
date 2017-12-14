// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/cross_site_document_resource_handler.h"

#include <stdint.h>

#include <initializer_list>
#include <memory>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/test/histogram_tester.h"
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
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

namespace {

enum class OriginHeader { kOmit, kInclude };

enum class AccessControlAllowOriginHeader {
  kOmit,
  kAllowAny,
  kAllowNull,
  kAllowInitiatorOrigin,
  kAllowExampleDotCom
};

enum class Verdict {
  kAllow,
  kBlock,
};

// This struct is used to describe each test case in this file.  It's passed as
// a test parameter to each TEST_P test.
struct TestScenario {
  // Attributes to make test failure messages useful.
  const char* description;
  int source_line;

  // Attributes of the HTTP Request.
  const char* target_url;
  ResourceType resource_type;
  const char* initiator_origin;
  OriginHeader cors_request;

  // Attributes of the HTTP response.
  const char* response_mime_type;
  CrossSiteDocumentMimeType canonical_mime_type;
  bool include_no_sniff_header;
  AccessControlAllowOriginHeader cors_response;
  // |packets| specifies the response data which may arrive over the course of
  // several writes.
  std::initializer_list<const char*> packets;

  std::string data() const {
    std::string data;
    for (const char* packet : packets) {
      data += packet;
    }
    return data;
  }

  // Expected result.
  Verdict verdict;
  // The packet number during which the verdict is decided. -1 means
  // that the verdict can be decided before the first packet's data
  // is available. |packets.size()| means that the verdict is decided
  // during the end-of-stream call.
  int verdict_packet;
};

// Stream operator to let GetParam() print a useful result if any tests fail.
::std::ostream& operator<<(::std::ostream& os, const TestScenario& scenario) {
  std::string cors_response;
  switch (scenario.cors_response) {
    case AccessControlAllowOriginHeader::kOmit:
      cors_response = "AccessControlAllowOriginHeader::kOmit";
      break;
    case AccessControlAllowOriginHeader::kAllowAny:
      cors_response = "AccessControlAllowOriginHeader::kAllowAny";
      break;
    case AccessControlAllowOriginHeader::kAllowNull:
      cors_response = "AccessControlAllowOriginHeader::kAllowNull";
      break;
    case AccessControlAllowOriginHeader::kAllowInitiatorOrigin:
      cors_response = "AccessControlAllowOriginHeader::kAllowInitiatorOrigin";
      break;
    case AccessControlAllowOriginHeader::kAllowExampleDotCom:
      cors_response = "AccessControlAllowOriginHeader::kAllowExampleDotCom";
      break;
  }

  std::string verdict;
  switch (scenario.verdict) {
    case Verdict::kAllow:
      verdict = "Verdict::kAllowWithoutSniffing";
      break;
    case Verdict::kBlock:
      verdict = "Verdict::kBlockWithoutSniffing";
      break;
  }

  return os << "\n  description         = " << scenario.description
            << "\n  target_url          = " << scenario.target_url
            << "\n  resource_type       = " << scenario.resource_type
            << "\n  initiator_origin    = " << scenario.initiator_origin
            << "\n  cors_request        = "
            << (scenario.cors_request == OriginHeader::kOmit
                    ? "OriginHeader::kOmit"
                    : "OriginHeader::kInclude")
            << "\n  response_mime_type  = " << scenario.response_mime_type
            << "\n  canonical_mime_type = " << scenario.canonical_mime_type
            << "\n  include_no_sniff    = "
            << (scenario.include_no_sniff_header ? "true" : "false")
            << "\n  cors_response       = " << cors_response
            << "\n  data                = " << scenario.data()
            << "\n  verdict             = " << verdict
            << "\n  verdict_packet      = " << scenario.verdict_packet;
}

// An HTML response with an HTML comment that's longer than the sniffing
// threshhold. We don't sniff past net::kMaxBytesToSniff, so these are not
// protected
const char kHTMLWithTooLongComment[] =
    "<!--.............................................................72 chars"
    "................................................................144 chars"
    "................................................................216 chars"
    "................................................................288 chars"
    "................................................................360 chars"
    "................................................................432 chars"
    "................................................................504 chars"
    "................................................................576 chars"
    "................................................................648 chars"
    "................................................................720 chars"
    "................................................................792 chars"
    "................................................................864 chars"
    "................................................................936 chars"
    "...............................................................1008 chars"
    "...............................................................1080 chars"
    "--><html><head>";

// A set of test cases that verify CrossSiteDocumentResourceHandler correctly
// classifies network responses as allowed or blocked.  These TestScenarios are
// passed to the TEST_P tests below as test parameters.
const TestScenario kScenarios[] = {
    // Allowed responses:
    {
        "Allowed: Same-site XHR to HTML",
        __LINE__,
        "http://www.a.com/resource.html",           // target_url
        RESOURCE_TYPE_XHR,                          // resource_type
        "http://www.a.com/",                        // initiator_origin
        OriginHeader::kOmit,                        // cors_request
        "text/html",                                // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,         // canonical_mime_type
        false,                                      // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,      // cors_response
        {"<html><head>this should sniff as HTML"},  // packets
        Verdict::kAllow,                            // verdict
        -1,                                         // verdict_packet
    },
    {
        "Allowed: Cross-site script",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_SCRIPT,                   // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "application/javascript",               // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS,   // canonical_mime_type
        false,                                  // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {"var x=3;"},                           // packets
        Verdict::kAllow,                        // verdict
        -1,                                     // verdict_packet
    },
    {
        "Allowed: Cross-site XHR to HTML with CORS for origin",
        __LINE__,
        "http://www.b.com/resource.html",    // target_url
        RESOURCE_TYPE_XHR,                   // resource_type
        "http://www.a.com/",                 // initiator_origin
        OriginHeader::kInclude,              // cors_request
        "text/html",                         // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,  // canonical_mime_type
        false,                               // include_no_sniff_header
        AccessControlAllowOriginHeader::kAllowInitiatorOrigin,  // cors_response
        {"<html><head>this should sniff as HTML"},              // packets
        Verdict::kAllow,                                        // verdict
        -1,  // verdict_packet
    },
    {
        "Allowed: Cross-site XHR to XML with CORS for any",
        __LINE__,
        "http://www.b.com/resource.html",           // target_url
        RESOURCE_TYPE_XHR,                          // resource_type
        "http://www.a.com/",                        // initiator_origin
        OriginHeader::kInclude,                     // cors_request
        "application/rss+xml",                      // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_XML,          // canonical_mime_type
        false,                                      // include_no_sniff_header
        AccessControlAllowOriginHeader::kAllowAny,  // cors_response
        {"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"},  // packets
        Verdict::kAllow,                                  // verdict
        -1,                                               // verdict_packet
    },
    {
        "Allowed: Cross-site XHR to JSON with CORS for null",
        __LINE__,
        "http://www.b.com/resource.html",            // target_url
        RESOURCE_TYPE_XHR,                           // resource_type
        "http://www.a.com/",                         // initiator_origin
        OriginHeader::kInclude,                      // cors_request
        "text/json",                                 // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_JSON,          // canonical_mime_type
        false,                                       // include_no_sniff_header
        AccessControlAllowOriginHeader::kAllowNull,  // cors_response
        {"{\"x\" : 3}"},                             // packets
        Verdict::kAllow,                             // verdict
        -1,                                          // verdict_packet
    },
    {
        "Allowed: Cross-site XHR to HTML over FTP",
        __LINE__,
        "ftp://www.b.com/resource.html",            // target_url
        RESOURCE_TYPE_XHR,                          // resource_type
        "http://www.a.com/",                        // initiator_origin
        OriginHeader::kOmit,                        // cors_request
        "text/html",                                // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,         // canonical_mime_type
        false,                                      // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,      // cors_response
        {"<html><head>this should sniff as HTML"},  // packets
        Verdict::kAllow,                            // verdict
        -1,                                         // verdict_packet
    },
    {
        "Allowed: Cross-site XHR to HTML from file://",
        __LINE__,
        "file:///foo/resource.html",                // target_url
        RESOURCE_TYPE_XHR,                          // resource_type
        "http://www.a.com/",                        // initiator_origin
        OriginHeader::kOmit,                        // cors_request
        "text/html",                                // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,         // canonical_mime_type
        false,                                      // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,      // cors_response
        {"<html><head>this should sniff as HTML"},  // packets
        Verdict::kAllow,                            // verdict
        -1,                                         // verdict_packet
    },
    {
        "Allowed: Cross-site fetch HTML from Flash without CORS", __LINE__,
        "http://www.b.com/plugin.html",           // target_url
        RESOURCE_TYPE_PLUGIN_RESOURCE,            // resource_type
        "http://www.a.com/",                      // initiator_origin
        OriginHeader::kOmit,                      // cors_request
        "text/html",                              // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,       // canonical_mime_type
        false,                                    // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,    // cors_response
        "<html><head>this should sniff as HTML",  // first_chunk
        Verdict::kAllowWithoutSniffing,           // verdict
    },
    {
        "Allowed: Cross-site fetch HTML from NaCl with CORS response", __LINE__,
        "http://www.b.com/plugin.html",      // target_url
        RESOURCE_TYPE_PLUGIN_RESOURCE,       // resource_type
        "http://www.a.com/",                 // initiator_origin
        OriginHeader::kInclude,              // cors_request
        "text/html",                         // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,  // canonical_mime_type
        false,                               // include_no_sniff_header
        AccessControlAllowOriginHeader::kAllowInitiatorOrigin,  // cors_response
        "<html><head>this should sniff as HTML",                // first_chunk
        Verdict::kAllowWithoutSniffing,                         // verdict
    },

    // Allowed responses due to sniffing:
    {
        "Allowed: Cross-site script to JSONP labeled as HTML",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_SCRIPT,                   // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "text/html",                            // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,     // canonical_mime_type
        false,                                  // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {"foo({\"x\" : 3})"},                   // packets
        Verdict::kAllow,                        // verdict
        0,                                      // verdict_packet
    },
    {
        "Allowed: Cross-site script to JavaScript labeled as text",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_SCRIPT,                   // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "text/plain",                           // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_PLAIN,    // canonical_mime_type
        false,                                  // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {"var x = 3;"},                         // packets
        Verdict::kAllow,                        // verdict
        0,                                      // verdict_packet
    },
    {
        "Allowed: Cross-site XHR to nonsense labeled as XML",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_XHR,                      // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "application/xml",                      // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_XML,      // canonical_mime_type
        false,                                  // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {"Won't sniff as XML"},                 // packets
        Verdict::kAllow,                        // verdict
        0,                                      // verdict_packet
    },
    {
        "Allowed: Cross-site XHR to nonsense labeled as JSON",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_XHR,                      // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "text/x-json",                          // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_JSON,     // canonical_mime_type
        false,                                  // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {"Won't sniff as JSON"},                // packets
        Verdict::kAllow,                        // verdict
        0,                                      // verdict_packet
    },
    {
        "Allowed: Cross-site XHR to partial match for <HTML> tag",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_XHR,                      // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "text/html",                            // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,     // canonical_mime_type
        false,                                  // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {"<htm"},                               // packets
        Verdict::kAllow,                        // verdict
        1,                                      // verdict_packet
    },
    {
        "Allowed: HTML tag appears only after net::kMaxBytesToSniff",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_XHR,                      // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "text/html",                            // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,     // canonical_mime_type
        false,                                  // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {kHTMLWithTooLongComment},              // packets
        Verdict::kAllow,                        // verdict
        0,                                      // verdict_packet
    },
    {
        "Allowed: Empty response with html mime type",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_XHR,                      // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "text/html",                            // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,     // canonical_mime_type
        false,                                  // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {},                                     // packets
        Verdict::kAllow,                        // verdict
        0,                                      // verdict_packet
    },
    {
        "Allowed: Empty response with PNG mime type",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_XHR,                      // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "image/png",                            // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS,   // canonical_mime_type
        false,                                  // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {},                                     // packets
        Verdict::kAllow,                        // verdict
        -1,                                     // verdict_packet
    },
    {
        "Allowed: Empty response with PNG mime type and nosniff header",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_XHR,                      // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "image/png",                            // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS,   // canonical_mime_type
        true,                                   // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {},                                     // packets
        Verdict::kAllow,                        // verdict
        -1,                                     // verdict_packet
    },

    // Blocked responses:
    {
        "Blocked: Cross-site XHR to HTML without CORS",
        __LINE__,
        "http://www.b.com/resource.html",           // target_url
        RESOURCE_TYPE_XHR,                          // resource_type
        "http://www.a.com/",                        // initiator_origin
        OriginHeader::kOmit,                        // cors_request
        "text/html",                                // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,         // canonical_mime_type
        false,                                      // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,      // cors_response
        {"<html><head>this should sniff as HTML"},  // packets
        Verdict::kBlock,                            // verdict
        0,                                          // verdict_packet
    },
    {
        "Blocked: Cross-site XHR to XML without CORS",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_XHR,                      // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "application/xml",                      // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_XML,      // canonical_mime_type
        false,                                  // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"},  // packets
        Verdict::kBlock,                                  // verdict
        0,                                                // verdict_packet

    },
    {
        "Blocked: Cross-site XHR to JSON without CORS",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_XHR,                      // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "application/json",                     // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_JSON,     // canonical_mime_type
        false,                                  // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {"{\"x\" : 3}"},                        // packets
        Verdict::kBlock,                        // verdict
        0,                                      // verdict_packet
    },
    {
        "Blocked: Cross-site XHR to HTML labeled as text without CORS",
        __LINE__,
        "http://www.b.com/resource.html",           // target_url
        RESOURCE_TYPE_XHR,                          // resource_type
        "http://www.a.com/",                        // initiator_origin
        OriginHeader::kOmit,                        // cors_request
        "text/plain",                               // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_PLAIN,        // canonical_mime_type
        false,                                      // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,      // cors_response
        {"<html><head>this should sniff as HTML"},  // packets
        Verdict::kBlock,                            // verdict
        0,                                          // verdict_packet

    },
    {
        "Blocked: Cross-site XHR to nosniff HTML without CORS",
        __LINE__,
        "http://www.b.com/resource.html",           // target_url
        RESOURCE_TYPE_XHR,                          // resource_type
        "http://www.a.com/",                        // initiator_origin
        OriginHeader::kOmit,                        // cors_request
        "text/html",                                // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,         // canonical_mime_type
        true,                                       // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,      // cors_response
        {"<html><head>this should sniff as HTML"},  // packets
        Verdict::kBlock,                            // verdict
        -1,                                         // verdict_packet
    },
    {
        "Blocked: Cross-site XHR to nosniff response without CORS",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_XHR,                      // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "text/html",                            // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,     // canonical_mime_type
        true,                                   // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {"Wouldn't sniff as HTML"},             // packets
        Verdict::kBlock,                        // verdict
        -1,                                     // verdict_packet
    },
    {
        "Blocked: Cross-site <script> inclusion of HTML w/ DTD without CORS",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_SCRIPT,                   // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kOmit,                    // cors_request
        "text/html",                            // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,     // canonical_mime_type
        false,                                  // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {"<!doc", "type html><html itemscope=\"\" ",
         "itemtype=\"http://schema.org/SearchResultsPage\" ",
         "lang=\"en\"><head>"},  // packets
        Verdict::kBlock,         // verdict
        1,                       // verdict_packet
    },
    {
        "Blocked: Cross-site XHR to HTML with wrong CORS",
        __LINE__,
        "http://www.b.com/resource.html",    // target_url
        RESOURCE_TYPE_XHR,                   // resource_type
        "http://www.a.com/",                 // initiator_origin
        OriginHeader::kInclude,              // cors_request
        "text/html",                         // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,  // canonical_mime_type
        false,                               // include_no_sniff_header
        AccessControlAllowOriginHeader::kAllowExampleDotCom,  // cors_response
        {"<hTmL><head>this should sniff as HTML"},            // packets
        Verdict::kBlock,                                      // verdict
        0,                                                    // verdict_packet
    },
    {
        "Blocked(-ish?): Nosniff header + empty response",
        __LINE__,
        "http://www.b.com/resource.html",       // target_url
        RESOURCE_TYPE_XHR,                      // resource_type
        "http://www.a.com/",                    // initiator_origin
        OriginHeader::kInclude,                 // cors_request
        "text/html",                            // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,     // canonical_mime_type
        true,                                   // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,  // cors_response
        {},                                     // packets
        Verdict::kBlock,                        // verdict
        -1,                                     // verdict_packet
    },
    {
        "Blocked: Cross-site fetch HTML from NaCl without CORS response",
        __LINE__,
        "http://www.b.com/plugin.html",           // target_url
        RESOURCE_TYPE_PLUGIN_RESOURCE,            // resource_type
        "http://www.a.com/",                      // initiator_origin
        OriginHeader::kInclude,                   // cors_request
        "text/html",                              // response_mime_type
        CROSS_SITE_DOCUMENT_MIME_TYPE_HTML,       // canonical_mime_type
        false,                                    // include_no_sniff_header
        AccessControlAllowOriginHeader::kOmit,    // cors_response
        "<html><head>this should sniff as HTML",  // first_chunk
        Verdict::kBlockAfterSniffing,             // verdict
    },
};

}  // namespace

// Tests that verify CrossSiteDocumentResourceHandler correctly classifies
// network responses as allowed or blocked, and ensures that empty responses are
// sent for the blocked cases.
//
// The various test cases are passed as a list of TestScenario structs.
class CrossSiteDocumentResourceHandlerTest
    : public testing::Test,
      public testing::WithParamInterface<TestScenario> {
 public:
  CrossSiteDocumentResourceHandlerTest()
      : stream_sink_status_(
            net::URLRequestStatus::FromError(net::ERR_IO_PENDING)) {
    IsolateAllSitesForTesting(base::CommandLine::ForCurrentProcess());
  }

  // Sets up the request, downstream ResourceHandler, test ResourceHandler, and
  // ResourceLoader.
  void Initialize(const std::string& target_url,
                  ResourceType resource_type,
                  const std::string& initiator_origin,
                  OriginHeader cors_request) {
    stream_sink_status_ = net::URLRequestStatus::FromError(net::ERR_IO_PENDING);

    // Initialize |request_| from the parameters.
    request_ = context_.CreateRequest(GURL(target_url), net::DEFAULT_PRIORITY,
                                      &delegate_, TRAFFIC_ANNOTATION_FOR_TESTS);
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
    auto stream_sink = std::make_unique<TestResourceHandler>(
        &stream_sink_status_, &stream_sink_body_);
    stream_sink_ = stream_sink->GetWeakPtr();

    // Create the CrossSiteDocumentResourceHandler.
    bool is_nocors_plugin_request =
        resource_type == RESOURCE_TYPE_PLUGIN_RESOURCE &&
        cors_request == OriginHeader::kOmit;
    document_blocker_ = std::make_unique<CrossSiteDocumentResourceHandler>(
        std::move(stream_sink), request_.get(), is_nocors_plugin_request);

    // Create a mock loader to drive the CrossSiteDocumentResourceHandler.
    mock_loader_ =
        std::make_unique<MockResourceLoader>(document_blocker_.get());
  }

  // Returns a ResourceResponse that matches the TestScenario's parameters.
  scoped_refptr<ResourceResponse> CreateResponse(
      const char* response_mime_type,
      bool include_no_sniff_header,
      AccessControlAllowOriginHeader cors_response,
      const char* initiator_origin) {
    scoped_refptr<ResourceResponse> response =
        base::MakeRefCounted<ResourceResponse>();
    response->head.mime_type = response_mime_type;
    scoped_refptr<net::HttpResponseHeaders> response_headers =
        base::MakeRefCounted<net::HttpResponseHeaders>("");

    // No sniff header.
    if (include_no_sniff_header)
      response_headers->AddHeader("X-Content-Type-Options: nosniff");

    // CORS header.
    if (cors_response == AccessControlAllowOriginHeader::kAllowAny) {
      response_headers->AddHeader("Access-Control-Allow-Origin: *");
    } else if (cors_response ==
               AccessControlAllowOriginHeader::kAllowInitiatorOrigin) {
      response_headers->AddHeader(base::StringPrintf(
          "Access-Control-Allow-Origin: %s", initiator_origin));
    } else if (cors_response == AccessControlAllowOriginHeader::kAllowNull) {
      response_headers->AddHeader("Access-Control-Allow-Origin: null");
    } else if (cors_response ==
               AccessControlAllowOriginHeader::kAllowExampleDotCom) {
      response_headers->AddHeader(
          "Access-Control-Allow-Origin: http://example.com");
    }

    response->head.headers = response_headers;

    return response;
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;
  net::TestURLRequestContext context_;
  net::TestDelegate delegate_;
  std::unique_ptr<net::URLRequest> request_;

  // |stream_sink_| is the handler that's immediately after |document_blocker_|
  // in the ResourceHandler chain; it records the values passed to it into
  // |stream_sink_status_| and |stream_sink_body_|, which our tests assert
  // against.
  //
  // |stream_sink_| is owned by |document_blocker_|, but we retain a reference
  // to it.
  base::WeakPtr<TestResourceHandler> stream_sink_;
  net::URLRequestStatus stream_sink_status_;
  std::string stream_sink_body_;

  // |document_blocker_| is the CrossSiteDocuemntResourceHandler instance under
  // test.
  std::unique_ptr<CrossSiteDocumentResourceHandler> document_blocker_;

  // |mock_loader_| is the mock loader used to drive |document_blocker_|.
  std::unique_ptr<MockResourceLoader> mock_loader_;
};

// Runs a particular TestScenario (passed as the test's parameter) through the
// ResourceLoader and CrossSiteDocumentResourceHandler, verifying that the
// response is correctly allowed or blocked based on the scenario.
TEST_P(CrossSiteDocumentResourceHandlerTest, ResponseBlocking) {
  const TestScenario scenario = GetParam();
  SCOPED_TRACE(testing::Message()
               << "\nScenario at " << __FILE__ << ":" << scenario.source_line);

  Initialize(scenario.target_url, scenario.resource_type,
             scenario.initiator_origin, scenario.cors_request);
  base::HistogramTester histograms;

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));

  // Set up response based on scenario.
  scoped_refptr<ResourceResponse> response = CreateResponse(
      scenario.response_mime_type, scenario.include_no_sniff_header,
      scenario.cors_response, scenario.initiator_origin);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseStarted(response));

  // Verify MIME type was classified correctly.
  EXPECT_EQ(scenario.canonical_mime_type,
            document_blocker_->canonical_mime_type_);

  // Verify that we will sniff content into a different buffer if sniffing is
  // needed.  Note that the different buffer is used even for blocking cases
  // where no sniffing is needed, to avoid complexity in the handler.  The
  // handler doesn't look at the data in that case, but there's no way to verify
  // it in the test.
  bool expected_to_sniff = scenario.verdict_packet >= 0;
  EXPECT_EQ(expected_to_sniff, document_blocker_->needs_sniffing_);

  // Verify that we correctly decide whether to block based on headers.  Note
  // that this includes cases that will later be allowed after sniffing.
  bool expected_to_block_based_on_headers =
      expected_to_sniff || scenario.verdict == Verdict::kBlock;
  EXPECT_EQ(expected_to_block_based_on_headers,
            document_blocker_->should_block_based_on_headers_);

  // Tell the ResourceHandlers to allocate the buffer for reading.  In this
  // test, the buffer will be allocated immediately by the downstream handler
  // and possibly replaced by a different buffer for sniffing.
  bool should_be_blocked = scenario.verdict == Verdict::kBlock;
  int expected_sniff_bytes = 0;
  int num_packets = static_cast<int>(scenario.packets.size());
  int effective_verdict_packet = scenario.verdict_packet;
  if (should_be_blocked) {
    // Our implementation currently only blocks at the second OnWillRead.
    effective_verdict_packet = std::max(0, effective_verdict_packet);
  }
  for (int i = 0; i <= num_packets; i++) {
    SCOPED_TRACE(testing::Message() << "While delivering packet #" << i);
    const base::StringPiece packet =
        (i == num_packets) ? "" : *(scenario.packets.begin() + i);

    if (i <= effective_verdict_packet) {
      EXPECT_FALSE(document_blocker_->blocked_read_completed_);
      EXPECT_FALSE(document_blocker_->allow_based_on_sniffing_);
      expected_sniff_bytes += packet.length();
    } else {
      ASSERT_EQ(should_be_blocked, document_blocker_->blocked_read_completed_);
      EXPECT_EQ(!should_be_blocked && expected_to_sniff,
                document_blocker_->allow_based_on_sniffing_);
    }

    mock_loader_->OnWillRead();
    if (should_be_blocked && i == effective_verdict_packet + 1) {
      EXPECT_EQ(MockResourceLoader::Status::CANCELED, mock_loader_->status());
      break;
    }

    EXPECT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());
    EXPECT_NE(nullptr, mock_loader_->io_buffer());
    if (i <= effective_verdict_packet) {
      EXPECT_EQ(1, stream_sink_->on_will_read_called());
      EXPECT_EQ(mock_loader_->io_buffer()->data(),
                document_blocker_->local_buffer_->data() +
                    document_blocker_->local_buffer_bytes_read_)
          << "Should have used a different IOBuffer for sniffing";
    } else {
      EXPECT_EQ(1 + i - (expected_to_sniff ? scenario.verdict_packet : 0),
                stream_sink_->on_will_read_called());
      EXPECT_FALSE(should_be_blocked);
      EXPECT_EQ(mock_loader_->io_buffer(), stream_sink_->buffer())
          << "Should have used original IOBuffer when sniffing not needed";
    }

    mock_loader_->OnReadCompleted(packet);

    // Deliver the next packet of the response body; this allows sniffing to
    // occur.
    if (mock_loader_->status() ==
        MockResourceLoader::Status::CALLBACK_PENDING) {
      // CALLBACK_PENDING is only expected in the case where an allow decision
      // is made during the final empty packet.
      EXPECT_FALSE(should_be_blocked);
      EXPECT_EQ(i, num_packets);
      EXPECT_EQ(num_packets, scenario.verdict_packet);
      EXPECT_EQ("", packet);
      mock_loader_->WaitUntilIdleOrCanceled();
    }
    EXPECT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());
    EXPECT_EQ(nullptr, mock_loader_->io_buffer());

    if (should_be_blocked) {
      EXPECT_FALSE(document_blocker_->allow_based_on_sniffing_);
      EXPECT_EQ("", stream_sink_body_)
          << "Response should not have been delivered to the renderer.";
      EXPECT_LE(i, effective_verdict_packet);
      EXPECT_EQ(i == effective_verdict_packet,
                document_blocker_->blocked_read_completed_);
    } else if (expected_to_sniff && i >= scenario.verdict_packet) {
      EXPECT_TRUE(document_blocker_->allow_based_on_sniffing_);
      EXPECT_FALSE(document_blocker_->blocked_read_completed_);
    } else {
      EXPECT_FALSE(document_blocker_->allow_based_on_sniffing_);
      EXPECT_FALSE(document_blocker_->blocked_read_completed_);
    }
    if (scenario.verdict == Verdict::kAllow && i >= scenario.verdict_packet) {
      EXPECT_EQ(packet, stream_sink_body_.substr(stream_sink_body_.size() -
                                                 packet.size()))
          << "Response should be streamed to the renderer.";
    }
  }

  // Send OnResponseCompleted.
  if (should_be_blocked) {
    EXPECT_EQ(1, stream_sink_->on_will_read_called());
    if (!scenario.data().empty()) {
      // TODO(nick): We may be left in an inconsistent state when blocking an
      // empty nosniff response. Remove the above 'if'.
      EXPECT_EQ(MockResourceLoader::Status::CANCELED, mock_loader_->status());
    }
    net::URLRequestStatus status(net::URLRequestStatus::CANCELED,
                                 net::ERR_ABORTED);
    EXPECT_EQ(stream_sink_body_, "");

    ASSERT_EQ(MockResourceLoader::Status::IDLE,
              mock_loader_->OnResponseCompleted(status));

    EXPECT_EQ(stream_sink_body_, "");
  } else {
    if (!expected_to_sniff) {
      EXPECT_EQ(num_packets + 1, stream_sink_->on_will_read_called());
    } else if (scenario.verdict_packet == num_packets && num_packets > 0) {
      EXPECT_EQ(2, stream_sink_->on_will_read_called());
    } else {
      EXPECT_EQ(1 + (num_packets - scenario.verdict_packet),
                stream_sink_->on_will_read_called());
    }

    EXPECT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());
    EXPECT_EQ(stream_sink_body_, scenario.data());

    ASSERT_EQ(MockResourceLoader::Status::IDLE,
              mock_loader_->OnResponseCompleted(
                  net::URLRequestStatus::FromError(net::OK)));
  }

  // Check the final block/no-block decision.
  EXPECT_EQ(document_blocker_->blocked_read_completed_, should_be_blocked);
  EXPECT_EQ(document_blocker_->allow_based_on_sniffing_,
            !should_be_blocked && expected_to_sniff);
  if (should_be_blocked)
    EXPECT_EQ(stream_sink_body_, "");
  else
    EXPECT_EQ(stream_sink_body_, scenario.data());

  // Verify that histograms are correctly incremented.
  base::HistogramTester::CountsMap expected_counts;
  std::string histogram_base = "SiteIsolation.XSD.Browser";
  std::string bucket;
  switch (scenario.canonical_mime_type) {
    case CROSS_SITE_DOCUMENT_MIME_TYPE_HTML:
      bucket = "HTML";
      break;
    case CROSS_SITE_DOCUMENT_MIME_TYPE_XML:
      bucket = "XML";
      break;
    case CROSS_SITE_DOCUMENT_MIME_TYPE_JSON:
      bucket = "JSON";
      break;
    case CROSS_SITE_DOCUMENT_MIME_TYPE_PLAIN:
      bucket = "Plain";
      break;
    case CROSS_SITE_DOCUMENT_MIME_TYPE_OTHERS:
      EXPECT_FALSE(should_be_blocked);
      bucket = "Others";
      break;
    default:
      NOTREACHED();
  }
  int start_action = static_cast<int>(
      CrossSiteDocumentResourceHandler::Action::kResponseStarted);
  int end_action = -1;
  if (should_be_blocked && expected_to_sniff) {
    end_action = static_cast<int>(
        CrossSiteDocumentResourceHandler::Action::kBlockedAfterSniffing);
  } else if (should_be_blocked && !expected_to_sniff) {
    end_action = static_cast<int>(
        CrossSiteDocumentResourceHandler::Action::kBlockedWithoutSniffing);
  } else if (!should_be_blocked && expected_to_sniff) {
    end_action = static_cast<int>(
        CrossSiteDocumentResourceHandler::Action::kAllowedAfterSniffing);
  } else if (!should_be_blocked && !expected_to_sniff) {
    end_action = static_cast<int>(
        CrossSiteDocumentResourceHandler::Action::kAllowedWithoutSniffing);
  } else {
    NOTREACHED();
  }
  // Expecting two actions: ResponseStarted and one of the outcomes.
  expected_counts[histogram_base + ".Action"] = 2;
  EXPECT_THAT(histograms.GetAllSamples(histogram_base + ".Action"),
              testing::ElementsAre(base::Bucket(start_action, 1),
                                   base::Bucket(end_action, 1)))
      << "Should have incremented the right actions.";
  // Expect to hear the number of bytes in the first read when sniffing is
  // required.
  if (expected_to_sniff) {
    expected_counts[histogram_base + ".BytesReadForSniffing"] = 1;

    // Only the packets up to verdict_packet are sniffed.
    EXPECT_EQ(
        1, histograms.GetBucketCount(histogram_base + ".BytesReadForSniffing",
                                     expected_sniff_bytes));
  }
  if (should_be_blocked) {
    expected_counts[histogram_base + ".Blocked"] = 1;
    expected_counts[histogram_base + ".Blocked." + bucket] = 1;
    EXPECT_THAT(histograms.GetAllSamples(histogram_base + ".Blocked"),
                testing::ElementsAre(base::Bucket(scenario.resource_type, 1)))
        << "Should have incremented aggregate blocking.";
    EXPECT_THAT(histograms.GetAllSamples(histogram_base + ".Blocked." + bucket),
                testing::ElementsAre(base::Bucket(scenario.resource_type, 1)))
        << "Should have incremented blocking for resource type.";
  }
  // Make sure that the expected metrics, and only those metrics, were
  // incremented.
  EXPECT_THAT(histograms.GetTotalCountsForPrefix("SiteIsolation.XSD.Browser"),
              testing::ContainerEq(expected_counts));
}

// Similar to the ResponseBlocking test above, but simulates the case that the
// downstream handler does not immediately resume from OnWillRead, in which case
// the downstream buffer may not be allocated until later.
TEST_P(CrossSiteDocumentResourceHandlerTest, OnWillReadDefer) {
  const TestScenario scenario = GetParam();
  SCOPED_TRACE(testing::Message()
               << "\nScenario at " << __FILE__ << ":" << scenario.source_line);

  Initialize(scenario.target_url, scenario.resource_type,
             scenario.initiator_origin, scenario.cors_request);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));

  // Set up response based on scenario.
  scoped_refptr<ResourceResponse> response = CreateResponse(
      scenario.response_mime_type, scenario.include_no_sniff_header,
      scenario.cors_response, scenario.initiator_origin);

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseStarted(response));

  // Verify that we will sniff content into a different buffer if sniffing is
  // needed.  Note that the different buffer is used even for blocking cases
  // where no sniffing is needed, to avoid complexity in the handler.  The
  // handler doesn't look at the data in that case, but there's no way to verify
  // it in the test.
  bool expected_to_sniff = (scenario.verdict_packet != -1);
  EXPECT_EQ(expected_to_sniff, document_blocker_->needs_sniffing_);

  // Cause the TestResourceHandler to defer when OnWillRead is called, to make
  // sure the test scenarios still work when the downstream handler's buffer
  // isn't allocated in the same call.
  size_t bytes_delivered = 0;
  int buffer_requests = 0;
  int packets = 0;
  for (const std::string& packet : scenario.packets) {
    stream_sink_->set_defer_on_will_read(true);
    mock_loader_->OnWillRead();
    if (bytes_delivered == 0 || (scenario.verdict == Verdict::kAllow &&
                                 packets > scenario.verdict_packet)) {
      EXPECT_EQ(++buffer_requests, stream_sink_->on_will_read_called());
      ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
                mock_loader_->status());

      // No buffers have been allocated yet.
      EXPECT_EQ(nullptr, mock_loader_->io_buffer());
      EXPECT_EQ(nullptr, document_blocker_->local_buffer_.get());

      // Resume the downstream handler, which should establish a buffer for the
      // ResourceLoader (either the downstream one or a local one for sniffing).
      stream_sink_->WaitUntilDeferred();
      stream_sink_->Resume();
    } else {
      if (document_blocker_->blocked_read_completed_) {
        EXPECT_EQ(Verdict::kBlock, scenario.verdict);
        EXPECT_EQ(MockResourceLoader::Status::CANCELED, mock_loader_->status());
        break;
      } else {
        EXPECT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());
      }
    }
    EXPECT_NE(nullptr, mock_loader_->io_buffer());

    if (expected_to_sniff || scenario.verdict == Verdict::kBlock) {
      EXPECT_EQ(mock_loader_->io_buffer()->data(),
                document_blocker_->local_buffer_->data() + bytes_delivered)
          << "Should have used a different IOBuffer for sniffing";
    } else {
      EXPECT_EQ(mock_loader_->io_buffer(), stream_sink_->buffer())
          << "Should have used original IOBuffer when sniffing not needed";
    }
    // Deliver the next packet of the response body.
    ASSERT_EQ(MockResourceLoader::Status::IDLE,
              mock_loader_->OnReadCompleted(packet));
    bytes_delivered += packet.size();
    packets++;
    EXPECT_EQ(nullptr, mock_loader_->io_buffer());
  }

  // Verify that the response is blocked or allowed as expected.
  if (scenario.verdict == Verdict::kBlock) {
    EXPECT_EQ("", stream_sink_body_)
        << "Response should not have been delivered to the renderer.";
    if (packets > 0) {
      EXPECT_TRUE(document_blocker_->blocked_read_completed_);
    } else {
      EXPECT_FALSE(document_blocker_->blocked_read_completed_);
    }
    EXPECT_FALSE(document_blocker_->allow_based_on_sniffing_);
  } else {
    stream_sink_->set_defer_on_will_read(true);
    mock_loader_->OnWillRead();
    if (packets == 0 || packets > scenario.verdict_packet) {
      stream_sink_->WaitUntilDeferred();
      stream_sink_->Resume();
    }
    mock_loader_->WaitUntilIdleOrCanceled();
    if (packets == scenario.verdict_packet && bytes_delivered > 0) {
      // This case will result in CrossSiteDocumentResourceHandler having to
      // synthesize an extra OnWillRead.
      stream_sink_->set_defer_on_will_read(true);
      mock_loader_->OnReadCompleted("");
      stream_sink_->WaitUntilDeferred();
      stream_sink_->Resume();
    } else {
      ASSERT_EQ(MockResourceLoader::Status::IDLE,
                mock_loader_->OnReadCompleted(""));
    }
    mock_loader_->WaitUntilIdleOrCanceled();
    EXPECT_EQ(scenario.data(), stream_sink_body_)
        << "Response should have been delivered to the renderer.";
    EXPECT_FALSE(document_blocker_->blocked_read_completed_);
    EXPECT_EQ(expected_to_sniff, document_blocker_->allow_based_on_sniffing_);
  }
}

INSTANTIATE_TEST_CASE_P(,
                        CrossSiteDocumentResourceHandlerTest,
                        ::testing::ValuesIn(kScenarios));

}  // namespace content
