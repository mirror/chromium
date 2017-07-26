// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_STORAGE_ID_H_
#define CHROME_BROWSER_MEDIA_STORAGE_ID_H_

#include <string.h>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "url/origin.h"

// This handles computing the Storage Id for platform verification.
class StorageId {
 public:
  using StorageIdCallback =
      base::OnceCallback<void(const std::string& storage_id)>;

  // Compute the Storage Id based on |salt| and |origin|. This may be
  // asynchronous, so call |callback| with the result. If Storage Id is not
  // supported on the current platform, an empty string will be passed to
  // |callback|.
  static void ComputeStorageId(const std::string& salt,
                               const url::Origin& origin,
                               StorageIdCallback callback);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StorageId);
};

#endif  // CHROME_BROWSER_MEDIA_STORAGE_ID_H_
