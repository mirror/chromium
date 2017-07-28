// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGColumnLayoutAlgorithm_h
#define NGColumnLayoutAlgorithm_h

#include "core/layout/ng/ng_block_layout_algorithm.h"
#include "core/layout/ng/ng_break_token.h"
#include "core/layout/ng/ng_constraint_space.h"

namespace blink {

class NGBlockNode;

class CORE_EXPORT NGColumnLayoutAlgorithm : public NGBlockLayoutAlgorithm {
 public:
  NGColumnLayoutAlgorithm(NGBlockNode node,
                          NGConstraintSpace* space,
                          NGBreakToken* break_token = nullptr);

 protected:
  bool LayoutInterior(NGPreviousInflowPosition*) override;

 private:
  RefPtr<NGConstraintSpace> CreateConstraintSpaceForColumns() const;
};

}  // namespace Blink

#endif  // NGColumnLayoutAlgorithm_h
