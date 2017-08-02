// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/net/cross_origin_access_control.h"
#include "content/renderer/net/str_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

std::string GetHeaderValue(const ResourceRequest request,
                           const std::string name) {
  net::HttpRequestHeaders headers;
  headers.AddHeadersFromString(request.headers);

  std::string value;
  DCHECK(headers.GetHeader(name, &value));

  return value;
}

TEST(CreateAccessControlPreflightRequestTest, LexicographicalOrder) {
  net::HttpRequestHeaders headers;
  headers.SetHeader("Orange", "Orange");
  headers.SetHeader("Apple", "Red");
  headers.SetHeader("Kiwifruit", "Green");
  headers.SetHeader("Content-Type", "application/octet-stream");
  headers.SetHeader("Strawberry", "Red");

  ResourceRequest request;
  request.headers = headers.ToString();

  ResourceRequest preflight =
      CrossOriginAccessControl::CreateAccessControlPreflightRequest(request);

  EXPECT_EQ("apple,content-type,kiwifruit,orange,strawberry",
            GetHeaderValue(preflight, "Access-Control-Request-Headers"));
}

TEST(CreateAccessControlPreflightRequestTest, ExcludeSimpleHeaders) {
  net::HttpRequestHeaders headers;
  headers.SetHeader("Accept", "everything");
  headers.SetHeader("Accept-Language", "everything");
  headers.SetHeader("Content-Language", "everything");
  headers.SetHeader("Save-Data", "on");

  ResourceRequest request;
  request.headers = headers.ToString();

  ResourceRequest preflight =
      CrossOriginAccessControl::CreateAccessControlPreflightRequest(request);

  net::HttpRequestHeaders preflight_headers;
  preflight_headers.AddHeadersFromString(preflight.headers);

  // Do not emit empty-valued headers; an empty list of non-"CORS safelisted"
  // request headers should cause "Access-Control-Request-Headers:" to be
  // left out in the preflight request.
  std::string ignored_value;
  EXPECT_FALSE(preflight_headers.GetHeader("Access-Control-Request-Headers",
                                           &ignored_value));
}

TEST(CreateAccessControlPreflightRequestTest, ExcludeSimpleContentTypeHeader) {
  ResourceRequest request;
  request.headers = "Content-Type: text/plain";

  ResourceRequest preflight =
      CrossOriginAccessControl::CreateAccessControlPreflightRequest(request);

  net::HttpRequestHeaders preflight_headers;
  preflight_headers.AddHeadersFromString(preflight.headers);

  // Empty list also; see comment in test above.
  std::string ignored_value;
  EXPECT_FALSE(preflight_headers.GetHeader("Access-Control-Request-Headers",
                                           &ignored_value));
}

TEST(CreateAccessControlPreflightRequestTest, IncludeNonSimpleHeader) {
  ResourceRequest request;
  request.headers = "X-Custom-Header: foobar";

  ResourceRequest preflight =
      CrossOriginAccessControl::CreateAccessControlPreflightRequest(request);

  EXPECT_EQ("x-custom-header",
            GetHeaderValue(preflight, "Access-Control-Request-Headers"));
}

TEST(CreateAccessControlPreflightRequestTest,
     IncludeNonSimpleContentTypeHeader) {
  ResourceRequest request;
  request.headers = "Content-Type: application/octet-stream";

  ResourceRequest preflight =
      CrossOriginAccessControl::CreateAccessControlPreflightRequest(request);

  EXPECT_EQ("content-type",
            GetHeaderValue(preflight, "Access-Control-Request-Headers"));
}

TEST(ParseAccessControlExposeHeadersAllowListTest, ValidInput) {
  IgnoreCaseStringSet set;
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList("valid",
                                                                     set);
  EXPECT_EQ(1U, set.size());
  EXPECT_TRUE(set.find("valid") != set.end());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList("a,b",
                                                                     set);
  EXPECT_EQ(2U, set.size());
  EXPECT_TRUE(set.find("a") != set.end());
  EXPECT_TRUE(set.find("b") != set.end());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList(
      "   a ,  b ", set);
  EXPECT_EQ(2U, set.size());
  EXPECT_TRUE(set.find("a") != set.end());
  EXPECT_TRUE(set.find("b") != set.end());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList(
      " \t   \t\t a", set);
  EXPECT_EQ(1U, set.size());
  EXPECT_TRUE(set.find("a") != set.end());
}

TEST(ParseAccessControlExposeHeadersAllowListTest, DuplicatedEntries) {
  IgnoreCaseStringSet set;
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList("a, a",
                                                                     set);
  EXPECT_EQ(1U, set.size());
  EXPECT_TRUE(set.find("a") != set.end());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList("A, a",
                                                                     set);
  EXPECT_EQ(1U, set.size());
  EXPECT_TRUE(set.find("a") != set.end());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList("a, a, b",
                                                                     set);
  EXPECT_EQ(2U, set.size());
  EXPECT_TRUE(set.find("a") != set.end());
  EXPECT_TRUE(set.find("b") != set.end());
}

TEST(ParseAccessControlExposeHeadersAllowListTest, InvalidInput) {
  IgnoreCaseStringSet set;
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList(
      "not valid", set);
  EXPECT_TRUE(set.empty());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList("///",
                                                                     set);
  EXPECT_TRUE(set.empty());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList("/a/",
                                                                     set);
  EXPECT_TRUE(set.empty());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList(",", set);
  EXPECT_TRUE(set.empty());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList(" , ",
                                                                     set);
  EXPECT_TRUE(set.empty());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList(" , a",
                                                                     set);
  EXPECT_TRUE(set.empty());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList("a , ",
                                                                     set);
  EXPECT_TRUE(set.empty());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList("", set);
  EXPECT_TRUE(set.empty());

  set.clear();
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList(" ", set);
  EXPECT_TRUE(set.empty());

  set.clear();
  // U+0141 which is 'A' (0x41) + 0x100.
  CrossOriginAccessControl::ParseAccessControlExposeHeadersAllowList("\xC5\x81",
                                                                     set);
  EXPECT_TRUE(set.empty());
}
}  // namespace

}  // namespace content
