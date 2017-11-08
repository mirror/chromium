// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/cronet.idl */

#include "components/cronet/native/cronet.idl_impl_struct.h"

#include <utility>

#include "base/logging.h"

// Struct Annotation.
Annotation::Annotation() {}

Annotation::~Annotation() {}

AnnotationPtr Annotation_Create() {
  return new Annotation();
}

void Annotation_Destroy(AnnotationPtr self) {
  delete self;
}

// Struct Annotation setters.
void Annotation_set_value(AnnotationPtr self, uint64_t value) {
  DCHECK(self);
  self->value = value;
}

// Struct Annotation getters.
uint64_t Annotation_get_value(AnnotationPtr self) {
  DCHECK(self);
  return self->value;
}

// Struct RawBuffer.
RawBuffer::RawBuffer() {}

RawBuffer::~RawBuffer() {}

RawBufferPtr RawBuffer_Create() {
  return new RawBuffer();
}

void RawBuffer_Destroy(RawBufferPtr self) {
  delete self;
}

// Struct RawBuffer setters.
void RawBuffer_add_data(RawBufferPtr self, int8_t data) {
  DCHECK(self);
  self->data.push_back(data);
}

// Struct RawBuffer getters.
uint32_t RawBuffer_get_dataSize(RawBufferPtr self) {
  DCHECK(self);
  return self->data.size();
}
int8_t RawBuffer_get_dataAtIndex(RawBufferPtr self, uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->data.size());
  return self->data[index];
}

// Struct Buffer.
Buffer::Buffer() {}

Buffer::~Buffer() {}

BufferPtr Buffer_Create() {
  return new Buffer();
}

void Buffer_Destroy(BufferPtr self) {
  delete self;
}

// Struct Buffer setters.
void Buffer_set_size(BufferPtr self, uint32_t size) {
  DCHECK(self);
  self->size = size;
}

void Buffer_set_limit(BufferPtr self, uint32_t limit) {
  DCHECK(self);
  self->limit = limit;
}

void Buffer_set_position(BufferPtr self, uint32_t position) {
  DCHECK(self);
  self->position = position;
}

void Buffer_set_data(BufferPtr self, RawBufferPtr data) {
  DCHECK(self);
  self->data.reset(data);
}

void Buffer_set_callback(BufferPtr self, BufferCallbackPtr callback) {
  DCHECK(self);
  self->callback = callback;
}

// Struct Buffer getters.
uint32_t Buffer_get_size(BufferPtr self) {
  DCHECK(self);
  return self->size;
}

uint32_t Buffer_get_limit(BufferPtr self) {
  DCHECK(self);
  return self->limit;
}

uint32_t Buffer_get_position(BufferPtr self) {
  DCHECK(self);
  return self->position;
}

RawBufferPtr Buffer_get_data(BufferPtr self) {
  DCHECK(self);
  return self->data.get();
}

BufferCallbackPtr Buffer_get_callback(BufferPtr self) {
  DCHECK(self);
  return self->callback;
}

// Struct CronetException.
CronetException::CronetException() {}

CronetException::~CronetException() {}

CronetExceptionPtr CronetException_Create() {
  return new CronetException();
}

void CronetException_Destroy(CronetExceptionPtr self) {
  delete self;
}

// Struct CronetException setters.
void CronetException_set_error_code(CronetExceptionPtr self,
                                    CronetException_ERROR_CODE error_code) {
  DCHECK(self);
  self->error_code = error_code;
}

void CronetException_set_cronet_internal_error_code(
    CronetExceptionPtr self,
    int32_t cronet_internal_error_code) {
  DCHECK(self);
  self->cronet_internal_error_code = cronet_internal_error_code;
}

void CronetException_set_immediately_retryable(CronetExceptionPtr self,
                                               bool immediately_retryable) {
  DCHECK(self);
  self->immediately_retryable = immediately_retryable;
}

void CronetException_set_quic_detailed_error_code(
    CronetExceptionPtr self,
    int32_t quic_detailed_error_code) {
  DCHECK(self);
  self->quic_detailed_error_code = quic_detailed_error_code;
}

// Struct CronetException getters.
CronetException_ERROR_CODE CronetException_get_error_code(
    CronetExceptionPtr self) {
  DCHECK(self);
  return self->error_code;
}

int32_t CronetException_get_cronet_internal_error_code(
    CronetExceptionPtr self) {
  DCHECK(self);
  return self->cronet_internal_error_code;
}

bool CronetException_get_immediately_retryable(CronetExceptionPtr self) {
  DCHECK(self);
  return self->immediately_retryable;
}

int32_t CronetException_get_quic_detailed_error_code(CronetExceptionPtr self) {
  DCHECK(self);
  return self->quic_detailed_error_code;
}

// Struct QuicHint.
QuicHint::QuicHint() {}

QuicHint::~QuicHint() {}

QuicHintPtr QuicHint_Create() {
  return new QuicHint();
}

void QuicHint_Destroy(QuicHintPtr self) {
  delete self;
}

// Struct QuicHint setters.
void QuicHint_set_host(QuicHintPtr self, CharString host) {
  DCHECK(self);
  self->host = host;
}

void QuicHint_set_port(QuicHintPtr self, int32_t port) {
  DCHECK(self);
  self->port = port;
}

void QuicHint_set_alternatePort(QuicHintPtr self, int32_t alternatePort) {
  DCHECK(self);
  self->alternatePort = alternatePort;
}

// Struct QuicHint getters.
CharString QuicHint_get_host(QuicHintPtr self) {
  DCHECK(self);
  return self->host.c_str();
}

int32_t QuicHint_get_port(QuicHintPtr self) {
  DCHECK(self);
  return self->port;
}

int32_t QuicHint_get_alternatePort(QuicHintPtr self) {
  DCHECK(self);
  return self->alternatePort;
}

// Struct PublicKeyPins.
PublicKeyPins::PublicKeyPins() {}

PublicKeyPins::~PublicKeyPins() {}

PublicKeyPinsPtr PublicKeyPins_Create() {
  return new PublicKeyPins();
}

void PublicKeyPins_Destroy(PublicKeyPinsPtr self) {
  delete self;
}

// Struct PublicKeyPins setters.
void PublicKeyPins_set_host(PublicKeyPinsPtr self, CharString host) {
  DCHECK(self);
  self->host = host;
}

void PublicKeyPins_add_pinsSha256(PublicKeyPinsPtr self,
                                  RawBufferPtr pinsSha256) {
  DCHECK(self);
  std::unique_ptr<RawBuffer> tmp_ptr(pinsSha256);
  self->pinsSha256.push_back(std::move(tmp_ptr));
}

void PublicKeyPins_set_includeSubdomains(PublicKeyPinsPtr self,
                                         bool includeSubdomains) {
  DCHECK(self);
  self->includeSubdomains = includeSubdomains;
}

// Struct PublicKeyPins getters.
CharString PublicKeyPins_get_host(PublicKeyPinsPtr self) {
  DCHECK(self);
  return self->host.c_str();
}

uint32_t PublicKeyPins_get_pinsSha256Size(PublicKeyPinsPtr self) {
  DCHECK(self);
  return self->pinsSha256.size();
}
RawBufferPtr PublicKeyPins_get_pinsSha256AtIndex(PublicKeyPinsPtr self,
                                                 uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->pinsSha256.size());
  return self->pinsSha256[index].get();
}

bool PublicKeyPins_get_includeSubdomains(PublicKeyPinsPtr self) {
  DCHECK(self);
  return self->includeSubdomains;
}

// Struct CronetEngineParams.
CronetEngineParams::CronetEngineParams() {}

CronetEngineParams::~CronetEngineParams() {}

CronetEngineParamsPtr CronetEngineParams_Create() {
  return new CronetEngineParams();
}

void CronetEngineParams_Destroy(CronetEngineParamsPtr self) {
  delete self;
}

// Struct CronetEngineParams setters.
void CronetEngineParams_set_userAgent(CronetEngineParamsPtr self,
                                      CharString userAgent) {
  DCHECK(self);
  self->userAgent = userAgent;
}

void CronetEngineParams_set_storagePath(CronetEngineParamsPtr self,
                                        CharString storagePath) {
  DCHECK(self);
  self->storagePath = storagePath;
}

void CronetEngineParams_set_enableQuic(CronetEngineParamsPtr self,
                                       bool enableQuic) {
  DCHECK(self);
  self->enableQuic = enableQuic;
}

void CronetEngineParams_set_enableHttp2(CronetEngineParamsPtr self,
                                        bool enableHttp2) {
  DCHECK(self);
  self->enableHttp2 = enableHttp2;
}

void CronetEngineParams_set_enableBrotli(CronetEngineParamsPtr self,
                                         bool enableBrotli) {
  DCHECK(self);
  self->enableBrotli = enableBrotli;
}

void CronetEngineParams_set_httpCacheMode(
    CronetEngineParamsPtr self,
    CronetEngineParams_HTTP_CACHE_MODE httpCacheMode) {
  DCHECK(self);
  self->httpCacheMode = httpCacheMode;
}

void CronetEngineParams_set_httpCacheMaxSize(CronetEngineParamsPtr self,
                                             int64_t httpCacheMaxSize) {
  DCHECK(self);
  self->httpCacheMaxSize = httpCacheMaxSize;
}

void CronetEngineParams_add_quicHints(CronetEngineParamsPtr self,
                                      QuicHintPtr quicHints) {
  DCHECK(self);
  std::unique_ptr<QuicHint> tmp_ptr(quicHints);
  self->quicHints.push_back(std::move(tmp_ptr));
}

void CronetEngineParams_add_publicKeyPins(CronetEngineParamsPtr self,
                                          PublicKeyPinsPtr publicKeyPins) {
  DCHECK(self);
  std::unique_ptr<PublicKeyPins> tmp_ptr(publicKeyPins);
  self->publicKeyPins.push_back(std::move(tmp_ptr));
}

void CronetEngineParams_set_enablePublicKeyPinningBypassForLocalTrustAnchors(
    CronetEngineParamsPtr self,
    bool enablePublicKeyPinningBypassForLocalTrustAnchors) {
  DCHECK(self);
  self->enablePublicKeyPinningBypassForLocalTrustAnchors =
      enablePublicKeyPinningBypassForLocalTrustAnchors;
}

// Struct CronetEngineParams getters.
CharString CronetEngineParams_get_userAgent(CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->userAgent.c_str();
}

CharString CronetEngineParams_get_storagePath(CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->storagePath.c_str();
}

bool CronetEngineParams_get_enableQuic(CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->enableQuic;
}

bool CronetEngineParams_get_enableHttp2(CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->enableHttp2;
}

bool CronetEngineParams_get_enableBrotli(CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->enableBrotli;
}

CronetEngineParams_HTTP_CACHE_MODE CronetEngineParams_get_httpCacheMode(
    CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->httpCacheMode;
}

int64_t CronetEngineParams_get_httpCacheMaxSize(CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->httpCacheMaxSize;
}

uint32_t CronetEngineParams_get_quicHintsSize(CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->quicHints.size();
}
QuicHintPtr CronetEngineParams_get_quicHintsAtIndex(CronetEngineParamsPtr self,
                                                    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->quicHints.size());
  return self->quicHints[index].get();
}

uint32_t CronetEngineParams_get_publicKeyPinsSize(CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->publicKeyPins.size();
}
PublicKeyPinsPtr CronetEngineParams_get_publicKeyPinsAtIndex(
    CronetEngineParamsPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->publicKeyPins.size());
  return self->publicKeyPins[index].get();
}

bool CronetEngineParams_get_enablePublicKeyPinningBypassForLocalTrustAnchors(
    CronetEngineParamsPtr self) {
  DCHECK(self);
  return self->enablePublicKeyPinningBypassForLocalTrustAnchors;
}

// Struct HttpHeader.
HttpHeader::HttpHeader() {}

HttpHeader::~HttpHeader() {}

HttpHeaderPtr HttpHeader_Create() {
  return new HttpHeader();
}

void HttpHeader_Destroy(HttpHeaderPtr self) {
  delete self;
}

// Struct HttpHeader setters.
void HttpHeader_set_name(HttpHeaderPtr self, CharString name) {
  DCHECK(self);
  self->name = name;
}

void HttpHeader_set_value(HttpHeaderPtr self, CharString value) {
  DCHECK(self);
  self->value = value;
}

// Struct HttpHeader getters.
CharString HttpHeader_get_name(HttpHeaderPtr self) {
  DCHECK(self);
  return self->name.c_str();
}

CharString HttpHeader_get_value(HttpHeaderPtr self) {
  DCHECK(self);
  return self->value.c_str();
}

// Struct UrlResponseInfo.
UrlResponseInfo::UrlResponseInfo() {}

UrlResponseInfo::~UrlResponseInfo() {}

UrlResponseInfoPtr UrlResponseInfo_Create() {
  return new UrlResponseInfo();
}

void UrlResponseInfo_Destroy(UrlResponseInfoPtr self) {
  delete self;
}

// Struct UrlResponseInfo setters.
void UrlResponseInfo_set_url(UrlResponseInfoPtr self, CharString url) {
  DCHECK(self);
  self->url = url;
}

void UrlResponseInfo_add_urlChain(UrlResponseInfoPtr self,
                                  CharString urlChain) {
  DCHECK(self);
  self->urlChain.push_back(urlChain);
}

void UrlResponseInfo_set_httpStatusCode(UrlResponseInfoPtr self,
                                        int32_t httpStatusCode) {
  DCHECK(self);
  self->httpStatusCode = httpStatusCode;
}

void UrlResponseInfo_set_httpStatusText(UrlResponseInfoPtr self,
                                        CharString httpStatusText) {
  DCHECK(self);
  self->httpStatusText = httpStatusText;
}

void UrlResponseInfo_add_allHeadersList(UrlResponseInfoPtr self,
                                        HttpHeaderPtr allHeadersList) {
  DCHECK(self);
  std::unique_ptr<HttpHeader> tmp_ptr(allHeadersList);
  self->allHeadersList.push_back(std::move(tmp_ptr));
}

void UrlResponseInfo_set_wasCached(UrlResponseInfoPtr self, bool wasCached) {
  DCHECK(self);
  self->wasCached = wasCached;
}

void UrlResponseInfo_set_negotiatedProtocol(UrlResponseInfoPtr self,
                                            CharString negotiatedProtocol) {
  DCHECK(self);
  self->negotiatedProtocol = negotiatedProtocol;
}

void UrlResponseInfo_set_proxyServer(UrlResponseInfoPtr self,
                                     CharString proxyServer) {
  DCHECK(self);
  self->proxyServer = proxyServer;
}

void UrlResponseInfo_set_receivedByteCount(UrlResponseInfoPtr self,
                                           int64_t receivedByteCount) {
  DCHECK(self);
  self->receivedByteCount = receivedByteCount;
}

// Struct UrlResponseInfo getters.
CharString UrlResponseInfo_get_url(UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->url.c_str();
}

uint32_t UrlResponseInfo_get_urlChainSize(UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->urlChain.size();
}
CharString UrlResponseInfo_get_urlChainAtIndex(UrlResponseInfoPtr self,
                                               uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->urlChain.size());
  return self->urlChain[index].c_str();
}

int32_t UrlResponseInfo_get_httpStatusCode(UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->httpStatusCode;
}

CharString UrlResponseInfo_get_httpStatusText(UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->httpStatusText.c_str();
}

uint32_t UrlResponseInfo_get_allHeadersListSize(UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->allHeadersList.size();
}
HttpHeaderPtr UrlResponseInfo_get_allHeadersListAtIndex(UrlResponseInfoPtr self,
                                                        uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->allHeadersList.size());
  return self->allHeadersList[index].get();
}

bool UrlResponseInfo_get_wasCached(UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->wasCached;
}

CharString UrlResponseInfo_get_negotiatedProtocol(UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->negotiatedProtocol.c_str();
}

CharString UrlResponseInfo_get_proxyServer(UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->proxyServer.c_str();
}

int64_t UrlResponseInfo_get_receivedByteCount(UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->receivedByteCount;
}

// Struct UrlRequestParams.
UrlRequestParams::UrlRequestParams() {}

UrlRequestParams::~UrlRequestParams() {}

UrlRequestParamsPtr UrlRequestParams_Create() {
  return new UrlRequestParams();
}

void UrlRequestParams_Destroy(UrlRequestParamsPtr self) {
  delete self;
}

// Struct UrlRequestParams setters.
void UrlRequestParams_set_url(UrlRequestParamsPtr self, CharString url) {
  DCHECK(self);
  self->url = url;
}

void UrlRequestParams_set_httpMethod(UrlRequestParamsPtr self,
                                     CharString httpMethod) {
  DCHECK(self);
  self->httpMethod = httpMethod;
}

void UrlRequestParams_add_requestHeaders(UrlRequestParamsPtr self,
                                         HttpHeaderPtr requestHeaders) {
  DCHECK(self);
  std::unique_ptr<HttpHeader> tmp_ptr(requestHeaders);
  self->requestHeaders.push_back(std::move(tmp_ptr));
}

void UrlRequestParams_set_disableCache(UrlRequestParamsPtr self,
                                       bool disableCache) {
  DCHECK(self);
  self->disableCache = disableCache;
}

void UrlRequestParams_set_priority(UrlRequestParamsPtr self,
                                   UrlRequestParams_REQUEST_PRIORITY priority) {
  DCHECK(self);
  self->priority = priority;
}

void UrlRequestParams_set_uploadDataProvider(
    UrlRequestParamsPtr self,
    UploadDataProviderPtr uploadDataProvider) {
  DCHECK(self);
  self->uploadDataProvider = uploadDataProvider;
}

void UrlRequestParams_set_uploadDataProviderExecutor(
    UrlRequestParamsPtr self,
    ExecutorPtr uploadDataProviderExecutor) {
  DCHECK(self);
  self->uploadDataProviderExecutor = uploadDataProviderExecutor;
}

void UrlRequestParams_set_allowDirectExecutor(UrlRequestParamsPtr self,
                                              bool allowDirectExecutor) {
  DCHECK(self);
  self->allowDirectExecutor = allowDirectExecutor;
}

void UrlRequestParams_add_annotations(UrlRequestParamsPtr self,
                                      AnnotationPtr annotations) {
  DCHECK(self);
  std::unique_ptr<Annotation> tmp_ptr(annotations);
  self->annotations.push_back(std::move(tmp_ptr));
}

// Struct UrlRequestParams getters.
CharString UrlRequestParams_get_url(UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->url.c_str();
}

CharString UrlRequestParams_get_httpMethod(UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->httpMethod.c_str();
}

uint32_t UrlRequestParams_get_requestHeadersSize(UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->requestHeaders.size();
}
HttpHeaderPtr UrlRequestParams_get_requestHeadersAtIndex(
    UrlRequestParamsPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->requestHeaders.size());
  return self->requestHeaders[index].get();
}

bool UrlRequestParams_get_disableCache(UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->disableCache;
}

UrlRequestParams_REQUEST_PRIORITY UrlRequestParams_get_priority(
    UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->priority;
}

UploadDataProviderPtr UrlRequestParams_get_uploadDataProvider(
    UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->uploadDataProvider;
}

ExecutorPtr UrlRequestParams_get_uploadDataProviderExecutor(
    UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->uploadDataProviderExecutor;
}

bool UrlRequestParams_get_allowDirectExecutor(UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->allowDirectExecutor;
}

uint32_t UrlRequestParams_get_annotationsSize(UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->annotations.size();
}
AnnotationPtr UrlRequestParams_get_annotationsAtIndex(UrlRequestParamsPtr self,
                                                      uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->annotations.size());
  return self->annotations[index].get();
}
