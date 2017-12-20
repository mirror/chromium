// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DATA_SAVER_DATA_SAVER_UTIL_H_
#define CHROME_BROWSER_DATA_SAVER_DATA_SAVER_UTIL_H_

class GURL;
class ProfileIOData;

namespace chrome {

// Synchronously returns true if the request to |gurl| would likely go
// through the data saver proxy if loaded by the profile corresponding to
// |profile_io_data|. Should be called on the IO thread.
bool WouldLikelyBeFetchedViaDataSaverIO(ProfileIOData* profile_io_data,
                                        const GURL& gurl);

}  // namespace chrome

#endif  // CHROME_BROWSER_DATA_SAVER_DATA_SAVER_UTIL_H_
