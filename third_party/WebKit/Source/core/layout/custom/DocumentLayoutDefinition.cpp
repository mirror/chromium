// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/custom/DocumentLayoutDefinition.h"

namespace blink {

DocumentLayoutDefinition::DocumentLayoutDefinition(
    CSSLayoutDefinition* definition)
    : layout_definition_(definition), registered_definitions_count_(1u) {
  DCHECK(definition);
}

DocumentLayoutDefinition::~DocumentLayoutDefinition() = default;

bool DocumentLayoutDefinition::RegisterAdditionalLayoutDefinition(
    const CSSLayoutDefinition& other) {
  if (NativeInvalidationProperties() != other.NativeInvalidationProperties() ||
      CustomInvalidationProperties() != other.CustomInvalidationProperties() ||
      ChildNativeInvalidationProperties() !=
          other.ChildNativeInvalidationProperties() ||
      ChildCustomInvalidationProperties() !=
          other.ChildCustomInvalidationProperties())
    return false;
  registered_definitions_count_++;
  return true;
}

void DocumentLayoutDefinition::Trace(blink::Visitor* visitor) {
  visitor->Trace(layout_definition_);
}

}  // namespace blink
