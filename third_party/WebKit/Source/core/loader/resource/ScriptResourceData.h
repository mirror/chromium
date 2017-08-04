// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScriptResourceData_h
#define ScriptResourceData_h

#include "core/CoreExport.h"
#include "platform/heap/Handle.h"
#include "platform/loader/fetch/AccessControlStatus.h"
#include "platform/loader/fetch/CachedMetadataHandler.h"
#include "platform/loader/fetch/ResourceLoaderOptions.h"
#include "platform/loader/fetch/ResourceResponse.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/CORSStatus.h"
#include "public/platform/WebURLRequest.h"

namespace blink {

class CORE_EXPORT ScriptResourceData final
    : public GarbageCollectedFinalized<ScriptResourceData> {
 public:
  ScriptResourceData(const KURL& url,
                     WebURLRequest::FetchCredentialsMode fetch_credentials_mode,
                     const ResourceResponse& response,
                     bool error_occurred,
                     const AtomicString& source_text,
                     CachedMetadataHandler* cache_handler,
                     CORSStatus cors_status)
      : url_(url),
        fetch_credentials_mode_(fetch_credentials_mode),
        response_(response),
        error_occurred_(error_occurred),
        source_text_(source_text),
        cache_handler_(cache_handler),
        cors_status_(cors_status) {}

  DEFINE_INLINE_TRACE() { visitor->Trace(cache_handler_); }

  // ResourceRequest-originated fields.
  const KURL& Url() const { return url_; }
  WebURLRequest::FetchCredentialsMode GetFetchCredentialsMode() const {
    return fetch_credentials_mode_;
  }

  // ResourceResponse.
  const ResourceResponse GetResponse() const { return response_; }
  const String& SourceText() const { return source_text_; }

  bool ErrorOccurred() const { return error_occurred_; }
  CachedMetadataHandler* CacheHandler() const { return cache_handler_; }

  AccessControlStatus GetAccessControlStatus() const;

  static bool MimeTypeAllowedByNosniff(const ResourceResponse&);

 private:
  const KURL url_;
  const WebURLRequest::FetchCredentialsMode fetch_credentials_mode_;

  const ResourceResponse response_;

  const bool error_occurred_;

  const AtomicString source_text_;

  const Member<CachedMetadataHandler> cache_handler_;

  const CORSStatus cors_status_;
};

}  // namespace blink

#endif
