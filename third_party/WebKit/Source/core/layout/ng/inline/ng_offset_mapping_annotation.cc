// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_offset_mapping_annotation.h"

#include "platform/wtf/StdLibExtras.h"

namespace blink {

NGOffsetMappingAnnotation::~NGOffsetMappingAnnotation() = default;

#define V(type) \
  bool NGOffsetMappingAnnotation::Is##type() const { return false; }
FOR_EACH_ANNOTATION_TYPE_INCLUDING_NULL(V)
#undef V

RefPtr<NGOffsetMappingAnnotation> NGOffsetMappingAnnotation::Composite(
    const NGOffsetMappingAnnotation& other) const {
  if (IsNull())
    return const_cast<NGOffsetMappingAnnotation*>(&other);
  if (other.IsNull())
    return const_cast<NGOffsetMappingAnnotation*>(this);

  if (IsIndex()) {
    DCHECK(other.IsIndex());
    return const_cast<NGOffsetMappingAnnotation*>(&other);
  }

  DCHECK(IsNode() || IsNodeIndexPair());
  const LayoutText* result_node =
      IsNode() ? ToNGNodeAnnotation(this)->GetValue()
               : ToNGNodeIndexPairAnnotation(this)->GetValue().first;

  DCHECK(other.IsIndex());
  unsigned result_index = ToNGIndexAnnotation(other).GetValue();

  return AdoptRef<NGOffsetMappingAnnotation>(
      new NGNodeIndexPairAnnotation({result_node, result_index}));
}

// ----------

NGNullAnnotation::~NGNullAnnotation() = default;

bool NGNullAnnotation::IsNull() const {
  return true;
}

// static
RefPtr<NGOffsetMappingAnnotation> NGNullAnnotation::GetInstance() {
  DEFINE_STATIC_LOCAL(
      RefPtr<NGOffsetMappingAnnotation>, instance,
      (AdoptRef<NGOffsetMappingAnnotation>(new NGNullAnnotation)));
  return instance;
}

// ----------

template <typename ValueType>
NGAnnotationTemplate<ValueType>::~NGAnnotationTemplate() = default;

#define V(type)                                            \
  template <typename ValueType>                            \
  bool NGAnnotationTemplate<ValueType>::Is##type() const { \
    return false;                                          \
  }
FOR_EACH_ANNOTATION_TYPE(V)
#undef V

#define V(type)                                                          \
  template <>                                                            \
  bool NGAnnotationTemplate<NGOffsetMappingAnnotation::type>::Is##type() \
      const {                                                            \
    return true;                                                         \
  }
FOR_EACH_ANNOTATION_TYPE(V)
#undef V

// ----------

#define V(type)                       \
  template class CORE_TEMPLATE_EXPORT \
      NGAnnotationTemplate<NGOffsetMappingAnnotation::type>;
FOR_EACH_ANNOTATION_TYPE(V)
#undef V

}  // namespace blink
