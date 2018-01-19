// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/LayoutBoxContainer.h"
#include "core/layout/LayoutBlock.h"
#include "core/layout/LayoutBox.h"
#include "core/layout/LayoutObject.h"
#include "core/paint/ObjectPaintInvalidator.h"
#include "core/paint/PaintLayer.h"
#include "platform/runtime_enabled_features.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/Vector.h"

namespace blink {
namespace {

using DescendantsMap =
    WTF::HashMap<const LayoutObject*,
                 std::unique_ptr<LayoutBoxContainer::LayoutBoxList>>;
using ContainerMap = WTF::HashMap<const LayoutBox*, LayoutObject*>;

// This map keeps track of the positioned objects associated with a container.
//
// This map is populated during layout. It is kept across layouts to handle
// that we skip unchanged sub-trees during layout, in such a way that we are
// able to lay out deeply nested out-of-flow descendants if their container got
// laid out. The map could be invalidated during style change but keeping track
// of containers at that time is complicated (we are in the middle of
// recomputing the style so we can't rely on any of its information), which is
// why it's easier to just update it for every layout.
DescendantsMap* g_positioned_descendants_map = nullptr;
ContainerMap* g_positioned_container_map = nullptr;

// This map keeps track of the descendants whose 'height' is percentage
// associated with a container. Like |gPositionedDescendantsMap|, it is
// also recomputed for every layout (see the comment above about why).
DescendantsMap* g_percent_height_descendants_map = nullptr;

}  // namespace

LayoutBoxContainer::LayoutBoxContainer()
    : has_positioned_objects_(false), has_percent_height_descendants_(false) {}

LayoutBoxContainer::~LayoutBoxContainer() {
  RemoveFromGlobalMaps();
}

// static
void LayoutBoxContainer::RemovePositionedObject(LayoutBox* o) {
  if (!g_positioned_container_map)
    return;

  LayoutObject* container = g_positioned_container_map->Take(o);
  if (!container)
    return;

  auto* positioned_descendants = g_positioned_descendants_map->at(container);
  DCHECK(positioned_descendants);
  DCHECK(positioned_descendants->Contains(o));
  positioned_descendants->erase(o);
  if (positioned_descendants->IsEmpty()) {
    g_positioned_descendants_map->erase(container);
    // TODO(vmpstr): This can change when the container is stored on the
    // LayoutObject.
    DCHECK(container->IsLayoutBlock());
    ToLayoutBlock(container)->has_positioned_objects_ = false;
  }

  // Need to clear the anchor of the positioned object in its container.
  // The anchors are created in the logical container, not in the CSS
  // container.
  if (LayoutObject* parent = o->Parent())
    parent->MarkContainerNeedsCollectInlines();
}

void LayoutBoxContainer::InsertPositionedObject(LayoutBox* o) {
  DCHECK(!AsLayoutObject()->IsAnonymousBlock());
  DCHECK_EQ(static_cast<LayoutObject*>(o->ContainingBlock()), AsLayoutObject());

  o->ClearContainingBlockOverrideSize();

  if (g_positioned_container_map) {
    auto container_map_it = g_positioned_container_map->find(o);
    if (container_map_it != g_positioned_container_map->end()) {
      if (container_map_it->value == AsLayoutObject()) {
        DCHECK(HasPositionedObjects());
        DCHECK(PositionedObjects()->Contains(o));
        return;
      }
      RemovePositionedObject(o);
    }
  } else {
    g_positioned_container_map = new ContainerMap;
  }
  g_positioned_container_map->Set(o, AsLayoutObject());

  if (!g_positioned_descendants_map)
    g_positioned_descendants_map = new DescendantsMap;
  auto* descendant_set = g_positioned_descendants_map->at(AsLayoutObject());
  if (!descendant_set) {
    descendant_set = new LayoutBoxList;
    g_positioned_descendants_map->Set(AsLayoutObject(),
                                      WTF::WrapUnique(descendant_set));
  }
  descendant_set->insert(o);

  has_positioned_objects_ = true;
}

void LayoutBoxContainer::RemovePositionedObjects(
    LayoutObject* o,
    ContainerState container_state) {
  auto* positioned_descendants = PositionedObjects();
  if (!positioned_descendants)
    return;

  Vector<LayoutBox*, 16> dead_objects;
  for (auto* positioned_object : *positioned_descendants) {
    if (!o ||
        (positioned_object->IsDescendantOf(o) && o != positioned_object)) {
      if (container_state == kNewContainer) {
        positioned_object->SetChildNeedsLayout(kMarkOnlyThis);

        // The positioned object changing containing block may change paint
        // invalidation container.
        // Invalidate it (including non-compositing descendants) on its original
        // paint invalidation container.
        if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
          // This valid because we need to invalidate based on the current
          // status.
          DisableCompositingQueryAsserts compositing_disabler;
          if (!positioned_object->IsPaintInvalidationContainer()) {
            ObjectPaintInvalidator(*positioned_object)
                .InvalidatePaintIncludingNonCompositingDescendants();
          }
        }
      }

      // It is parent blocks job to add positioned child to positioned objects
      // list of its containing block
      // Parent layout needs to be invalidated to ensure this happens.
      LayoutObject* p = positioned_object->Parent();
      while (p && !p->IsLayoutBlock())
        p = p->Parent();
      if (p)
        p->SetChildNeedsLayout();

      dead_objects.push_back(positioned_object);
    }
  }

  for (auto* object : dead_objects) {
    DCHECK_EQ(g_positioned_container_map->at(object), AsLayoutObject());
    positioned_descendants->erase(object);
    g_positioned_container_map->erase(object);
  }
  if (positioned_descendants->IsEmpty()) {
    g_positioned_descendants_map->erase(AsLayoutObject());
    has_positioned_objects_ = false;
  }
}

void LayoutBoxContainer::AddPercentHeightDescendant(LayoutBox* descendant) {
  if (descendant->PercentHeightContainer()) {
    if (descendant->PercentHeightContainer() == AsLayoutObject()) {
      DCHECK(HasPercentHeightDescendant(descendant));
      return;
    }
    descendant->RemoveFromPercentHeightContainer();
  }
  descendant->SetPercentHeightContainer(AsLayoutObject());

  if (!g_percent_height_descendants_map)
    g_percent_height_descendants_map = new DescendantsMap;
  auto* descendant_set = g_percent_height_descendants_map->at(AsLayoutObject());
  if (!descendant_set) {
    descendant_set = new LayoutBoxList;
    g_percent_height_descendants_map->Set(AsLayoutObject(),
                                          WTF::WrapUnique(descendant_set));
  }
  descendant_set->insert(descendant);

  has_percent_height_descendants_ = true;
}

void LayoutBoxContainer::RemovePercentHeightDescendant(LayoutBox* descendant) {
  if (auto* descendants = PercentHeightDescendants()) {
    descendants->erase(descendant);
    descendant->SetPercentHeightContainer(nullptr);
    if (descendants->IsEmpty()) {
      g_percent_height_descendants_map->erase(AsLayoutObject());
      has_percent_height_descendants_ = false;
    }
  }
}

#if DCHECK_IS_ON()
void LayoutBoxContainer::CheckPositionedObjectsNeedLayout() {
  if (!g_positioned_descendants_map)
    return;

  if (auto* positioned_descendant_set = PositionedObjects()) {
    for (auto* curr_box : *positioned_descendant_set)
      DCHECK(!curr_box->NeedsLayout());
  }
}
#endif

void LayoutBoxContainer::RemoveFromGlobalMaps() {
  if (HasPositionedObjects()) {
    auto descendants = g_positioned_descendants_map->Take(AsLayoutObject());
    DCHECK(!descendants->IsEmpty());
    for (LayoutBox* descendant : *descendants) {
      DCHECK_EQ(g_positioned_container_map->at(descendant), AsLayoutObject());
      g_positioned_container_map->erase(descendant);
    }
  }
  if (HasPercentHeightDescendants()) {
    auto descendants = g_percent_height_descendants_map->Take(AsLayoutObject());
    DCHECK(!descendants->IsEmpty());
    for (LayoutBox* descendant : *descendants) {
      DCHECK_EQ(descendant->PercentHeightContainer(), AsLayoutObject());
      descendant->SetPercentHeightContainer(nullptr);
    }
  }
}

LayoutBoxContainer::LayoutBoxList*
LayoutBoxContainer::PositionedObjectsInternal() const {
  return g_positioned_descendants_map
             ? g_positioned_descendants_map->at(AsLayoutObject())
             : nullptr;
}

LayoutBoxContainer::LayoutBoxList*
LayoutBoxContainer::PercentHeightDescendantsInternal() const {
  return g_percent_height_descendants_map
             ? g_percent_height_descendants_map->at(AsLayoutObject())
             : nullptr;
}

const LayoutObject* LayoutBoxContainer::AsLayoutObject() const {
  return static_cast<const LayoutBlock*>(this);
}

LayoutObject* LayoutBoxContainer::AsLayoutObject() {
  return static_cast<LayoutBlock*>(this);
}

}  // namespace blink
