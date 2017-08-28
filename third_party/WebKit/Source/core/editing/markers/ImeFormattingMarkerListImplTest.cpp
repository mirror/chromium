// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/markers/ImeFormattingMarkerListImpl.h"

#include "core/editing/EditingTestBase.h"
#include "core/editing/markers/ImeFormattingMarker.h"

namespace blink {

class ImeFormattingMarkerListImplTest : public EditingTestBase {
 protected:
  ImeFormattingMarkerListImplTest()
      : marker_list_(new ImeFormattingMarkerListImpl()) {}

  DocumentMarker* CreateMarker(unsigned start_offset, unsigned end_offset) {
    return new ImeFormattingMarker(start_offset, end_offset, Color::kBlack,
                                   StyleableMarker::Thickness::kThin,
                                   Color::kBlack);
  }

  Persistent<ImeFormattingMarkerListImpl> marker_list_;
};

// ImeFormattingMarkerListImpl shouldn't merge markers with touching endpoints
TEST_F(ImeFormattingMarkerListImplTest, Add) {
  EXPECT_EQ(0u, marker_list_->GetMarkers().size());

  marker_list_->Add(CreateMarker(0, 1));
  marker_list_->Add(CreateMarker(1, 2));

  EXPECT_EQ(2u, marker_list_->GetMarkers().size());

  EXPECT_EQ(0u, marker_list_->GetMarkers()[0]->StartOffset());
  EXPECT_EQ(1u, marker_list_->GetMarkers()[0]->EndOffset());

  EXPECT_EQ(1u, marker_list_->GetMarkers()[1]->StartOffset());
  EXPECT_EQ(2u, marker_list_->GetMarkers()[1]->EndOffset());
}

}  // namespace blink
