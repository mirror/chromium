// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutNGData_h
#define LayoutNGData_h

#include "core/CoreExport.h"
#include "core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

class NGBreakToken;
class NGConstraintSpace;
struct NGInlineNodeData;
class NGLayoutResult;

class CORE_EXPORT LayoutNGData {
 public:
  NGInlineNodeData* GetNGInlineNodeData() const;
  void ResetNGInlineNodeData();
  bool HasNGInlineNodeData() const { return ng_inline_node_data_.get(); }

  // Returns the last layout result for this block flow with the given
  // constraint space and break token, or null if it is not up-to-date or
  // otherwise unavailable.
  RefPtr<NGLayoutResult> CachedLayoutResult(LayoutBox* box,
                                            const NGConstraintSpace&,
                                            NGBreakToken*) const;

  void SetCachedLayoutResult(const NGConstraintSpace&,
                             NGBreakToken*,
                             RefPtr<NGLayoutResult>);

 private:
  std::unique_ptr<NGInlineNodeData> ng_inline_node_data_;

  RefPtr<NGLayoutResult> cached_result_;
  RefPtr<const NGConstraintSpace> cached_constraint_space_;
};

}  // namespace blink

#endif  // LayoutNGData_h
