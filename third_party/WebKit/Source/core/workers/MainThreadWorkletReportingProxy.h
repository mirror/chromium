// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MainThreadWorkletReportingProxy_h
#define MainThreadWorkletReportingProxy_h

#include "core/workers/WorkerReportingProxy.h"

namespace blink {

class Document;

class MainThreadWorkletReportingProxy final : public WorkerReportingProxy {
 public:
  explicit MainThreadWorkletReportingProxy(Document*);

  // Implements WorkerReportingProxy.
  void CountFeature(WebFeature) override;
  void CountDeprecation(WebFeature) override;

 private:
  Persistent<Document> document_;
};

}  // namespace blink

#endif  // MainThreadWorkletReportingProxy_h
