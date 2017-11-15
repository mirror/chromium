// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/generated/cronet.idl */

#include "components/cronet/native/generated/cronet.idl_c.h"

#include "base/logging.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

class CrStructTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  CrStructTest() {}
  ~CrStructTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(CrStructTest);
};

// Test Struct cr_Buffer setters and getters.
TEST_F(CrStructTest, Testcr_Buffer) {
  cr_BufferPtr first = cr_Buffer_Create();
  cr_BufferPtr second = cr_Buffer_Create();

  // Copy values from |first| to |second|.
  cr_Buffer_set_size(second, cr_Buffer_get_size(first));
  EXPECT_EQ(cr_Buffer_get_size(first), cr_Buffer_get_size(second));
  cr_Buffer_set_limit(second, cr_Buffer_get_limit(first));
  EXPECT_EQ(cr_Buffer_get_limit(first), cr_Buffer_get_limit(second));
  cr_Buffer_set_position(second, cr_Buffer_get_position(first));
  EXPECT_EQ(cr_Buffer_get_position(first), cr_Buffer_get_position(second));
  cr_Buffer_set_data(second, cr_Buffer_get_data(first));
  EXPECT_EQ(cr_Buffer_get_data(first), cr_Buffer_get_data(second));
  cr_Buffer_set_callback(second, cr_Buffer_get_callback(first));
  EXPECT_EQ(cr_Buffer_get_callback(first), cr_Buffer_get_callback(second));
  cr_Buffer_Destroy(first);
  cr_Buffer_Destroy(second);
}

// Test Struct cr_CronetException setters and getters.
TEST_F(CrStructTest, Testcr_CronetException) {
  cr_CronetExceptionPtr first = cr_CronetException_Create();
  cr_CronetExceptionPtr second = cr_CronetException_Create();

  // Copy values from |first| to |second|.
  cr_CronetException_set_error_code(second,
                                    cr_CronetException_get_error_code(first));
  EXPECT_EQ(cr_CronetException_get_error_code(first),
            cr_CronetException_get_error_code(second));
  cr_CronetException_set_cronet_internal_error_code(
      second, cr_CronetException_get_cronet_internal_error_code(first));
  EXPECT_EQ(cr_CronetException_get_cronet_internal_error_code(first),
            cr_CronetException_get_cronet_internal_error_code(second));
  cr_CronetException_set_immediately_retryable(
      second, cr_CronetException_get_immediately_retryable(first));
  EXPECT_EQ(cr_CronetException_get_immediately_retryable(first),
            cr_CronetException_get_immediately_retryable(second));
  cr_CronetException_set_quic_detailed_error_code(
      second, cr_CronetException_get_quic_detailed_error_code(first));
  EXPECT_EQ(cr_CronetException_get_quic_detailed_error_code(first),
            cr_CronetException_get_quic_detailed_error_code(second));
  cr_CronetException_Destroy(first);
  cr_CronetException_Destroy(second);
}

// Test Struct cr_QuicHint setters and getters.
TEST_F(CrStructTest, Testcr_QuicHint) {
  cr_QuicHintPtr first = cr_QuicHint_Create();
  cr_QuicHintPtr second = cr_QuicHint_Create();

  // Copy values from |first| to |second|.
  cr_QuicHint_set_host(second, cr_QuicHint_get_host(first));
  EXPECT_STREQ(cr_QuicHint_get_host(first), cr_QuicHint_get_host(second));
  cr_QuicHint_set_port(second, cr_QuicHint_get_port(first));
  EXPECT_EQ(cr_QuicHint_get_port(first), cr_QuicHint_get_port(second));
  cr_QuicHint_set_alternatePort(second, cr_QuicHint_get_alternatePort(first));
  EXPECT_EQ(cr_QuicHint_get_alternatePort(first),
            cr_QuicHint_get_alternatePort(second));
  cr_QuicHint_Destroy(first);
  cr_QuicHint_Destroy(second);
}

// Test Struct cr_PublicKeyPins setters and getters.
TEST_F(CrStructTest, Testcr_PublicKeyPins) {
  cr_PublicKeyPinsPtr first = cr_PublicKeyPins_Create();
  cr_PublicKeyPinsPtr second = cr_PublicKeyPins_Create();

  // Copy values from |first| to |second|.
  cr_PublicKeyPins_set_host(second, cr_PublicKeyPins_get_host(first));
  EXPECT_STREQ(cr_PublicKeyPins_get_host(first),
               cr_PublicKeyPins_get_host(second));
  // TODO(mef): Test array |pinsSha256|.
  cr_PublicKeyPins_set_includeSubdomains(
      second, cr_PublicKeyPins_get_includeSubdomains(first));
  EXPECT_EQ(cr_PublicKeyPins_get_includeSubdomains(first),
            cr_PublicKeyPins_get_includeSubdomains(second));
  cr_PublicKeyPins_Destroy(first);
  cr_PublicKeyPins_Destroy(second);
}

// Test Struct cr_CronetEngineParams setters and getters.
TEST_F(CrStructTest, Testcr_CronetEngineParams) {
  cr_CronetEngineParamsPtr first = cr_CronetEngineParams_Create();
  cr_CronetEngineParamsPtr second = cr_CronetEngineParams_Create();

  // Copy values from |first| to |second|.
  cr_CronetEngineParams_set_userAgent(
      second, cr_CronetEngineParams_get_userAgent(first));
  EXPECT_STREQ(cr_CronetEngineParams_get_userAgent(first),
               cr_CronetEngineParams_get_userAgent(second));
  cr_CronetEngineParams_set_storagePath(
      second, cr_CronetEngineParams_get_storagePath(first));
  EXPECT_STREQ(cr_CronetEngineParams_get_storagePath(first),
               cr_CronetEngineParams_get_storagePath(second));
  cr_CronetEngineParams_set_enableQuic(
      second, cr_CronetEngineParams_get_enableQuic(first));
  EXPECT_EQ(cr_CronetEngineParams_get_enableQuic(first),
            cr_CronetEngineParams_get_enableQuic(second));
  cr_CronetEngineParams_set_enableHttp2(
      second, cr_CronetEngineParams_get_enableHttp2(first));
  EXPECT_EQ(cr_CronetEngineParams_get_enableHttp2(first),
            cr_CronetEngineParams_get_enableHttp2(second));
  cr_CronetEngineParams_set_enableBrotli(
      second, cr_CronetEngineParams_get_enableBrotli(first));
  EXPECT_EQ(cr_CronetEngineParams_get_enableBrotli(first),
            cr_CronetEngineParams_get_enableBrotli(second));
  cr_CronetEngineParams_set_httpCacheMode(
      second, cr_CronetEngineParams_get_httpCacheMode(first));
  EXPECT_EQ(cr_CronetEngineParams_get_httpCacheMode(first),
            cr_CronetEngineParams_get_httpCacheMode(second));
  cr_CronetEngineParams_set_httpCacheMaxSize(
      second, cr_CronetEngineParams_get_httpCacheMaxSize(first));
  EXPECT_EQ(cr_CronetEngineParams_get_httpCacheMaxSize(first),
            cr_CronetEngineParams_get_httpCacheMaxSize(second));
  // TODO(mef): Test array |quicHints|.
  // TODO(mef): Test array |publicKeyPins|.
  cr_CronetEngineParams_set_enablePublicKeyPinningBypassForLocalTrustAnchors(
      second,
      cr_CronetEngineParams_get_enablePublicKeyPinningBypassForLocalTrustAnchors(
          first));
  EXPECT_EQ(
      cr_CronetEngineParams_get_enablePublicKeyPinningBypassForLocalTrustAnchors(
          first),
      cr_CronetEngineParams_get_enablePublicKeyPinningBypassForLocalTrustAnchors(
          second));
  cr_CronetEngineParams_Destroy(first);
  cr_CronetEngineParams_Destroy(second);
}

// Test Struct cr_HttpHeader setters and getters.
TEST_F(CrStructTest, Testcr_HttpHeader) {
  cr_HttpHeaderPtr first = cr_HttpHeader_Create();
  cr_HttpHeaderPtr second = cr_HttpHeader_Create();

  // Copy values from |first| to |second|.
  cr_HttpHeader_set_name(second, cr_HttpHeader_get_name(first));
  EXPECT_STREQ(cr_HttpHeader_get_name(first), cr_HttpHeader_get_name(second));
  cr_HttpHeader_set_value(second, cr_HttpHeader_get_value(first));
  EXPECT_STREQ(cr_HttpHeader_get_value(first), cr_HttpHeader_get_value(second));
  cr_HttpHeader_Destroy(first);
  cr_HttpHeader_Destroy(second);
}

// Test Struct cr_UrlResponseInfo setters and getters.
TEST_F(CrStructTest, Testcr_UrlResponseInfo) {
  cr_UrlResponseInfoPtr first = cr_UrlResponseInfo_Create();
  cr_UrlResponseInfoPtr second = cr_UrlResponseInfo_Create();

  // Copy values from |first| to |second|.
  cr_UrlResponseInfo_set_url(second, cr_UrlResponseInfo_get_url(first));
  EXPECT_STREQ(cr_UrlResponseInfo_get_url(first),
               cr_UrlResponseInfo_get_url(second));
  // TODO(mef): Test array |urlChain|.
  cr_UrlResponseInfo_set_httpStatusCode(
      second, cr_UrlResponseInfo_get_httpStatusCode(first));
  EXPECT_EQ(cr_UrlResponseInfo_get_httpStatusCode(first),
            cr_UrlResponseInfo_get_httpStatusCode(second));
  cr_UrlResponseInfo_set_httpStatusText(
      second, cr_UrlResponseInfo_get_httpStatusText(first));
  EXPECT_STREQ(cr_UrlResponseInfo_get_httpStatusText(first),
               cr_UrlResponseInfo_get_httpStatusText(second));
  // TODO(mef): Test array |allHeadersList|.
  cr_UrlResponseInfo_set_wasCached(second,
                                   cr_UrlResponseInfo_get_wasCached(first));
  EXPECT_EQ(cr_UrlResponseInfo_get_wasCached(first),
            cr_UrlResponseInfo_get_wasCached(second));
  cr_UrlResponseInfo_set_negotiatedProtocol(
      second, cr_UrlResponseInfo_get_negotiatedProtocol(first));
  EXPECT_STREQ(cr_UrlResponseInfo_get_negotiatedProtocol(first),
               cr_UrlResponseInfo_get_negotiatedProtocol(second));
  cr_UrlResponseInfo_set_proxyServer(second,
                                     cr_UrlResponseInfo_get_proxyServer(first));
  EXPECT_STREQ(cr_UrlResponseInfo_get_proxyServer(first),
               cr_UrlResponseInfo_get_proxyServer(second));
  cr_UrlResponseInfo_set_receivedByteCount(
      second, cr_UrlResponseInfo_get_receivedByteCount(first));
  EXPECT_EQ(cr_UrlResponseInfo_get_receivedByteCount(first),
            cr_UrlResponseInfo_get_receivedByteCount(second));
  cr_UrlResponseInfo_Destroy(first);
  cr_UrlResponseInfo_Destroy(second);
}

// Test Struct cr_UrlRequestParams setters and getters.
TEST_F(CrStructTest, Testcr_UrlRequestParams) {
  cr_UrlRequestParamsPtr first = cr_UrlRequestParams_Create();
  cr_UrlRequestParamsPtr second = cr_UrlRequestParams_Create();

  // Copy values from |first| to |second|.
  cr_UrlRequestParams_set_httpMethod(second,
                                     cr_UrlRequestParams_get_httpMethod(first));
  EXPECT_STREQ(cr_UrlRequestParams_get_httpMethod(first),
               cr_UrlRequestParams_get_httpMethod(second));
  // TODO(mef): Test array |requestHeaders|.
  cr_UrlRequestParams_set_disableCache(
      second, cr_UrlRequestParams_get_disableCache(first));
  EXPECT_EQ(cr_UrlRequestParams_get_disableCache(first),
            cr_UrlRequestParams_get_disableCache(second));
  cr_UrlRequestParams_set_priority(second,
                                   cr_UrlRequestParams_get_priority(first));
  EXPECT_EQ(cr_UrlRequestParams_get_priority(first),
            cr_UrlRequestParams_get_priority(second));
  cr_UrlRequestParams_set_uploadDataProvider(
      second, cr_UrlRequestParams_get_uploadDataProvider(first));
  EXPECT_EQ(cr_UrlRequestParams_get_uploadDataProvider(first),
            cr_UrlRequestParams_get_uploadDataProvider(second));
  cr_UrlRequestParams_set_uploadDataProviderExecutor(
      second, cr_UrlRequestParams_get_uploadDataProviderExecutor(first));
  EXPECT_EQ(cr_UrlRequestParams_get_uploadDataProviderExecutor(first),
            cr_UrlRequestParams_get_uploadDataProviderExecutor(second));
  cr_UrlRequestParams_set_allowDirectExecutor(
      second, cr_UrlRequestParams_get_allowDirectExecutor(first));
  EXPECT_EQ(cr_UrlRequestParams_get_allowDirectExecutor(first),
            cr_UrlRequestParams_get_allowDirectExecutor(second));
  // TODO(mef): Test array |annotations|.
  cr_UrlRequestParams_Destroy(first);
  cr_UrlRequestParams_Destroy(second);
}
