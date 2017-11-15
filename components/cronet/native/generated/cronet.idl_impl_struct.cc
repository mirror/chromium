// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/generated/cronet.idl */

#include "components/cronet/native/generated/cronet.idl_impl_struct.h"

#include <utility>

#include "base/logging.h"

// Struct cr_Buffer.
cr_Buffer::cr_Buffer() {}

cr_Buffer::~cr_Buffer() {}

cr_BufferPtr cr_Buffer_Create() {
  return new cr_Buffer();
}

void cr_Buffer_Destroy(cr_BufferPtr self) {
  delete self;
}

// Struct cr_Buffer setters.
void cr_Buffer_set_size(cr_BufferPtr self, uint32_t size) {
  DCHECK(self);
  self->size = size;
}

void cr_Buffer_set_limit(cr_BufferPtr self, uint32_t limit) {
  DCHECK(self);
  self->limit = limit;
}

void cr_Buffer_set_position(cr_BufferPtr self, uint32_t position) {
  DCHECK(self);
  self->position = position;
}

void cr_Buffer_set_data(cr_BufferPtr self, RawDataPtr data) {
  DCHECK(self);
  self->data = data;
}

void cr_Buffer_set_callback(cr_BufferPtr self, cr_BufferCallbackPtr callback) {
  DCHECK(self);
  self->callback = callback;
}

// Struct cr_Buffer getters.
uint32_t cr_Buffer_get_size(cr_BufferPtr self) {
  DCHECK(self);
  return self->size;
}

uint32_t cr_Buffer_get_limit(cr_BufferPtr self) {
  DCHECK(self);
  return self->limit;
}

uint32_t cr_Buffer_get_position(cr_BufferPtr self) {
  DCHECK(self);
  return self->position;
}

RawDataPtr cr_Buffer_get_data(cr_BufferPtr self) {
  DCHECK(self);
  return self->data;
}

cr_BufferCallbackPtr cr_Buffer_get_callback(cr_BufferPtr self) {
  DCHECK(self);
  return self->callback;
}

// Struct cr_CronetException.
cr_CronetException::cr_CronetException() {}

cr_CronetException::~cr_CronetException() {}

cr_CronetExceptionPtr cr_CronetException_Create() {
  return new cr_CronetException();
}

void cr_CronetException_Destroy(cr_CronetExceptionPtr self) {
  delete self;
}

// Struct cr_CronetException setters.
void cr_CronetException_set_error_code(
    cr_CronetExceptionPtr self,
    cr_CronetException_ERROR_CODE error_code) {
  DCHECK(self);
  self->error_code = error_code;
}

void cr_CronetException_set_cronet_internal_error_code(
    cr_CronetExceptionPtr self,
    int32_t cronet_internal_error_code) {
  DCHECK(self);
  self->cronet_internal_error_code = cronet_internal_error_code;
}

void cr_CronetException_set_immediately_retryable(cr_CronetExceptionPtr self,
                                                  bool immediately_retryable) {
  DCHECK(self);
  self->immediately_retryable = immediately_retryable;
}

void cr_CronetException_set_quic_detailed_error_code(
    cr_CronetExceptionPtr self,
    int32_t quic_detailed_error_code) {
  DCHECK(self);
  self->quic_detailed_error_code = quic_detailed_error_code;
}

// Struct cr_CronetException getters.
cr_CronetException_ERROR_CODE cr_CronetException_get_error_code(
    cr_CronetExceptionPtr self) {
  DCHECK(self);
  return self->error_code;
}

int32_t cr_CronetException_get_cronet_internal_error_code(
    cr_CronetExceptionPtr self) {
  DCHECK(self);
  return self->cronet_internal_error_code;
}

bool cr_CronetException_get_immediately_retryable(cr_CronetExceptionPtr self) {
  DCHECK(self);
  return self->immediately_retryable;
}

int32_t cr_CronetException_get_quic_detailed_error_code(
    cr_CronetExceptionPtr self) {
  DCHECK(self);
  return self->quic_detailed_error_code;
}

// Struct cr_QuicHint.
cr_QuicHint::cr_QuicHint() {}

cr_QuicHint::~cr_QuicHint() {}

cr_QuicHintPtr cr_QuicHint_Create() {
  return new cr_QuicHint();
}

void cr_QuicHint_Destroy(cr_QuicHintPtr self) {
  delete self;
}

// Struct cr_QuicHint setters.
void cr_QuicHint_set_host(cr_QuicHintPtr self, CharString host) {
  DCHECK(self);
  self->host = host;
}

void cr_QuicHint_set_port(cr_QuicHintPtr self, int32_t port) {
  DCHECK(self);
  self->port = port;
}

void cr_QuicHint_set_alternatePort(cr_QuicHintPtr self, int32_t alternatePort) {
  DCHECK(self);
  self->alternatePort = alternatePort;
}

// Struct cr_QuicHint getters.
CharString cr_QuicHint_get_host(cr_QuicHintPtr self) {
  DCHECK(self);
  return self->host.c_str();
}

int32_t cr_QuicHint_get_port(cr_QuicHintPtr self) {
  DCHECK(self);
  return self->port;
}

int32_t cr_QuicHint_get_alternatePort(cr_QuicHintPtr self) {
  DCHECK(self);
  return self->alternatePort;
}

// Struct cr_PublicKeyPins.
cr_PublicKeyPins::cr_PublicKeyPins() {}

cr_PublicKeyPins::~cr_PublicKeyPins() {}

cr_PublicKeyPinsPtr cr_PublicKeyPins_Create() {
  return new cr_PublicKeyPins();
}

void cr_PublicKeyPins_Destroy(cr_PublicKeyPinsPtr self) {
  delete self;
}

// Struct cr_PublicKeyPins setters.
void cr_PublicKeyPins_set_host(cr_PublicKeyPinsPtr self, CharString host) {
  DCHECK(self);
  self->host = host;
}

void cr_PublicKeyPins_add_pinsSha256(cr_PublicKeyPinsPtr self,
                                     RawDataPtr pinsSha256) {
  DCHECK(self);
  self->pinsSha256.push_back(pinsSha256);
}

void cr_PublicKeyPins_set_includeSubdomains(cr_PublicKeyPinsPtr self,
                                            bool includeSubdomains) {
  DCHECK(self);
  self->includeSubdomains = includeSubdomains;
}

// Struct cr_PublicKeyPins getters.
CharString cr_PublicKeyPins_get_host(cr_PublicKeyPinsPtr self) {
  DCHECK(self);
  return self->host.c_str();
}

uint32_t cr_PublicKeyPins_get_pinsSha256Size(cr_PublicKeyPinsPtr self) {
  DCHECK(self);
  return self->pinsSha256.size();
}
RawDataPtr cr_PublicKeyPins_get_pinsSha256AtIndex(cr_PublicKeyPinsPtr self,
                                                  uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->pinsSha256.size());
  return self->pinsSha256[index];
}

bool cr_PublicKeyPins_get_includeSubdomains(cr_PublicKeyPinsPtr self) {
  DCHECK(self);
  return self->includeSubdomains;
}

// Struct cr_CronetEngineParams.
cr_CronetEngineParams::cr_CronetEngineParams() {}

cr_CronetEngineParams::~cr_CronetEngineParams() {}

cr_CronetEngineParamsPtr cr_CronetEngineParams_Create() {
  return new cr_CronetEngineParams();
}

void cr_CronetEngineParams_Destroy(cr_CronetEngineParamsPtr self) {
  delete self;
}

// Struct cr_CronetEngineParams setters.
void cr_CronetEngineParams_set_userAgent(cr_CronetEngineParamsPtr self,
                                         CharString userAgent) {
  DCHECK(self);
  self->userAgent = userAgent;
}

void cr_CronetEngineParams_set_storagePath(cr_CronetEngineParamsPtr self,
                                           CharString storagePath) {
  DCHECK(self);
  self->storagePath = storagePath;
}

void cr_CronetEngineParams_set_enableQuic(cr_CronetEngineParamsPtr self,
                                          bool enableQuic) {
  DCHECK(self);
  self->enableQuic = enableQuic;
}

void cr_CronetEngineParams_set_enableHttp2(cr_CronetEngineParamsPtr self,
                                           bool enableHttp2) {
  DCHECK(self);
  self->enableHttp2 = enableHttp2;
}

void cr_CronetEngineParams_set_enableBrotli(cr_CronetEngineParamsPtr self,
                                            bool enableBrotli) {
  DCHECK(self);
  self->enableBrotli = enableBrotli;
}

void cr_CronetEngineParams_set_httpCacheMode(
    cr_CronetEngineParamsPtr self,
    cr_CronetEngineParams_HTTP_CACHE_MODE httpCacheMode) {
  DCHECK(self);
  self->httpCacheMode = httpCacheMode;
}

void cr_CronetEngineParams_set_httpCacheMaxSize(cr_CronetEngineParamsPtr self,
                                                int64_t httpCacheMaxSize) {
  DCHECK(self);
  self->httpCacheMaxSize = httpCacheMaxSize;
}

void cr_CronetEngineParams_add_quicHints(cr_CronetEngineParamsPtr self,
                                         cr_QuicHintPtr quicHints) {
  DCHECK(self);
  std::unique_ptr<cr_QuicHint> tmp_ptr(quicHints);
  self->quicHints.push_back(std::move(tmp_ptr));
}

void cr_CronetEngineParams_add_publicKeyPins(
    cr_CronetEngineParamsPtr self,
    cr_PublicKeyPinsPtr publicKeyPins) {
  DCHECK(self);
  std::unique_ptr<cr_PublicKeyPins> tmp_ptr(publicKeyPins);
  self->publicKeyPins.push_back(std::move(tmp_ptr));
}

void cr_CronetEngineParams_set_enablePublicKeyPinningBypassForLocalTrustAnchors(
    cr_CronetEngineParamsPtr self,
    bool enablePublicKeyPinningBypassForLocalTrustAnchors) {
  DCHECK(self);
  self->enablePublicKeyPinningBypassForLocalTrustAnchors =
      enablePublicKeyPinningBypassForLocalTrustAnchors;
}

// Struct cr_CronetEngineParams getters.
CharString cr_CronetEngineParams_get_userAgent(cr_CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->userAgent.c_str();
}

CharString cr_CronetEngineParams_get_storagePath(
    cr_CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->storagePath.c_str();
}

bool cr_CronetEngineParams_get_enableQuic(cr_CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->enableQuic;
}

bool cr_CronetEngineParams_get_enableHttp2(cr_CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->enableHttp2;
}

bool cr_CronetEngineParams_get_enableBrotli(cr_CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->enableBrotli;
}

cr_CronetEngineParams_HTTP_CACHE_MODE cr_CronetEngineParams_get_httpCacheMode(
    cr_CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->httpCacheMode;
}

int64_t cr_CronetEngineParams_get_httpCacheMaxSize(
    cr_CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->httpCacheMaxSize;
}

uint32_t cr_CronetEngineParams_get_quicHintsSize(
    cr_CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->quicHints.size();
}
cr_QuicHintPtr cr_CronetEngineParams_get_quicHintsAtIndex(
    cr_CronetEngineParamsPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->quicHints.size());
  return self->quicHints[index].get();
}

uint32_t cr_CronetEngineParams_get_publicKeyPinsSize(
    cr_CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->publicKeyPins.size();
}
cr_PublicKeyPinsPtr cr_CronetEngineParams_get_publicKeyPinsAtIndex(
    cr_CronetEngineParamsPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->publicKeyPins.size());
  return self->publicKeyPins[index].get();
}

bool cr_CronetEngineParams_get_enablePublicKeyPinningBypassForLocalTrustAnchors(
    cr_CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->enablePublicKeyPinningBypassForLocalTrustAnchors;
}

// Struct cr_HttpHeader.
cr_HttpHeader::cr_HttpHeader() {}

cr_HttpHeader::~cr_HttpHeader() {}

cr_HttpHeaderPtr cr_HttpHeader_Create() {
  return new cr_HttpHeader();
}

void cr_HttpHeader_Destroy(cr_HttpHeaderPtr self) {
  delete self;
}

// Struct cr_HttpHeader setters.
void cr_HttpHeader_set_name(cr_HttpHeaderPtr self, CharString name) {
  DCHECK(self);
  self->name = name;
}

void cr_HttpHeader_set_value(cr_HttpHeaderPtr self, CharString value) {
  DCHECK(self);
  self->value = value;
}

// Struct cr_HttpHeader getters.
CharString cr_HttpHeader_get_name(cr_HttpHeaderPtr self) {
  DCHECK(self);
  return self->name.c_str();
}

CharString cr_HttpHeader_get_value(cr_HttpHeaderPtr self) {
  DCHECK(self);
  return self->value.c_str();
}

// Struct cr_UrlResponseInfo.
cr_UrlResponseInfo::cr_UrlResponseInfo() {}

cr_UrlResponseInfo::~cr_UrlResponseInfo() {}

cr_UrlResponseInfoPtr cr_UrlResponseInfo_Create() {
  return new cr_UrlResponseInfo();
}

void cr_UrlResponseInfo_Destroy(cr_UrlResponseInfoPtr self) {
  delete self;
}

// Struct cr_UrlResponseInfo setters.
void cr_UrlResponseInfo_set_url(cr_UrlResponseInfoPtr self, CharString url) {
  DCHECK(self);
  self->url = url;
}

void cr_UrlResponseInfo_add_urlChain(cr_UrlResponseInfoPtr self,
                                     CharString urlChain) {
  DCHECK(self);
  self->urlChain.push_back(urlChain);
}

void cr_UrlResponseInfo_set_httpStatusCode(cr_UrlResponseInfoPtr self,
                                           int32_t httpStatusCode) {
  DCHECK(self);
  self->httpStatusCode = httpStatusCode;
}

void cr_UrlResponseInfo_set_httpStatusText(cr_UrlResponseInfoPtr self,
                                           CharString httpStatusText) {
  DCHECK(self);
  self->httpStatusText = httpStatusText;
}

void cr_UrlResponseInfo_add_allHeadersList(cr_UrlResponseInfoPtr self,
                                           cr_HttpHeaderPtr allHeadersList) {
  DCHECK(self);
  std::unique_ptr<cr_HttpHeader> tmp_ptr(allHeadersList);
  self->allHeadersList.push_back(std::move(tmp_ptr));
}

void cr_UrlResponseInfo_set_wasCached(cr_UrlResponseInfoPtr self,
                                      bool wasCached) {
  DCHECK(self);
  self->wasCached = wasCached;
}

void cr_UrlResponseInfo_set_negotiatedProtocol(cr_UrlResponseInfoPtr self,
                                               CharString negotiatedProtocol) {
  DCHECK(self);
  self->negotiatedProtocol = negotiatedProtocol;
}

void cr_UrlResponseInfo_set_proxyServer(cr_UrlResponseInfoPtr self,
                                        CharString proxyServer) {
  DCHECK(self);
  self->proxyServer = proxyServer;
}

void cr_UrlResponseInfo_set_receivedByteCount(cr_UrlResponseInfoPtr self,
                                              int64_t receivedByteCount) {
  DCHECK(self);
  self->receivedByteCount = receivedByteCount;
}

// Struct cr_UrlResponseInfo getters.
CharString cr_UrlResponseInfo_get_url(cr_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->url.c_str();
}

uint32_t cr_UrlResponseInfo_get_urlChainSize(cr_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->urlChain.size();
}
CharString cr_UrlResponseInfo_get_urlChainAtIndex(cr_UrlResponseInfoPtr self,
                                                  uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->urlChain.size());
  return self->urlChain[index].c_str();
}

int32_t cr_UrlResponseInfo_get_httpStatusCode(cr_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->httpStatusCode;
}

CharString cr_UrlResponseInfo_get_httpStatusText(cr_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->httpStatusText.c_str();
}

uint32_t cr_UrlResponseInfo_get_allHeadersListSize(cr_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->allHeadersList.size();
}
cr_HttpHeaderPtr cr_UrlResponseInfo_get_allHeadersListAtIndex(
    cr_UrlResponseInfoPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->allHeadersList.size());
  return self->allHeadersList[index].get();
}

bool cr_UrlResponseInfo_get_wasCached(cr_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->wasCached;
}

CharString cr_UrlResponseInfo_get_negotiatedProtocol(
    cr_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->negotiatedProtocol.c_str();
}

CharString cr_UrlResponseInfo_get_proxyServer(cr_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->proxyServer.c_str();
}

int64_t cr_UrlResponseInfo_get_receivedByteCount(cr_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->receivedByteCount;
}

// Struct cr_UrlRequestParams.
cr_UrlRequestParams::cr_UrlRequestParams() {}

cr_UrlRequestParams::~cr_UrlRequestParams() {}

cr_UrlRequestParamsPtr cr_UrlRequestParams_Create() {
  return new cr_UrlRequestParams();
}

void cr_UrlRequestParams_Destroy(cr_UrlRequestParamsPtr self) {
  delete self;
}

// Struct cr_UrlRequestParams setters.
void cr_UrlRequestParams_set_httpMethod(cr_UrlRequestParamsPtr self,
                                        CharString httpMethod) {
  DCHECK(self);
  self->httpMethod = httpMethod;
}

void cr_UrlRequestParams_add_requestHeaders(cr_UrlRequestParamsPtr self,
                                            cr_HttpHeaderPtr requestHeaders) {
  DCHECK(self);
  std::unique_ptr<cr_HttpHeader> tmp_ptr(requestHeaders);
  self->requestHeaders.push_back(std::move(tmp_ptr));
}

void cr_UrlRequestParams_set_disableCache(cr_UrlRequestParamsPtr self,
                                          bool disableCache) {
  DCHECK(self);
  self->disableCache = disableCache;
}

void cr_UrlRequestParams_set_priority(
    cr_UrlRequestParamsPtr self,
    cr_UrlRequestParams_REQUEST_PRIORITY priority) {
  DCHECK(self);
  self->priority = priority;
}

void cr_UrlRequestParams_set_uploadDataProvider(
    cr_UrlRequestParamsPtr self,
    cr_UploadDataProviderPtr uploadDataProvider) {
  DCHECK(self);
  self->uploadDataProvider = uploadDataProvider;
}

void cr_UrlRequestParams_set_uploadDataProviderExecutor(
    cr_UrlRequestParamsPtr self,
    cr_ExecutorPtr uploadDataProviderExecutor) {
  DCHECK(self);
  self->uploadDataProviderExecutor = uploadDataProviderExecutor;
}

void cr_UrlRequestParams_set_allowDirectExecutor(cr_UrlRequestParamsPtr self,
                                                 bool allowDirectExecutor) {
  DCHECK(self);
  self->allowDirectExecutor = allowDirectExecutor;
}

void cr_UrlRequestParams_add_annotations(cr_UrlRequestParamsPtr self,
                                         RawDataPtr annotations) {
  DCHECK(self);
  self->annotations.push_back(annotations);
}

// Struct cr_UrlRequestParams getters.
CharString cr_UrlRequestParams_get_httpMethod(cr_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->httpMethod.c_str();
}

uint32_t cr_UrlRequestParams_get_requestHeadersSize(
    cr_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->requestHeaders.size();
}
cr_HttpHeaderPtr cr_UrlRequestParams_get_requestHeadersAtIndex(
    cr_UrlRequestParamsPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->requestHeaders.size());
  return self->requestHeaders[index].get();
}

bool cr_UrlRequestParams_get_disableCache(cr_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->disableCache;
}

cr_UrlRequestParams_REQUEST_PRIORITY cr_UrlRequestParams_get_priority(
    cr_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->priority;
}

cr_UploadDataProviderPtr cr_UrlRequestParams_get_uploadDataProvider(
    cr_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->uploadDataProvider;
}

cr_ExecutorPtr cr_UrlRequestParams_get_uploadDataProviderExecutor(
    cr_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->uploadDataProviderExecutor;
}

bool cr_UrlRequestParams_get_allowDirectExecutor(cr_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->allowDirectExecutor;
}

uint32_t cr_UrlRequestParams_get_annotationsSize(cr_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->annotations.size();
}
RawDataPtr cr_UrlRequestParams_get_annotationsAtIndex(
    cr_UrlRequestParamsPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->annotations.size());
  return self->annotations[index];
}
