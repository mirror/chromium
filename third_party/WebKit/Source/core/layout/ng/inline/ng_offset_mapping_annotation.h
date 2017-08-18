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

class CORE_EXPORT NGOffsetMappingAnnotation
    : public RefCounted<NGOffsetMappingAnnotation> {
 public:
  virtual ~NGOffsetMappingAnnotation();
  virtual bool IsNullAnnotation() const;
  virtual bool IsNodeAnnotation() const;

  virtual RefPtr<NGOffsetMappingAnnotation> Composite(
      const NGOffsetMappingAnnotation&) const = 0;

 protected:
  NGOffsetMappingAnnotation() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(NGOffsetMappingAnnotation);
};

class CORE_EXPORT NGNullAnnotation final : public NGOffsetMappingAnnotation {
 public:
  static RefPtr<NGOffsetMappingAnnotation> GetInstance();

  ~NGNullAnnotation() final;
  bool IsNullAnnotation() const final;
  RefPtr<NGOffsetMappingAnnotation> Composite(
      const NGOffsetMappingAnnotation&) const final;

 private:
  // There is only one NGNullAnnotation instance, returned by GetInstance().
  NGNullAnnotation() = default;

  DISALLOW_COPY_AND_ASSIGN(NGNullAnnotation);
};

class CORE_EXPORT NGNodeAnnotation final : public NGOffsetMappingAnnotation {
 public:
  explicit NGNodeAnnotation(const LayoutText* value) : value_(value) {}

  ~NGNodeAnnotation() final;
  bool IsNodeAnnotation() const final;
  RefPtr<NGOffsetMappingAnnotation> Composite(
      const NGOffsetMappingAnnotation&) const final;

  const LayoutText* GetValue() const { return value_; }

 private:
  // TODO(xiaochengh): Store a reference instead.
  const LayoutText* const value_;

  DISALLOW_COPY_AND_ASSIGN(NGNodeAnnotation);
};

DEFINE_TYPE_CASTS(NGNodeAnnotation,
                  NGOffsetMappingAnnotation,
                  annotation,
                  annotation->IsNodeAnnotation(),
                  annotation.IsNodeAnnotation());

// TODO(xiaochengh): Introduce an |NGIndexAnnotation|.

}  // namespace blink

#endif  // NGOffsetMappingAnnotation_h
