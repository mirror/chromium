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

  const TransformationMatrix& to_plane_root() const { return to_plane_root_; }
  const TransformationMatrix& from_plane_root() const {
    return from_plane_root_;
  }
  const TransformPaintPropertyNode* plane_root() const { return plane_root_; }

 private:
  void Update(const TransformPaintPropertyNode&);

  static unsigned s_global_generation;

  // The screen matrix of this node, as defined by:
  // to_screen = (flattens_inherited_transform ?
  //     flatten(parent.to_screen) : parent.to_screen) * local
  TransformationMatrix to_screen_;
  // Back projection from screen, if inversion exists:
  // projection_from_screen = flatten(to_screen)^-1
  // Is guaranteed to be flat.
  TransformationMatrix projection_from_screen_;
  // The accumulated matrix between this node and plane_root.
  // flatten(to_screen) = flatten(plane_root.to_screen) * to_plane_root
  // Is guaranteed to be flat.
  TransformationMatrix to_plane_root_;
  // from_plane_root = to_plane_root^-1
  // Is guaranteed to be flat.
  TransformationMatrix from_plane_root_;
  // The oldest ancestor node that every nodes in the ancestor chain are
  // coplanar to, and have invertible transform.
  // i.e. For all nodes n in the ancestor chain [this, plane_root],
  // n.local is flat and invertible.
  const TransformPaintPropertyNode* plane_root_ = nullptr;
  unsigned cache_generation_ = s_global_generation - 1;
  // Whether to_screen^-1 is valid.
  unsigned to_screen_is_invertible_ : 1;
  // Whether flatten(to_screen)^-1 is valid.
  unsigned projection_from_screen_is_valid_ : 1;
  DISALLOW_COPY_AND_ASSIGN(GeometryMapperTransformCache);
};

}  // namespace blink

#endif  // GeometryMapperTransformCache_h
