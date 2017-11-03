// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/cronet.mojom */

#ifndef COMPONENTS_CRONET_NATIVE_CRONET_MOJOM_IMPL_STRUCT_H_
#define COMPONENTS_CRONET_NATIVE_CRONET_MOJOM_IMPL_STRUCT_H_

#include "components/cronet/native/cronet.mojom_c.h"

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"

// Struct Annotation.
struct Annotation {
 public:
  Annotation();
  ~Annotation();

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

  int32_t size;
  int32_t limit;
  int32_t position;
  std::unique_ptr<RawBuffer> data;
  BufferCallbackPtr callback;

 private:
  DISALLOW_COPY_AND_ASSIGN(Buffer);
};

// Struct QuicHint.
struct QuicHint {
 public:
  QuicHint();
  ~QuicHint();

  std::string host;
  int32_t port;
  int32_t alternatePort;

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
  bool includeSubdomains;

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
  bool enableQuic;
  bool enableHttp2;
  bool enableBrotli;
  CronetEngineParams_HTTP_CACHE_MODE httpCacheMode;
  int32_t httpCacheMaxSize;
  std::vector<std::unique_ptr<QuicHint>> quicHints;
  std::vector<std::unique_ptr<PublicKeyPins>> publicKeyPins;
  bool enablePublicKeyPinningBypassForLocalTrustAnchors;

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
  bool disableCache;
  UrlRequestParams_REQUEST_PRIORITY priority;
  UploadDataProviderPtr uploadDataProvider;
  ExecutorPtr uploadDataProviderExecutor;
  bool allowDirectExecutor;
  std::unique_ptr<Annotation> annotation;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlRequestParams);
};

#endif  // COMPONENTS_CRONET_NATIVE_CRONET_MOJOM_IMPL_STRUCT_H_
