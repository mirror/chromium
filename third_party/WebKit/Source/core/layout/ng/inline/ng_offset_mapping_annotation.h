// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGOffsetMappingAnnotation_h
#define NGOffsetMappingAnnotation_h

#include "core/CoreExport.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/RefCounted.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

class LayoutText;

#define FOR_EACH_ANNOTATION_TYPE(V) \
  V(Node)                           \
  V(Index)                          \
  V(NodeIndexPair)

#define FOR_EACH_ANNOTATION_TYPE_INCLUDING_NULL(V) \
  V(Null)                                          \
  FOR_EACH_ANNOTATION_TYPE(V)

class CORE_EXPORT NGOffsetMappingAnnotation
    : public RefCounted<NGOffsetMappingAnnotation> {
 public:
  using Node = const LayoutText*;
  using Index = unsigned;
  using NodeIndexPair = std::pair<Node, Index>;

  virtual ~NGOffsetMappingAnnotation();

#define V(type) virtual bool Is##type() const;
  FOR_EACH_ANNOTATION_TYPE_INCLUDING_NULL(V)
#undef V

  RefPtr<NGOffsetMappingAnnotation> Composite(
      const NGOffsetMappingAnnotation&) const;

 protected:
  NGOffsetMappingAnnotation() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(NGOffsetMappingAnnotation);
};

class CORE_EXPORT NGNullAnnotation final : public NGOffsetMappingAnnotation {
 public:
  static RefPtr<NGOffsetMappingAnnotation> GetInstance();

  ~NGNullAnnotation() final;
  bool IsNull() const final;

 private:
  // There is only one NGNullAnnotation instance, returned by GetInstance().
  NGNullAnnotation() = default;

  DISALLOW_COPY_AND_ASSIGN(NGNullAnnotation);
};

template <typename ValueType>
class CORE_TEMPLATE_CLASS_EXPORT NGAnnotationTemplate final
    : public NGOffsetMappingAnnotation {
 public:
  explicit NGAnnotationTemplate(ValueType value) : value_(value) {}

  ~NGAnnotationTemplate() final;

#define V(type) bool Is##type() const final;
  FOR_EACH_ANNOTATION_TYPE(V)
#undef V

  ValueType GetValue() const { return value_; }

 private:
  const ValueType value_;

  DISALLOW_COPY_AND_ASSIGN(NGAnnotationTemplate);
};

#define V(type)                                                          \
  template <>                                                            \
  bool NGAnnotationTemplate<NGOffsetMappingAnnotation::type>::Is##type() \
      const;
FOR_EACH_ANNOTATION_TYPE(V)
#undef V

#define V(type)                                     \
  extern template class CORE_EXTERN_TEMPLATE_EXPORT \
      NGAnnotationTemplate<NGOffsetMappingAnnotation::type>;
FOR_EACH_ANNOTATION_TYPE(V)
#undef V

#define V(type)                \
  using NG##type##Annotation = \
      NGAnnotationTemplate<NGOffsetMappingAnnotation::type>;
FOR_EACH_ANNOTATION_TYPE(V)
#undef V

#define V(type)                                                      \
  DEFINE_TYPE_CASTS(NG##type##Annotation, NGOffsetMappingAnnotation, \
                    annotation, annotation->Is##type(),              \
                    annotation.Is##type());
FOR_EACH_ANNOTATION_TYPE(V)
#undef V

}  // namespace blink

#endif  // NGOffsetMappingAnnotation_h
