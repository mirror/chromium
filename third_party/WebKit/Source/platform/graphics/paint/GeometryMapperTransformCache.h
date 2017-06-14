// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GeometryMapperTransformCache_h
#define GeometryMapperTransformCache_h

#include "platform/PlatformExport.h"

#include "platform/transforms/TransformationMatrix.h"
#include "platform/wtf/Vector.h"

namespace blink {

class TransformPaintPropertyNode;

// A GeometryMapperTransformCache hangs off a TransformPaintPropertyNode.
// It stores useful intermediate results such as screen matrix for geometry
// queries.
class PLATFORM_EXPORT GeometryMapperTransformCache {
  USING_FAST_MALLOC(GeometryMapperTransformCache);
 public:
  GeometryMapperTransformCache() = default;

  static void ClearCache();

  void UpdateIfNeeded(const TransformPaintPropertyNode& node) {
    if (cache_generation_ != s_global_generation)
      Update(node);
    DCHECK_EQ(cache_generation_, s_global_generation);
  }

  const TransformationMatrix& to_screen() const { return to_screen_; }
  bool to_screen_is_invertible() const { return to_screen_is_invertible_; }

  const TransformationMatrix& projection_from_screen() const {
    return projection_from_screen_;
  }
  bool projection_from_screen_is_valid() const {
    return projection_from_screen_is_valid_;
  }

  const TransformationMatrix& to_checkpoint() const { return to_checkpoint_; }
  const TransformationMatrix& from_checkpoint() const {
    return from_checkpoint_;
  }
  const TransformPaintPropertyNode* checkpoint() const { return checkpoint_; }

 private:
  void Update(const TransformPaintPropertyNode&);

  static unsigned s_global_generation;

  TransformationMatrix to_screen_;
  TransformationMatrix projection_from_screen_;
  TransformationMatrix to_checkpoint_;
  TransformationMatrix from_checkpoint_;
  const TransformPaintPropertyNode* checkpoint_ = nullptr;
  unsigned cache_generation_ = s_global_generation - 1;
  unsigned to_screen_is_invertible_ : 1;
  unsigned projection_from_screen_is_valid_ : 1;
  DISALLOW_COPY_AND_ASSIGN(GeometryMapperTransformCache);
};

}  // namespace blink

#endif  // GeometryMapperTransformCache_h
