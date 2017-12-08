// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FramePolicy_h
#define FramePolicy_h

#include "core/policy/Policy.h"
#include "platform/heap/Member.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace blink {

class FramePolicy final : public Policy {
 public:
  // Create a FramePolicy, which is synthetic for a frame contained within a
  // document.
  static Policy* Create(Document* document,
                        const ParsedFeaturePolicy& container_policy,
                        scoped_refptr<const SecurityOrigin> src_origin) {
    DCHECK(src_origin);
    return new FramePolicy(document, container_policy, src_origin);
  }

  void UpdateContainerPolicy(
      const ParsedFeaturePolicy& container_policy,
      scoped_refptr<const SecurityOrigin> src_origin) override {
    policy_ = FeaturePolicy::CreateFromParentPolicy(
        parent_document_->GetFeaturePolicy(), container_policy,
        src_origin->ToUrlOrigin());
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(parent_document_);
    ScriptWrappable::Trace(visitor);
  }

 protected:
  const FeaturePolicy* GetPolicy() const override { return policy_.get(); }
  Document* GetDocument() const override { return parent_document_; }

 private:
  explicit FramePolicy(Document* parent_document,
                       const ParsedFeaturePolicy& container_policy,
                       scoped_refptr<const SecurityOrigin> src_origin)
      : parent_document_(parent_document) {
    UpdateContainerPolicy(container_policy, src_origin);
  }
  Member<Document> parent_document_;
  std::unique_ptr<FeaturePolicy> policy_;
};

}  // namespace blink

#endif  // FramePolicy_h
