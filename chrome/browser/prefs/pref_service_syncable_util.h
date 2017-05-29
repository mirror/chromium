// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFS_PREF_SERVICE_SYNCABLE_UTIL_H_
#define CHROME_BROWSER_PREFS_PREF_SERVICE_SYNCABLE_UTIL_H_

class Profile;

namespace sync_preferences {
class PrefServiceSyncable;
}

// sync_preferences::PrefServiceSyncable is a PrefService with added integration
// for sync, and knowledge of how to create an incognito PrefService. For code
// that does not need to know about the sync integration, you should use only
// the plain PrefService type.
//
// For this reason, Profile does not expose an accessor for the
// sync_preferences::PrefServiceSyncable type. Instead, you can use the
// utilities below to retrieve the sync_preferences::PrefServiceSyncable (or its
// incognito version) from a Profile.
sync_preferences::PrefServiceSyncable* PrefServiceSyncableFromProfile(
    Profile* profile);

#endif  // CHROME_BROWSER_PREFS_PREF_SERVICE_SYNCABLE_UTIL_H_
