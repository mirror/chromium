// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_TEST_APP_IOS_CHROME_SCOPED_TESTING_BLOCK_POPUPS_PREF_H_
#define IOS_CHROME_TEST_APP_IOS_CHROME_SCOPED_TESTING_BLOCK_POPUPS_PREF_H_

#include "base/macros.h"
#include "components/content_settings/core/common/content_settings.h"

// IOSChromeScopedTestingBlockPopupsPref modifies the block popups preference
// and resets the preference to its original value when this object goes out of
// scope.
class IOSChromeScopedTestingBlockPopupsPref {
 public:
  explicit IOSChromeScopedTestingBlockPopupsPref(ContentSetting setting);
  ~IOSChromeScopedTestingBlockPopupsPref();

 private:
  // Gets the current value of the preference.
  ContentSetting GetPrefValue();

  // Sets the preference to the given value.
  void SetPrefValue(ContentSetting setting);

  // Saves the original pref setting so that it can be restored when the scoper
  // is destroyed.
  ContentSetting original_setting_;

  DISALLOW_COPY_AND_ASSIGN(IOSChromeScopedTestingBlockPopupsPref);
};

#endif  // IOS_CHROME_TEST_APP_IOS_CHROME_SCOPED_TESTING_BLOCK_POPUPS_PREF_H_
