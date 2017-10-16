// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_FIND_BAR_DECORATION_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_FIND_BAR_DECORATION_H_

#import <Cocoa/Cocoa.h>

#include "base/macros.h"
#include "chrome/browser/ui/cocoa/location_bar/location_bar_decoration.h"

// Find bar icon on the right side of the field just before the star decoration.

class FindBarDecoration : public LocationBarDecoration {
 public:
  explicit FindBarDecoration();
  ~FindBarDecoration() override;

  // Implement |LocationBarDecoration|.
  bool AcceptsMousePress() override;
  NSString* GetToolTip() override;

 protected:
  // Overridden from LocationBarDecoration:
  SkColor GetMaterialIconColor(bool location_bar_is_dark) const override;
  const gfx::VectorIcon* GetMaterialVectorIcon() const override;

 private:
  // The string to show for a tooltip.
  base::scoped_nsobject<NSString> tooltip_;

  DISALLOW_COPY_AND_ASSIGN(FindBarDecoration);
};

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_FIND_BAR_DECORATION_H_
