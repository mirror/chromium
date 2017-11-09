// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/generated/cronet.idl */

#include "components/cronet/native/generated/cronet.idl_c.h"

#include "base/logging.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

class CronetStructTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  CronetStructTest() {}
  ~CronetStructTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(CronetStructTest);
};

// Test Struct Buffer setters and getters.
TEST_F(CronetStructTest, TestBuffer) {
  BufferPtr first = Buffer_Create();
  BufferPtr second = Buffer_Create();

  // Copy values from |first| to |second|.
  Buffer_set_size(second, Buffer_get_size(first));
  EXPECT_EQ(Buffer_get_size(first), Buffer_get_size(second));
  Buffer_set_limit(second, Buffer_get_limit(first));
  EXPECT_EQ(Buffer_get_limit(first), Buffer_get_limit(second));
  Buffer_set_position(second, Buffer_get_position(first));
  EXPECT_EQ(Buffer_get_position(first), Buffer_get_position(second));
  Buffer_set_data(second, Buffer_get_data(first));
  EXPECT_EQ(Buffer_get_data(first), Buffer_get_data(second));
  Buffer_set_callback(second, Buffer_get_callback(first));
  EXPECT_EQ(Buffer_get_callback(first), Buffer_get_callback(second));
  Buffer_Destroy(first);
  Buffer_Destroy(second);
}

// Test Struct CronetException setters and getters.
TEST_F(CronetStructTest, TestCronetException) {
  CronetExceptionPtr first = CronetException_Create();
  CronetExceptionPtr second = CronetException_Create();

  // Copy values from |first| to |second|.
  CronetException_set_error_code(second, CronetException_get_error_code(first));
  EXPECT_EQ(CronetException_get_error_code(first),
            CronetException_get_error_code(second));
  CronetException_set_cronet_internal_error_code(
      second, CronetException_get_cronet_internal_error_code(first));
  EXPECT_EQ(CronetException_get_cronet_internal_error_code(first),
            CronetException_get_cronet_internal_error_code(second));
  CronetException_set_immediately_retryable(
      second, CronetException_get_immediately_retryable(first));
  EXPECT_EQ(CronetException_get_immediately_retryable(first),
            CronetException_get_immediately_retryable(second));
  CronetException_set_quic_detailed_error_code(
      second, CronetException_get_quic_detailed_error_code(first));
  EXPECT_EQ(CronetException_get_quic_detailed_error_code(first),
            CronetException_get_quic_detailed_error_code(second));
  CronetException_Destroy(first);
  CronetException_Destroy(second);
}

// Test Struct QuicHint setters and getters.
TEST_F(CronetStructTest, TestQuicHint) {
  QuicHintPtr first = QuicHint_Create();
  QuicHintPtr second = QuicHint_Create();

  // Copy values from |first| to |second|.
  QuicHint_set_host(second, QuicHint_get_host(first));
  EXPECT_STREQ(QuicHint_get_host(first), QuicHint_get_host(second));
  QuicHint_set_port(second, QuicHint_get_port(first));
  EXPECT_EQ(QuicHint_get_port(first), QuicHint_get_port(second));
  QuicHint_set_alternatePort(second, QuicHint_get_alternatePort(first));
  EXPECT_EQ(QuicHint_get_alternatePort(first),
            QuicHint_get_alternatePort(second));
  QuicHint_Destroy(first);
  QuicHint_Destroy(second);
}

// Test Struct PublicKeyPins setters and getters.
TEST_F(CronetStructTest, TestPublicKeyPins) {
  PublicKeyPinsPtr first = PublicKeyPins_Create();
  PublicKeyPinsPtr second = PublicKeyPins_Create();

  // Copy values from |first| to |second|.
  PublicKeyPins_set_host(second, PublicKeyPins_get_host(first));
  EXPECT_STREQ(PublicKeyPins_get_host(first), PublicKeyPins_get_host(second));
  // TODO(mef): Test array |pinsSha256|.
  PublicKeyPins_set_includeSubdomains(
      second, PublicKeyPins_get_includeSubdomains(first));
  EXPECT_EQ(PublicKeyPins_get_includeSubdomains(first),
            PublicKeyPins_get_includeSubdomains(second));
  PublicKeyPins_Destroy(first);
  PublicKeyPins_Destroy(second);
}

// Test Struct CronetEngineParams setters and getters.
TEST_F(CronetStructTest, TestCronetEngineParams) {
  CronetEngineParamsPtr first = CronetEngineParams_Create();
  CronetEngineParamsPtr second = CronetEngineParams_Create();

  // Copy values from |first| to |second|.
  CronetEngineParams_set_userAgent(second,
                                   CronetEngineParams_get_userAgent(first));
  EXPECT_STREQ(CronetEngineParams_get_userAgent(first),
               CronetEngineParams_get_userAgent(second));
  CronetEngineParams_set_storagePath(second,
                                     CronetEngineParams_get_storagePath(first));
  EXPECT_STREQ(CronetEngineParams_get_storagePath(first),
               CronetEngineParams_get_storagePath(second));
  CronetEngineParams_set_enableQuic(second,
                                    CronetEngineParams_get_enableQuic(first));
  EXPECT_EQ(CronetEngineParams_get_enableQuic(first),
            CronetEngineParams_get_enableQuic(second));
  CronetEngineParams_set_enableHttp2(second,
                                     CronetEngineParams_get_enableHttp2(first));
  EXPECT_EQ(CronetEngineParams_get_enableHttp2(first),
            CronetEngineParams_get_enableHttp2(second));
  CronetEngineParams_set_enableBrotli(
      second, CronetEngineParams_get_enableBrotli(first));
  EXPECT_EQ(CronetEngineParams_get_enableBrotli(first),
            CronetEngineParams_get_enableBrotli(second));
  CronetEngineParams_set_httpCacheMode(
      second, CronetEngineParams_get_httpCacheMode(first));
  EXPECT_EQ(CronetEngineParams_get_httpCacheMode(first),
            CronetEngineParams_get_httpCacheMode(second));
  CronetEngineParams_set_httpCacheMaxSize(
      second, CronetEngineParams_get_httpCacheMaxSize(first));
  EXPECT_EQ(CronetEngineParams_get_httpCacheMaxSize(first),
            CronetEngineParams_get_httpCacheMaxSize(second));
  // TODO(mef): Test array |quicHints|.
  // TODO(mef): Test array |publicKeyPins|.
  CronetEngineParams_set_enablePublicKeyPinningBypassForLocalTrustAnchors(
      second,
      CronetEngineParams_get_enablePublicKeyPinningBypassForLocalTrustAnchors(
          first));
  EXPECT_EQ(
      CronetEngineParams_get_enablePublicKeyPinningBypassForLocalTrustAnchors(
          first),
      CronetEngineParams_get_enablePublicKeyPinningBypassForLocalTrustAnchors(
          second));
  CronetEngineParams_Destroy(first);
  CronetEngineParams_Destroy(second);
}

// Test Struct HttpHeader setters and getters.
TEST_F(CronetStructTest, TestHttpHeader) {
  HttpHeaderPtr first = HttpHeader_Create();
  HttpHeaderPtr second = HttpHeader_Create();

  // Copy values from |first| to |second|.
  HttpHeader_set_name(second, HttpHeader_get_name(first));
  EXPECT_STREQ(HttpHeader_get_name(first), HttpHeader_get_name(second));
  HttpHeader_set_value(second, HttpHeader_get_value(first));
  EXPECT_STREQ(HttpHeader_get_value(first), HttpHeader_get_value(second));
  HttpHeader_Destroy(first);
  HttpHeader_Destroy(second);
}

// Test Struct UrlResponseInfo setters and getters.
TEST_F(CronetStructTest, TestUrlResponseInfo) {
  UrlResponseInfoPtr first = UrlResponseInfo_Create();
  UrlResponseInfoPtr second = UrlResponseInfo_Create();

  // Copy values from |first| to |second|.
  UrlResponseInfo_set_url(second, UrlResponseInfo_get_url(first));
  EXPECT_STREQ(UrlResponseInfo_get_url(first), UrlResponseInfo_get_url(second));
  // TODO(mef): Test array |urlChain|.
  UrlResponseInfo_set_httpStatusCode(second,
                                     UrlResponseInfo_get_httpStatusCode(first));
  EXPECT_EQ(UrlResponseInfo_get_httpStatusCode(first),
            UrlResponseInfo_get_httpStatusCode(second));
  UrlResponseInfo_set_httpStatusText(second,
                                     UrlResponseInfo_get_httpStatusText(first));
  EXPECT_STREQ(UrlResponseInfo_get_httpStatusText(first),
               UrlResponseInfo_get_httpStatusText(second));
  // TODO(mef): Test array |allHeadersList|.
  UrlResponseInfo_set_wasCached(second, UrlResponseInfo_get_wasCached(first));
  EXPECT_EQ(UrlResponseInfo_get_wasCached(first),
            UrlResponseInfo_get_wasCached(second));
  UrlResponseInfo_set_negotiatedProtocol(
      second, UrlResponseInfo_get_negotiatedProtocol(first));
  EXPECT_STREQ(UrlResponseInfo_get_negotiatedProtocol(first),
               UrlResponseInfo_get_negotiatedProtocol(second));
  UrlResponseInfo_set_proxyServer(second,
                                  UrlResponseInfo_get_proxyServer(first));
  EXPECT_STREQ(UrlResponseInfo_get_proxyServer(first),
               UrlResponseInfo_get_proxyServer(second));
  UrlResponseInfo_set_receivedByteCount(
      second, UrlResponseInfo_get_receivedByteCount(first));
  EXPECT_EQ(UrlResponseInfo_get_receivedByteCount(first),
            UrlResponseInfo_get_receivedByteCount(second));
  UrlResponseInfo_Destroy(first);
  UrlResponseInfo_Destroy(second);
}

// Test Struct UrlRequestParams setters and getters.
TEST_F(CronetStructTest, TestUrlRequestParams) {
  UrlRequestParamsPtr first = UrlRequestParams_Create();
  UrlRequestParamsPtr second = UrlRequestParams_Create();

  // Copy values from |first| to |second|.
  UrlRequestParams_set_url(second, UrlRequestParams_get_url(first));
  EXPECT_STREQ(UrlRequestParams_get_url(first),
               UrlRequestParams_get_url(second));
  UrlRequestParams_set_httpMethod(second,
                                  UrlRequestParams_get_httpMethod(first));
  EXPECT_STREQ(UrlRequestParams_get_httpMethod(first),
               UrlRequestParams_get_httpMethod(second));
  // TODO(mef): Test array |requestHeaders|.
  UrlRequestParams_set_disableCache(second,
                                    UrlRequestParams_get_disableCache(first));
  EXPECT_EQ(UrlRequestParams_get_disableCache(first),
            UrlRequestParams_get_disableCache(second));
  UrlRequestParams_set_priority(second, UrlRequestParams_get_priority(first));
  EXPECT_EQ(UrlRequestParams_get_priority(first),
            UrlRequestParams_get_priority(second));
  UrlRequestParams_set_uploadDataProvider(
      second, UrlRequestParams_get_uploadDataProvider(first));
  EXPECT_EQ(UrlRequestParams_get_uploadDataProvider(first),
            UrlRequestParams_get_uploadDataProvider(second));
  UrlRequestParams_set_uploadDataProviderExecutor(
      second, UrlRequestParams_get_uploadDataProviderExecutor(first));
  EXPECT_EQ(UrlRequestParams_get_uploadDataProviderExecutor(first),
            UrlRequestParams_get_uploadDataProviderExecutor(second));
  UrlRequestParams_set_allowDirectExecutor(
      second, UrlRequestParams_get_allowDirectExecutor(first));
  EXPECT_EQ(UrlRequestParams_get_allowDirectExecutor(first),
            UrlRequestParams_get_allowDirectExecutor(second));
  // TODO(mef): Test array |annotations|.
  UrlRequestParams_Destroy(first);
  UrlRequestParams_Destroy(second);
}
