// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_offset_mapping_annotation.h"

#include "platform/wtf/StdLibExtras.h"

namespace blink {

NGOffsetMappingAnnotation::~NGOffsetMappingAnnotation() = default;
NGNullAnnotation::~NGNullAnnotation() = default;
NGNodeAnnotation::~NGNodeAnnotation() = default;

bool NGOffsetMappingAnnotation::IsNullAnnotation() const {
  return false;
}

bool NGOffsetMappingAnnotation::IsNodeAnnotation() const {
  return false;
}

bool NGNullAnnotation::IsNullAnnotation() const {
  return true;
}

bool NGNodeAnnotation::IsNodeAnnotation() const {
  return true;
}

// static
RefPtr<NGOffsetMappingAnnotation> NGNullAnnotation::GetInstance() {
  DEFINE_STATIC_LOCAL(
      RefPtr<NGOffsetMappingAnnotation>, instance,
      (AdoptRef<NGOffsetMappingAnnotation>(new NGNullAnnotation)));
  return instance;
}

RefPtr<NGOffsetMappingAnnotation> NGNullAnnotation::Composite(
    const NGOffsetMappingAnnotation& other) const {
  return const_cast<NGOffsetMappingAnnotation*>(&other);
}

RefPtr<NGOffsetMappingAnnotation> NGNodeAnnotation::Composite(
    const NGOffsetMappingAnnotation& other) const {
  // TODO(xiaochengh): Allow |node X index| composition.
  DCHECK(!other.IsNodeAnnotation());
  return const_cast<NGNodeAnnotation*>(this);
}

}  // namespace blink
