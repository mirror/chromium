// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class OriginManifestTest : public testing::Test {};

namespace {

std::string csp_header = "Content-Security-Policy";

std::string manifest_json =
    "{"
    "  \"content-security-policy\": \"default-src baseline.com\","
    "  \"content-security-policy-fallback\": \"default-src fallback.com\","
    "  \"unsupported-directive\": \"some value\","
    "  \"unsupported-directive-fallback\": \"some fallback value\","
    "}";
}  // namespace

TEST_F(OriginManifestTest, CreateOriginManifestFromJson) {
  struct TestCase {
    std::string origin;
    std::string version;
    std::string json;
    bool isNullptr;
  } cases[]{{"https://a.com", "v1", "", true},
            {"https://a.com", "v1", "{}", false},
            {"https://a.com", "v1", manifest_json, false}};

  for (const auto& test : cases) {
    EXPECT_EQ(OriginManifest::CreateOriginManifest(test.origin, test.version,
                                                   test.json) == nullptr,
              test.isNullptr);
  }
}

TEST_F(OriginManifestTest, ApplyOriginManifestToRequest) {
  std::unique_ptr<OriginManifest> manifest =
      OriginManifest::CreateOriginManifest("https://a.com", "v1",
                                           manifest_json);
  ASSERT_NE(manifest, nullptr);

  // measure the expeted effects for the request
  // Currently it does not do anything at all. We don't implement CORS, HSTS,
  // etc yet.
  std::unique_ptr<net::HttpRequestHeaders> request_headers(
      new net::HttpRequestHeaders());
  std::string headers_str = request_headers->ToString();

  manifest->ApplyToRequest(request_headers.get());

  EXPECT_EQ(headers_str, request_headers->ToString());
}

TEST_F(OriginManifestTest, ApplyOriginManifestToRedirect) {
  std::unique_ptr<OriginManifest> manifest =
      OriginManifest::CreateOriginManifest("https://a.com", "v1",
                                           manifest_json);
  ASSERT_NE(manifest, nullptr);

  // measure the expeted effects for the redirect
  scoped_refptr<net::HttpResponseHeaders> response_headers(
      new net::HttpResponseHeaders(""));
  const std::string headers_str = response_headers->raw_headers();

  manifest->ApplyToRedirect(response_headers.get());

  EXPECT_EQ(headers_str, response_headers->raw_headers());
}

TEST_F(OriginManifestTest, ApplyNoRelevantHeaderInResponse) {
  std::unique_ptr<OriginManifest> manifest =
      OriginManifest::CreateOriginManifest("https://a.com", "v1",
                                           manifest_json);
  ASSERT_NE(manifest, nullptr);

  scoped_refptr<net::HttpResponseHeaders> response_headers(
      new net::HttpResponseHeaders(""));

  manifest->ApplyToResponse(response_headers.get());

  EXPECT_TRUE(
      response_headers->HasHeaderValue(csp_header, "default-src baseline.com"));
  EXPECT_TRUE(
      response_headers->HasHeaderValue(csp_header, "default-src fallback.com"));
  EXPECT_FALSE(response_headers->HasHeader("unsupported-directive"));
}

TEST_F(OriginManifestTest, ApplyWithRelevantHeaderInResponse) {
  std::unique_ptr<OriginManifest> manifest =
      OriginManifest::CreateOriginManifest("https://a.com", "v1",
                                           manifest_json);
  ASSERT_NE(manifest, nullptr);

  std::string response_csp_value = "default-src a.com";
  scoped_refptr<net::HttpResponseHeaders> response_headers(
      new net::HttpResponseHeaders(""));
  response_headers->AddHeader(csp_header + ": " + response_csp_value);

  manifest->ApplyToResponse(response_headers.get());

  EXPECT_TRUE(response_headers->HasHeaderValue(csp_header, response_csp_value));
  EXPECT_TRUE(
      response_headers->HasHeaderValue(csp_header, "default-src baseline.com"));
  EXPECT_FALSE(
      response_headers->HasHeaderValue(csp_header, "default-src fallback.com"));
  EXPECT_FALSE(response_headers->HasHeader("unsupported-directive"));
}

TEST_F(OriginManifestTest, RevokeOriginManifest) {
  std::unique_ptr<OriginManifest> manifest =
      OriginManifest::CreateOriginManifest("https://a.com", "v1",
                                           manifest_json);
  ASSERT_NE(manifest, nullptr);

  // measure the expeted effects
  // well, it literally does not do anything, so nothing to measure
  manifest->Revoke();
}

}  // namespace content
