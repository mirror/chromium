// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SubresourceFilter_h
#define SubresourceFilter_h

#include <memory>

#include "core/CoreExport.h"
#include "core/loader/DocumentLoader.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/SecurityViolationReportingPolicy.h"
#include "public/platform/WebDocumentSubresourceFilter.h"
#include "public/platform/WebURLRequest.h"

namespace blink {

class KURL;
class WorkerOrWorkletGlobalScope;

// Wrapper around a WebDocumentSubresourceFilter. This class will make it easier
// to extend the subresource filter with optimizations only possible using blink
// types (e.g. a caching layer using StringImpl).
class CORE_EXPORT SubresourceFilter final
    : public GarbageCollectedFinalized<SubresourceFilter> {
 public:
  static SubresourceFilter* Create(
      DocumentLoader*,
      std::unique_ptr<WebDocumentSubresourceFilter>);
  static SubresourceFilter* CreateForWorker(
      WorkerOrWorkletGlobalScope* worker_global_scope,
      std::unique_ptr<WebDocumentSubresourceFilter>);
  ~SubresourceFilter();

  bool AllowLoad(const KURL& resource_url,
                 WebURLRequest::RequestContext,
                 SecurityViolationReportingPolicy);
  bool AllowWebSocketConnection(const KURL&);

  DECLARE_VIRTUAL_TRACE();

 private:
  SubresourceFilter(DocumentLoader*,
                    std::unique_ptr<WebDocumentSubresourceFilter>);
  SubresourceFilter(WorkerOrWorkletGlobalScope*,
                    std::unique_ptr<WebDocumentSubresourceFilter>);

  void ReportLoad(const KURL& resource_url,
                  WebDocumentSubresourceFilter::LoadPolicy);

  // This is null for Worker.
  Member<DocumentLoader> document_loader_;
  Member<WorkerOrWorkletGlobalScope> worker_global_scope_;
  std::unique_ptr<WebDocumentSubresourceFilter> subresource_filter_;
};

}  // namespace blink

#endif  // SubresourceFilter_h
