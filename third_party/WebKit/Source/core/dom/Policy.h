// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef Policy_h
#define Policy_h

#include "core/CoreExport.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/feature_policy/FeaturePolicy.h"
#include "platform/heap/GarbageCollected.h"
#include "platform/heap/Member.h"

namespace blink {

class Document;

class CORE_EXPORT Policy final : public GarbageCollectedFinalized<Policy>,
                                 public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~Policy() {}
  static Policy* Create(Document*);
  // Returns whether or not the given feature is allowed on the self origin of
  // the document that owns the policy.
  bool allowsFeature(const String& feature) const;
  // Returns whether or not the given feature is allowed on the given origin.
  bool allowsFeature(const String& feature, const String& origin) const;
  // Returns a list of feature names that are allowed on the self origin.
  Vector<String> allowedFeatures() const;
  // Returns a list of feature name that are allowed on the given origin.
  Vector<String> getAllowlistForFeature(const String& origin) const;
  DECLARE_TRACE();

 private:
  // Add console message to |document_|.
  void AddWarningForUnrecognizedFeature(const String& message) const;

  Policy(Document*);
  Member<Document> document_;
  const WebFeaturePolicy* policy_;
  const FeatureNameMap& feature_names_ = GetDefaultFeatureNameMap();
};

}  // namespace blink

#endif
