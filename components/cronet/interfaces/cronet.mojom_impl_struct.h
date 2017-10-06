// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/interfaces/cronet.mojom */

#ifndef COMPONENTS_CRONET_INTERFACES_CRONET_MOJOM_IMPL_STRUCT_H_
#define COMPONENTS_CRONET_INTERFACES_CRONET_MOJOM_IMPL_STRUCT_H_

#include "components/cronet/interfaces/cronet.mojom_c.h"

#include <string>
#include <vector>

// Struct Annotation.
struct Annotation {
  // Annotation members.
};

// Struct RawBuffer.
struct RawBuffer {
  // RawBuffer members.
  std::vector<int8_t> data;
};

// Struct Buffer.
struct Buffer {
  // Buffer members.
  int32_t size;
  int32_t limit;
  int32_t position;
  std::unique_ptr<RawBuffer> data;
  BufferCallbackPtr callback;
};

// Struct QuicHint.
struct QuicHint {
  // QuicHint members.
  std::string host;
  int32_t port;
  int32_t alternatePort;
};

// Struct PublicKeyPins.
struct PublicKeyPins {
  // PublicKeyPins members.
  std::string host;
  std::vector<std::unique_ptr<RawBuffer>> pinsSha256;
  bool includeSubdomains;
};

// Struct CronetEngineParams.
struct CronetEngineParams {
  // CronetEngineParams members.
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
};

// Struct HttpHeader.
struct HttpHeader {
  // HttpHeader members.
  std::string name;
  std::string value;
};

// Struct UrlResponseInfo.
struct UrlResponseInfo {
  // UrlResponseInfo members.
  std::string url;
  std::vector<std::string> urlChain;
  int32_t httpStatusCode;
  std::string httpStatusText;
  std::vector<std::unique_ptr<HttpHeader>> allHeadersList;
  bool wasCached;
  std::string negotiatedProtocol;
  std::string proxyServer;
  int64_t receivedByteCount;
};

// Struct UrlRequestParams.
struct UrlRequestParams {
  // UrlRequestParams members.
  std::string url;
  std::string httpMethod;
  std::vector<std::unique_ptr<HttpHeader>> requestHeaders;
  bool disableCache;
  UrlRequestParams_REQUEST_PRIORITY priority;
  UploadDataProviderPtr uploadDataProvider;
  ExecutorPtr uploadDataProviderExecutor;
  bool allowDirectExecutor;
  std::unique_ptr<Annotation> annotation;
};

#endif  // COMPONENTS_CRONET_INTERFACES_CRONET_MOJOM_IMPL_STRUCT_H_