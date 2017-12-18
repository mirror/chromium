// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DocumentPolicy_h
#define DocumentPolicy_h

#include "core/CoreExport.h"
#include "core/dom/Document.h"
#include "core/policy/Policy.h"
#include "platform/heap/Member.h"

namespace blink {

class CORE_EXPORT DocumentPolicy final : public Policy {
 public:
  // Create a new DocumentPolicy, which is associated with the FeaturePolicy of
  // the document.
  DocumentPolicy(Document* document)
      : document_(document), policy_(document_->GetFeaturePolicy()) {}

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(document_);
    ScriptWrappable::Trace(visitor);
  }

 protected:
  const FeaturePolicy* GetPolicy() const override { return policy_; }
  Document* GetDocument() const override { return document_; }

 private:
  Member<Document> document_;
  const FeaturePolicy* policy_;
};

}  // namespace blink

#endif  // DocumentPolicy_h
