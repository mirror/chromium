// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PaintPropertyNode_h
#define PaintPropertyNode_h

#include "platform/wtf/PassRefPtr.h"
#include "platform/wtf/RefCounted.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

template <typename T>
class PaintPropertyNode : public RefCounted<T> {
 public:
  // Parent property node, or nullptr if this is the root node.
  const T* Parent() const { return parent_.Get(); }
  bool IsRoot() const { return !parent_; }

  // Rules of checking and setting the changed status:
  // 1 Before PrePaint, all existing paint property nodes should be in kUnknown
  //   or kChanged state. No nodes could be in kUnchanged state.
  // 2 During PrePaint, PaintPropertyTreeBuilder creates or updates paint
  //   property nodes.
  //   - Any new node created during the tree walk is set kUnknown state.
  //   - If any field of an existing node is set to a different value, the
  //     node's status will be set to kChanged.
  //   - Otherwise the statuses will remain kUnknown.
  // 3 During compositing update, PaintArtifactCompositor checks the status of
  //   paint property nodes to issue raster invalidations if needed. If a node
  //   is in kUnknown state, it will query its parent's status recursively and
  //   update its status to kChanged or kUnchanged, or update to kUnchanged if
  //   the node is a root node. This is the only stage that we can see nodes in
  //   kUnchanged state.
  // 4 After compositing update, PaintArtifactCompositor will call
  //   ResetChangedStatus() for all property nodes referenced by paint chunks to
  //   reset the changed statuses to kUnknown. The nodes that are not referenced
  //   by paint chunks thus not reached by PaintArtifactCompositor will remain
  //   in kUnknown or kChanged states.

  // This is called by PaintArtifactCompositor only. See 3 above.
  bool Changed() const {
    if (changed_status_ != kUnknown)
      return changed_status_ == kChanged;

    if (parent_ && parent_->Changed()) {
      changed_status_ = kChanged;
      return true;
    }

    if (static_cast<const T*>(this)->ReferencedNodeChanged()) {
      changed_status_ = kChanged;
      return true;
    }

    changed_status_ = kUnchanged;
    return false;
  }

  // This is called by PaintArtifactCompositor only. See 4 above.
  void ResetChangedStatus() { changed_status_ = kUnknown; }

 protected:
  PaintPropertyNode(PassRefPtr<const T> parent)
      : parent_(std::move(parent)), changed_status_(kUnknown) {}

  void Update(PassRefPtr<const T> parent) {
    DCHECK(!IsRoot());
    DCHECK(parent != this);
    DCHECK(changed_status_ != kUnchanged);
    changed_status_ = kChanged;
    parent_ = std::move(parent);
  }

 private:
  RefPtr<const T> parent_;

  enum ChangedStatus { kUnknown, kChanged, kUnchanged };
  mutable ChangedStatus changed_status_;
};

}  // namespace blink

#endif  // PaintPropertyNode_h
