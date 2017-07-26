// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MEDIA_STORAGE_ID_SALT_H_
#define CHROME_BROWSER_MEDIA_MEDIA_STORAGE_ID_SALT_H_

#include <string>

#include "base/macros.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

// MediaStorageIDSalt is responsible for creating and retrieving a salt string
// that is used when creating Storage IDs.
class MediaStorageIdSalt {
 public:
  enum { kSaltLength = 32 };

  // Retrieves the current salt. If one does not currently exist it is created.
  static std::string GetSalt(PrefService* pref_service);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
  static void Reset(PrefService* pref_service);

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaStorageIdSalt);
};

#endif  // CHROME_BROWSER_MEDIA_MEDIA_STORAGE_ID_SALT_H_
