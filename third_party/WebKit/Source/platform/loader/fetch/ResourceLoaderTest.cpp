// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/RawResource.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "platform/loader/fetch/ResourceLoader.h"
#include "platform/loader/fetch/ResourceResponse.h"
#include "platform/loader/testing/MockFetchContext.h"
#include "platform/testing/TestingPlatformSupport.h"
#include "platform/wtf/text/StringBuilder.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class ResourceLoaderTest : public ::testing::Test {
  DISALLOW_COPY_AND_ASSIGN(ResourceLoaderTest);

 public:
  ResourceLoaderTest()
      : fooUrl(kParsedURLString, "https://foo.test"),
        barUrl(kParsedURLString, "https://bar.test"){};

  void SetUp() {
    context_ =
        MockFetchContext::Create(MockFetchContext::kShouldLoadNewResource);
  }

 protected:
  enum ServiceWorkerMode { kNoSW, kSWOpaque, kSWClear };

  struct TestCase {
    const KURL& origin;
    const KURL& target;
    const KURL* allow_origin_url;
    const ServiceWorkerMode service_worker;
    const Resource::Type resourceTyp;
    const CORSStatus expectation;
  };

  Persistent<MockFetchContext> context_;

  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;

  KURL fooUrl;
  KURL barUrl;
};

TEST_F(ResourceLoaderTest, DetermineCORSStatus) {
  TestCase cases[] = {
      // No CORS status for main resources:
      {fooUrl, fooUrl, nullptr, kNoSW, Resource::Type::kMainResource,
       CORSStatus::kNotApplicable},

      // Same origin:
      {fooUrl, fooUrl, nullptr, kNoSW, Resource::Type::kRaw,
       CORSStatus::kSameOrigin},

      // Cross origin CORS successful:
      {fooUrl, barUrl, &fooUrl, kNoSW, Resource::Type::kRaw,
       CORSStatus::kSuccessful},

      // Cross origin CORS failed:
      {fooUrl, barUrl, nullptr, kNoSW, Resource::Type::kRaw,
       CORSStatus::kFailed},
      {fooUrl, barUrl, &barUrl, kNoSW, Resource::Type::kRaw,
       CORSStatus::kFailed},

      // CORS handled by service worker
      {fooUrl, fooUrl, nullptr, kSWClear, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerSuccessful},
      {fooUrl, fooUrl, &fooUrl, kSWClear, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerSuccessful},
      {fooUrl, barUrl, nullptr, kSWClear, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerSuccessful},
      {fooUrl, barUrl, &fooUrl, kSWClear, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerSuccessful},

      // Opaque response by service worker
      {fooUrl, fooUrl, nullptr, kSWOpaque, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerOpaque},
      {fooUrl, barUrl, nullptr, kSWOpaque, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerOpaque},
      {fooUrl, barUrl, &fooUrl, kSWOpaque, Resource::Type::kRaw,
       CORSStatus::kServiceWorkerOpaque},
  };

  for (const auto& test : cases) {
    SCOPED_TRACE(
        ::testing::Message()
        << "Origin: " << test.origin.BaseAsString()
        << ", target: " << test.target.BaseAsString()
        << ", CORS access-control-allow-origin header: "
        << (test.allow_origin_url ? test.allow_origin_url->BaseAsString() : "-")
        << ", service worker: "
        << (test.service_worker == kNoSW
                ? "no"
                : (test.service_worker == kSWClear ? "clear response"
                                                   : "opaque response"))
        << ", expected CORSStatus =="
        << static_cast<unsigned>(test.expectation));

    context_->SetSecurityOrigin(SecurityOrigin::Create(test.origin));
    ResourceFetcher* fetcher =
        ResourceFetcher::Create(context_, context_->GetTaskRunner().Get());

    Resource* resource =
        RawResource::CreateForTest(test.target, test.resourceTyp);
    ResourceLoader* loader = ResourceLoader::Create(fetcher, resource);

    ResourceRequest request;
    request.SetURL(test.target);

    ResourceResponse response;
    response.SetHTTPStatusCode(200);
    response.SetURL(test.target);

    if (test.allow_origin_url) {
      request.SetFetchRequestMode(WebURLRequest::kFetchRequestModeCORS);
      resource->MutableOptions().cors_handling_by_resource_fetcher =
          kEnableCORSHandlingByResourceFetcher;
      response.SetHTTPHeaderField(
          "access-control-allow-origin",
          SecurityOrigin::Create(*test.allow_origin_url)->ToAtomicString());
      response.SetHTTPHeaderField("access-control-allow-credentials", "true");
    }

    resource->SetResourceRequest(request);

    if (test.service_worker != kNoSW) {
      response.SetWasFetchedViaServiceWorker(true);

      if (test.service_worker == kSWOpaque) {
        response.SetServiceWorkerResponseType(
            WebServiceWorkerResponseType::kWebServiceWorkerResponseTypeOpaque);
      } else {
        response.SetServiceWorkerResponseType(
            WebServiceWorkerResponseType::kWebServiceWorkerResponseTypeDefault);
      }
    }

    StringBuilder cors_error_msg;
    CORSStatus cors_status =
        loader->DetermineCORSStatus(response, cors_error_msg);

    EXPECT_EQ(cors_status, test.expectation);
  }
}
}  // namespace blink
