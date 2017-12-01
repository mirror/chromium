// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_NAVIGATION_ENTRY_REMOVER_H_
#define CHROME_BROWSER_BROWSING_DATA_NAVIGATION_ENTRY_REMOVER_H_

class Profile;

namespace browsing_data {

// Remove navigation entries except the active one from the tabs of |profile|.
void RemoveNavigationEntries(Profile* profile);

}  // namespace browsing_data

#endif  // CHROME_BROWSER_BROWSING_DATA_NAVIGATION_ENTRY_REMOVER_H_
