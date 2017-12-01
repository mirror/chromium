// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_TEST_SCOPED_BLOCK_POPUPS_PREF_H_
#define IOS_CHROME_TEST_SCOPED_BLOCK_POPUPS_PREF_H_

#include "base/macros.h"
#include "components/content_settings/core/common/content_settings.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

// ScopedBlockPopupsPref modifies the block popups preference
// and resets the preference to its original value when this object goes out of
// scope.
class ScopedBlockPopupsPref {
 public:
  explicit ScopedBlockPopupsPref(ContentSetting setting,
                                 ios::ChromeBrowserState* browser_state);
  ~ScopedBlockPopupsPref();

 private:
  // Gets the current value of the preference.
  ContentSetting GetPrefValue();

  // Sets the preference to the given value.
  void SetPrefValue(ContentSetting setting);

  // Saves the original pref setting so that it can be restored when the scoper
  // is destroyed.
  ContentSetting original_setting_;

  ios::ChromeBrowserState* browser_state_;

  DISALLOW_COPY_AND_ASSIGN(ScopedBlockPopupsPref);
};

#endif  // IOS_CHROME_TEST_SCOPED_BLOCK_POPUPS_PREF_H_
