// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutBoxContainer_h
#define LayoutBoxContainer_h

#include "core/CoreExport.h"
#include "platform/wtf/Assertions.h"
#include "platform/wtf/ListHashSet.h"

namespace blink {

class LayoutBox;
class LayoutObject;

class CORE_EXPORT LayoutBoxContainer {
 private:
  // This object can only be subclassed by LayoutBlock.
  friend class LayoutBlock;
  friend struct SameSizeAsLayoutBlock;

  LayoutBoxContainer();
  virtual ~LayoutBoxContainer() = 0;

 public:
  using LayoutBoxList = WTF::ListHashSet<LayoutBox*, 16>;

  enum ContainerState { kNewContainer, kSameContainer };

  static void RemovePositionedObject(LayoutBox*);

  void InsertPositionedObject(LayoutBox*);
  void RemovePositionedObjects(LayoutObject*, ContainerState = kSameContainer);
  bool HasPositionedObjects() const {
    DCHECK(has_positioned_objects_ ? (PositionedObjectsInternal() &&
                                      !PositionedObjectsInternal()->IsEmpty())
                                   : !PositionedObjectsInternal());
    return has_positioned_objects_;
  }
  LayoutBoxList* PositionedObjects() const {
    return HasPositionedObjects() ? PositionedObjectsInternal() : nullptr;
  }

  void AddPercentHeightDescendant(LayoutBox*);
  void RemovePercentHeightDescendant(LayoutBox*);
  bool HasPercentHeightDescendants() const {
    DCHECK(has_percent_height_descendants_
               ? (PercentHeightDescendantsInternal() &&
                  !PercentHeightDescendantsInternal()->IsEmpty())
               : !PercentHeightDescendantsInternal());
    return has_percent_height_descendants_;
  }
  bool HasPercentHeightDescendant(LayoutBox* o) const {
    return HasPercentHeightDescendants() &
           PercentHeightDescendantsInternal()->Contains(o);
  }
  LayoutBoxList* PercentHeightDescendants() const {
    return HasPercentHeightDescendants() ? PercentHeightDescendantsInternal()
                                         : nullptr;
  }

#if DCHECK_IS_ON()
  void CheckPositionedObjectsNeedLayout();
#endif

 private:
  void RemoveFromGlobalMaps();

  LayoutBoxList* PositionedObjectsInternal() const;
  LayoutBoxList* PercentHeightDescendantsInternal() const;
  const LayoutObject* AsLayoutObject() const;
  LayoutObject* AsLayoutObject();

  bool has_positioned_objects_ : 1;
  bool has_percent_height_descendants_ : 1;
};

}  // namespace blink

#endif  // LayoutBoxContainer_h
