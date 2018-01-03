// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mojo/common/time_struct_traits.h"
#include "mojo/public/cpp/bindings/array_traits_wtf_vector.h"
#include "mojo/public/cpp/bindings/map_traits_wtf_hash_map.h"
#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "platform/mojo/KURLStructTraits.h"
#include "platform/mojo/ReferrerStructTraits.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/Source/platform/mojo/FetchAPIRequestStructTraits.h"
#include "third_party/WebKit/Source/platform/mojo/FetchAPIResponseStructTraits.h"

namespace blink {

namespace {

class FetchAPIResponseStructTraitsTests : public ::testing::Test {
 public:
  FetchAPIResponseStructTraitsTests() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FetchAPIResponseStructTraitsTests);
};

template <typename Map>
bool MapsEqual(const Map& lhs, const Map& rhs) {
  auto pred = [](decltype(*lhs.begin()) a, decltype(a) b) {
    return (a.key == b.key) && (a.value == b.value);
  };

  return lhs.size() == rhs.size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin(), pred);
}

template <typename T>
bool VectorsEqual(const T& lhs, const T& rhs) {
  if (lhs.size() != rhs.size())
    return false;
  for (size_t i = 0; i < lhs.size(); i++) {
    if (lhs[i] != rhs[i])
      return false;
  }
  return true;
}

}  // namespace

TEST_F(FetchAPIResponseStructTraitsTests, ResponseSerializeDefaultValues) {
  blink::WebServiceWorkerResponse input;
  blink::WebServiceWorkerResponse output;
  ASSERT_TRUE(mojom::blink::FetchAPIResponse::Deserialize(
      mojom::blink::FetchAPIResponse::Serialize(&input), &output));

  EXPECT_TRUE(VectorsEqual(input.UrlList(), output.UrlList()));
  EXPECT_EQ(input.Status(), output.Status());
  EXPECT_EQ(input.StatusText(), output.StatusText());
  EXPECT_EQ(input.ResponseType(), output.ResponseType());

  EXPECT_TRUE(MapsEqual(input.Headers(), output.Headers()));
  EXPECT_EQ(input.BlobUUID(), output.BlobUUID());
  EXPECT_EQ(input.BlobSize(), output.BlobSize());
  // TODO(cmumford): Test blob data.
  EXPECT_EQ(input.GetError(), output.GetError());
  EXPECT_EQ(input.ResponseTime(), output.ResponseTime());
  EXPECT_EQ(input.CacheStorageCacheName(), output.CacheStorageCacheName());
  EXPECT_TRUE(VectorsEqual(input.CorsExposedHeaderNames(),
                           output.CorsExposedHeaderNames()));
}

TEST_F(FetchAPIResponseStructTraitsTests, ResponseSerializeNonDefaultValues) {
  blink::WebServiceWorkerResponse input;

  // Set input to non-default values.
  input.SetError(
      mojom::ServiceWorkerResponseError::kResponseTypeNotBasicOrDefault);

  blink::WebServiceWorkerResponse output;
  ASSERT_TRUE(mojom::blink::FetchAPIResponse::Deserialize(
      mojom::blink::FetchAPIResponse::Serialize(&input), &output));

  EXPECT_TRUE(VectorsEqual(input.UrlList(), output.UrlList()));
  EXPECT_EQ(input.Status(), output.Status());
  EXPECT_EQ(input.StatusText(), output.StatusText());
  EXPECT_EQ(input.ResponseType(), output.ResponseType());

  EXPECT_TRUE(MapsEqual(input.Headers(), output.Headers()));
  EXPECT_EQ(input.BlobUUID(), output.BlobUUID());
  EXPECT_EQ(input.BlobSize(), output.BlobSize());
  // TODO(cmumford): Test blob data.
  EXPECT_EQ(input.GetError(), output.GetError());
  EXPECT_EQ(input.ResponseTime(), output.ResponseTime());
  EXPECT_EQ(input.CacheStorageCacheName(), output.CacheStorageCacheName());
  EXPECT_TRUE(VectorsEqual(input.CorsExposedHeaderNames(),
                           output.CorsExposedHeaderNames()));
}

TEST_F(FetchAPIResponseStructTraitsTests, RequestSerializeDefaultValues) {
  blink::WebServiceWorkerRequest input;
  blink::WebServiceWorkerRequest output;
#if 1
  input.SetMethod("GET");
  input.SetReferrer("http://www.referrer.com/", kWebReferrerPolicyAlways);
#endif
  ASSERT_TRUE(mojom::blink::FetchAPIRequest::Deserialize(
      mojom::blink::FetchAPIRequest::Serialize(&input), &output));

  EXPECT_EQ(input.Method(), output.Method());
#if 1
  EXPECT_FALSE(input.Url().IsValid());
  EXPECT_FALSE(output.Url().IsValid());
  EXPECT_TRUE(input.Url().GetString().IsNull());
  // Is this a bug in the URL mojom?
  EXPECT_FALSE(output.Url().GetString().IsNull());
#endif
  EXPECT_TRUE(MapsEqual(input.Headers(), output.Headers()));
  EXPECT_EQ(input.ReferrerUrl(), output.ReferrerUrl());
  EXPECT_EQ(input.GetReferrerPolicy(), output.GetReferrerPolicy());
  EXPECT_EQ(input.Mode(), output.Mode());
  EXPECT_EQ(input.IsMainResourceLoad(), output.IsMainResourceLoad());
  EXPECT_EQ(input.CredentialsMode(), output.CredentialsMode());
  EXPECT_EQ(input.Integrity(), output.Integrity());
  EXPECT_EQ(input.CacheMode(), output.CacheMode());
  EXPECT_EQ(input.RedirectMode(), output.RedirectMode());
  EXPECT_EQ(input.GetRequestContext(), output.GetRequestContext());
  EXPECT_EQ(input.GetFrameType(), output.GetFrameType());
  EXPECT_EQ(input.ClientId(), output.ClientId());
  EXPECT_EQ(input.IsReload(), output.IsReload());
}

TEST_F(FetchAPIResponseStructTraitsTests, RequestSerializeNonDefaultValues) {
  blink::WebServiceWorkerRequest input;
  blink::WebServiceWorkerRequest output;
  const WebString method = "GET";
  const WebString referrer = "http://www.referrer.com/";
  const WebURL url = KURL("http://www.request.com/");
  // Referrer deserialization will fail w/o valid method.
  input.SetMethod(method);
  // Referrer deserialization will fail w/o valid referrer.
  input.SetReferrer(referrer, kWebReferrerPolicyAlways);
  // Must set URL.
  input.SetURL(url);
  ASSERT_TRUE(mojom::blink::FetchAPIRequest::Deserialize(
      mojom::blink::FetchAPIRequest::Serialize(&input), &output));

  EXPECT_EQ(input.Url(), output.Url());
  EXPECT_EQ(input.Method(), output.Method());
  EXPECT_TRUE(MapsEqual(input.Headers(), output.Headers()));
  EXPECT_EQ(input.ReferrerUrl(), output.ReferrerUrl());
  EXPECT_EQ(input.GetReferrerPolicy(), output.GetReferrerPolicy());
  EXPECT_EQ(input.Mode(), output.Mode());
  EXPECT_EQ(input.IsMainResourceLoad(), output.IsMainResourceLoad());
  EXPECT_EQ(input.CredentialsMode(), output.CredentialsMode());
  EXPECT_EQ(input.Integrity(), output.Integrity());
  EXPECT_EQ(input.CacheMode(), output.CacheMode());
  EXPECT_EQ(input.RedirectMode(), output.RedirectMode());
  EXPECT_EQ(input.GetRequestContext(), output.GetRequestContext());
  EXPECT_EQ(input.GetFrameType(), output.GetFrameType());
  EXPECT_EQ(input.ClientId(), output.ClientId());
  EXPECT_EQ(input.IsReload(), output.IsReload());
}

}  // namespace blink
