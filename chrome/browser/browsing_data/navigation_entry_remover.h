// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_NAVIGATION_ENTRY_REMOVER_H_
#define CHROME_BROWSER_BROWSING_DATA_NAVIGATION_ENTRY_REMOVER_H_

#include "components/history/core/browser/url_row.h"

class Profile;

namespace browsing_data {

// Remove navigation entries from the tabs of |profile|.
void RemoveNavigationEntries(Profile* profile,
                             bool all_history,
                             const history::URLRows& deleted_rows);

}  // namespace browsing_data

#endif  // CHROME_BROWSER_BROWSING_DATA_NAVIGATION_ENTRY_REMOVER_H_
