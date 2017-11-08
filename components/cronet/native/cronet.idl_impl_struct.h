// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/cronet.idl */

#ifndef COMPONENTS_CRONET_NATIVE_CRONET_IDL_IMPL_STRUCT_H_
#define COMPONENTS_CRONET_NATIVE_CRONET_IDL_IMPL_STRUCT_H_

#include "components/cronet/native/cronet.idl_c.h"

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"

// Struct Annotation.
struct Annotation {
 public:
  Annotation();
  ~Annotation();

  uint64_t value = 0ULL;

 private:
  DISALLOW_COPY_AND_ASSIGN(Annotation);
};

// Struct RawBuffer.
struct RawBuffer {
 public:
  RawBuffer();
  ~RawBuffer();

  std::vector<int8_t> data;

 private:
  DISALLOW_COPY_AND_ASSIGN(RawBuffer);
};

// Struct Buffer.
struct Buffer {
 public:
  Buffer();
  ~Buffer();

  uint32_t size = 0U;
  uint32_t limit = 0U;
  uint32_t position = 0U;
  std::unique_ptr<RawBuffer> data;
  BufferCallbackPtr callback;

 private:
  DISALLOW_COPY_AND_ASSIGN(Buffer);
};

// Struct CronetException.
struct CronetException {
 public:
  CronetException();
  ~CronetException();

  CronetException_ERROR_CODE error_code =
      CronetException_ERROR_CODE_ERROR_CALLBACK;
  int32_t cronet_internal_error_code = 0;
  bool immediately_retryable = false;
  int32_t quic_detailed_error_code = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CronetException);
};

// Struct QuicHint.
struct QuicHint {
 public:
  QuicHint();
  ~QuicHint();

  std::string host;
  int32_t port = 0;
  int32_t alternatePort = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(QuicHint);
};

// Struct PublicKeyPins.
struct PublicKeyPins {
 public:
  PublicKeyPins();
  ~PublicKeyPins();

  std::string host;
  std::vector<std::unique_ptr<RawBuffer>> pinsSha256;
  bool includeSubdomains = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(PublicKeyPins);
};

// Struct CronetEngineParams.
struct CronetEngineParams {
 public:
  CronetEngineParams();
  ~CronetEngineParams();

  std::string userAgent;
  std::string storagePath;
  bool enableQuic = false;
  bool enableHttp2 = true;
  bool enableBrotli = true;
  CronetEngineParams_HTTP_CACHE_MODE httpCacheMode =
      CronetEngineParams_HTTP_CACHE_MODE_DISABLED;
  int64_t httpCacheMaxSize = 0;
  std::vector<std::unique_ptr<QuicHint>> quicHints;
  std::vector<std::unique_ptr<PublicKeyPins>> publicKeyPins;
  bool enablePublicKeyPinningBypassForLocalTrustAnchors = true;

 private:
  DISALLOW_COPY_AND_ASSIGN(CronetEngineParams);
};

// Struct HttpHeader.
struct HttpHeader {
 public:
  HttpHeader();
  ~HttpHeader();

  std::string name;
  std::string value;

 private:
  DISALLOW_COPY_AND_ASSIGN(HttpHeader);
};

// Struct UrlResponseInfo.
struct UrlResponseInfo {
 public:
  UrlResponseInfo();
  ~UrlResponseInfo();

  std::string url;
  std::vector<std::string> urlChain;
  int32_t httpStatusCode;
  std::string httpStatusText;
  std::vector<std::unique_ptr<HttpHeader>> allHeadersList;
  bool wasCached;
  std::string negotiatedProtocol;
  std::string proxyServer;
  int64_t receivedByteCount;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlResponseInfo);
};

// Struct UrlRequestParams.
struct UrlRequestParams {
 public:
  UrlRequestParams();
  ~UrlRequestParams();

  std::string url;
  std::string httpMethod;
  std::vector<std::unique_ptr<HttpHeader>> requestHeaders;
  bool disableCache = false;
  UrlRequestParams_REQUEST_PRIORITY priority =
      UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_MEDIUM;
  UploadDataProviderPtr uploadDataProvider;
  ExecutorPtr uploadDataProviderExecutor;
  bool allowDirectExecutor = false;
  std::vector<std::unique_ptr<Annotation>> annotations;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlRequestParams);
};

#endif  // COMPONENTS_CRONET_NATIVE_CRONET_IDL_IMPL_STRUCT_H_
