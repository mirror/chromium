// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DocumentPolicy_h
#define DocumentPolicy_h

#include "core/dom/Policy.h"
#include "platform/heap/Member.h"

namespace blink {

class DocumentPolicy final : public Policy {
 public:
  // Create a DocumentPolicy, which is associated with the FeaturePolicy of the
  // document.
  static Policy* Create(Document* document) {
    return new DocumentPolicy(document);
  }

  void UpdateContainerPolicy(
      const ParsedFeaturePolicy* = nullptr,
      const scoped_refptr<SecurityOrigin> = nullptr) override {}

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(document_);
    ScriptWrappable::Trace(visitor);
  }

 protected:
  const FeaturePolicy* GetPolicy() const override { return policy_; }
  Document* GetDocument() const override { return document_; }

 private:
  explicit DocumentPolicy(Document* document)
      : document_(document), policy_(document_->GetFeaturePolicy()) {}
  Member<Document> document_;
  const FeaturePolicy* policy_;
};

}  // namespace blink

#endif  // DocumentPolicy_h
