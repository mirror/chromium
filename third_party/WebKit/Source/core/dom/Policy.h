// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef Policy_h
#define Policy_h

#include "core/CoreExport.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Member.h"
#include "third_party/WebKit/common/feature_policy/feature_policy.h"

namespace blink {

class Document;
class SecurityOrigin;

class CORE_EXPORT Policy final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~Policy() {}
  // Create a Policy for a document. The policy associated with the document
  // itself.
  static Policy* Create(Document*);
  // Create a Policy for a frame. The policy is synthetic for a frame
  // conatined within the document.
  static Policy* Create(Document* parent_document,
                        const ParsedFeaturePolicy&,
                        const scoped_refptr<SecurityOrigin>);

  // Inform the Policy object when the container policy on its frame element has
  // changed.
  void UpdateContainerPolicy(const ParsedFeaturePolicy&,
                             const scoped_refptr<SecurityOrigin>);

  // Returns whether or not the given feature is allowed on the origin of the
  // document that owns the policy.
  bool allowsFeature(const String& feature) const;
  // Returns whether or not the given feature is allowed on the origin of the
  // given URL.
  bool allowsFeature(const String& feature, const String& url) const;
  // Returns a list of feature names that are allowed on the self origin.
  Vector<String> allowedFeatures() const;
  // Returns a list of feature name that are allowed on the origin of the given
  // URL.
  Vector<String> getAllowlistForFeature(const String& url) const;

  void Trace(blink::Visitor*);

 private:
  // Add console message to |document_|.
  void AddWarningForUnrecognizedFeature(const String& message) const;

  explicit Policy(Document*);
  explicit Policy(Document* parent_document,
                  const ParsedFeaturePolicy&,
                  const scoped_refptr<SecurityOrigin>);
  Member<Document> document_;
  std::unique_ptr<FeaturePolicy> policy_;
};

}  // namespace blink

#endif
