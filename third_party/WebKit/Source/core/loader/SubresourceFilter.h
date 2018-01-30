// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SubresourceFilter_h
#define SubresourceFilter_h

#include <memory>

#include "core/CoreExport.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/SecurityViolationReportingPolicy.h"
#include "public/platform/WebDocumentSubresourceFilter.h"
#include "public/platform/WebURLRequest.h"

namespace blink {

class ExecutionContext;
class KURL;

// Wrapper around a WebDocumentSubresourceFilter. This class will make it easier
// to extend the subresource filter with optimizations only possible using blink
// types (e.g. a caching layer using StringImpl).
class CORE_EXPORT SubresourceFilter final
    : public GarbageCollectedFinalized<SubresourceFilter> {
 public:
  static SubresourceFilter* Create(
      ExecutionContext&,
      std::unique_ptr<WebDocumentSubresourceFilter>);
  ~SubresourceFilter();

  bool AllowLoad(const KURL& resource_url,
                 WebURLRequest::RequestContext,
                 SecurityViolationReportingPolicy);
  bool AllowWebSocketConnection(const KURL&);

  // Returns if the last checked resource was an ad resource. Note that this
  // will return false if the |resource_url| and type do not match the last
  // resource that was checked in AllowLoad.
  bool IsAdResource(const KURL&, WebURLRequest::RequestContext);

  virtual void Trace(blink::Visitor*);

 private:
  SubresourceFilter(ExecutionContext*,
                    std::unique_ptr<WebDocumentSubresourceFilter>);

  void ReportLoad(const KURL& resource_url,
                  WebDocumentSubresourceFilter::LoadPolicy);

  Member<ExecutionContext> execution_context_;
  std::unique_ptr<WebDocumentSubresourceFilter> subresource_filter_;

  // Save the last resource check's result.
  std::pair<std::pair<KURL, WebURLRequest::RequestContext>,
            WebDocumentSubresourceFilter::LoadPolicy>
      last_resource_check_result_;
};

}  // namespace blink

#endif  // SubresourceFilter_h
