// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SelectionInUndoStep_h
#define SelectionInUndoStep_h

#include "base/macros.h"
#include "core/editing/Position.h"
#include "core/editing/SelectionTemplate.h"
#include "core/editing/TextAffinity.h"
#include "core/editing/VisiblePosition.h"
#include "core/editing/VisibleSelection.h"
#include "platform/heap/Handle.h"

namespace blink {

class Element;

// This class represents selection in |UndoStep|.
class SelectionInUndoStep final {
  DISALLOW_NEW();

 public:
  static SelectionInUndoStep From(const SelectionInDOMTree&);

  SelectionInUndoStep(const SelectionInUndoStep&);
  SelectionInUndoStep();

  SelectionInUndoStep& operator=(const SelectionInUndoStep&);

  bool operator==(const SelectionInUndoStep&) const;
  bool operator!=(const SelectionInUndoStep&) const;

  TextAffinity Affinity() const { return affinity_; }
  SelectionInDOMTree AsSelection() const;
  Position Base() const { return base_; }
  Position Extent() const { return extent_; }
  bool IsBaseFirst() const;
  bool IsDirectional() const { return is_directional_; }

  // TODO(editing-dev): We should rename to |ComputeStart()|.
  Position Start() const;
  // TODO(editing-dev): We should rename to |ComputeEnd()|.
  Position End() const;

  // TODO(editing-dev): We should move following functions out side
  // |SelectionInUndoStep| class.
  bool IsCaret() const;
  bool IsNone() const;
  bool IsRange() const;
  bool IsNonOrphanedCaretOrRange() const;
  bool IsValidFor(const Document&) const;
  Element* RootEditableElement() const;
  VisiblePosition VisibleStart() const;
  VisiblePosition VisibleEnd() const;

  DECLARE_TRACE();

 private:
  void EnsureStartAndEnd() const;

  // |base_| and |extent_| can be disconnected from document.
  Position base_;
  Position extent_;
  TextAffinity affinity_ = TextAffinity::kDownstream;
  mutable bool did_compute_is_base_first_ = false;
  mutable bool is_base_first_ = false;
  bool is_directional_ = false;
};

VisibleSelection CreateVisibleSelection(const SelectionInUndoStep&);

}  // namespace blink

#endif  // SelectionInUndoStep_h
