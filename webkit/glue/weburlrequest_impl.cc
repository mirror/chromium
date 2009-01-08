// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#pragma warning(push, 0)
#include "FormData.h"
#include "HTTPHeaderMap.h"
#include "ResourceRequest.h"
#pragma warning(pop)

#undef LOG
#include "base/logging.h"
#include "net/base/upload_data.h"
#include "webkit/glue/weburlrequest_impl.h"
#include "webkit/glue/glue_serialize.h"
#include "webkit/glue/glue_util.h"

using WebCore::FrameLoadRequest;
using WebCore::ResourceRequestCachePolicy;
using WebCore::String;

// WebRequest -----------------------------------------------------------------

WebRequestImpl::WebRequestImpl() {
}

WebRequestImpl::WebRequestImpl(const GURL& url)
    : request_(FrameLoadRequest(webkit_glue::GURLToKURL(url))) {
}

WebRequestImpl::WebRequestImpl(const WebCore::ResourceRequest& request)
    : request_(FrameLoadRequest(request)) {
}

WebRequestImpl::WebRequestImpl(const WebCore::FrameLoadRequest& request)
    : request_(request) {
}

WebRequest* WebRequestImpl::Clone() const {
  return new WebRequestImpl(*this);
}

void WebRequestImpl::SetExtraData(ExtraData* extra) {
  extra_data_ = extra;
}

WebRequest::ExtraData* WebRequestImpl::GetExtraData() const {
  return extra_data_.get();
}

GURL WebRequestImpl::GetURL() const {
  return webkit_glue::KURLToGURL(request_.resourceRequest().url());
}

void WebRequestImpl::SetURL(const GURL& url) {
  request_.resourceRequest().setURL(webkit_glue::GURLToKURL(url));
}

GURL WebRequestImpl::GetMainDocumentURL() const {
  return webkit_glue::KURLToGURL(request_.resourceRequest().mainDocumentURL());
}

void WebRequestImpl::SetMainDocumentURL(const GURL& url) {
  request_.resourceRequest().setMainDocumentURL(webkit_glue::GURLToKURL(url));
}

WebRequestCachePolicy WebRequestImpl::GetCachePolicy() const {
  // WebRequestCachePolicy mirrors ResourceRequestCachePolicy
  return static_cast<WebRequestCachePolicy>(
      request_.resourceRequest().cachePolicy());
}

void WebRequestImpl::SetCachePolicy(WebRequestCachePolicy policy) {
  // WebRequestCachePolicy mirrors ResourceRequestCachePolicy
  request_.resourceRequest().setCachePolicy(
      static_cast<WebCore::ResourceRequestCachePolicy>(policy));
}

std::wstring WebRequestImpl::GetHttpMethod() const {
  return webkit_glue::StringToStdWString(
      request_.resourceRequest().httpMethod());
}

void WebRequestImpl::SetHttpMethod(const std::wstring& method) {
  request_.resourceRequest().setHTTPMethod(
      webkit_glue::StdWStringToString(method));
}

std::wstring WebRequestImpl::GetHttpHeaderValue(const std::wstring& field) const {
  return webkit_glue::StringToStdWString(
      request_.resourceRequest().httpHeaderField(
          webkit_glue::StdWStringToString(field)));
}

void WebRequestImpl::SetHttpHeaderValue(const std::wstring& field,
                                        const std::wstring& value) {
  request_.resourceRequest().setHTTPHeaderField(
      webkit_glue::StdWStringToString(field),
      webkit_glue::StdWStringToString(value));
}

void WebRequestImpl::GetHttpHeaders(HeaderMap* headers) const {
  headers->clear();

  const WebCore::HTTPHeaderMap& map =
      request_.resourceRequest().httpHeaderFields();
  WebCore::HTTPHeaderMap::const_iterator end = map.end();
  WebCore::HTTPHeaderMap::const_iterator it = map.begin();
  for (; it != end; ++it) {
    headers->insert(
        std::make_pair(
            webkit_glue::StringToStdString(it->first),
            webkit_glue::StringToStdString(it->second)));
  }
}

void WebRequestImpl::SetHttpHeaders(const HeaderMap& headers) {
  WebCore::ResourceRequest& request = request_.resourceRequest();

  HeaderMap::const_iterator end = headers.end();
  HeaderMap::const_iterator it = headers.begin();
  for (; it != end; ++it) {
    request.setHTTPHeaderField(
        webkit_glue::StdStringToString(it->first),
        webkit_glue::StdStringToString(it->second));
  }
}

std::wstring WebRequestImpl::GetHttpReferrer() const {
  return webkit_glue::StringToStdWString(
      request_.resourceRequest().httpReferrer());
}

std::string WebRequestImpl::GetHistoryState() const {
  std::string value;
  webkit_glue::HistoryItemToString(history_item_, &value);
  return value;
}

void WebRequestImpl::SetHistoryState(const std::string& value) {
  history_item_ = webkit_glue::HistoryItemFromString(value);
}

std::string WebRequestImpl::GetSecurityInfo() const {
  return webkit_glue::CStringToStdString(
      request_.resourceRequest().securityInfo());
}

void WebRequestImpl::SetSecurityInfo(const std::string& value) {
  request_.resourceRequest().setSecurityInfo(
      webkit_glue::StdStringToCString(value));
}

bool WebRequestImpl::HasFormData() const {
  return history_item() && history_item()->formData();
}

// static
WebRequest* WebRequest::Create(const GURL& url) {
  return new WebRequestImpl(url);
}

