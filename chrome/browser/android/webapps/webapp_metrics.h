// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_WEBAPPS_WEBAPP_METRICS_H_
#define CHROME_BROWSER_ANDROID_WEBAPPS_WEBAPP_METRICS_H_

namespace webapp {

// The enum backs a UMA histogram and must be treated as append only.
enum class AddToHomescreenType {
  // Legacy PWA shortcut. Opens in full screen.
  STANDALONE = 0,

  // Bookmark type shortcut which launches the tabbed browser.
  SHORTCUT = 1,

  COUNT = 2
};

// Records the type of homescreen shortcut added when a user taps "Add to Home
// screen" in the app menu.
void RecordAddToHomescreenFromAppMenuType(AddToHomescreenType type);

}  // namespace webapp

#endif  // CHROME_BROWSER_ANDROID_WEBAPPS_WEBAPP_METRICS_H_
