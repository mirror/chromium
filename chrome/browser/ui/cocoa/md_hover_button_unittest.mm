// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/md_hover_button.h"

#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"

namespace {

class MDHoverButtonTest : public ui::CocoaTest {
 public:
  MDHoverButtonTest() {
    base::scoped_nsobject<MDHoverButton> button(
        [[MDHoverButton alloc] initWithFrame:NSMakeRect(0, 0, 20, 20)]);
    button_ = button;
    [[test_window() contentView] addSubview:button_];
  }

 protected:
  MDHoverButton* button_;  // Weak, owned by test_window().
};

TEST_VIEW(MDHoverButtonTest, button_)

}  // namespace
