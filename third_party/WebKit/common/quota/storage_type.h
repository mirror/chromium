// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_QUOTA_STORAGE_TYPE_H_
#define THIRD_PARTY_WEBKIT_COMMON_QUOTA_STORAGE_TYPE_H_

namespace blink {

enum StorageType {
  kStorageTypeTemporary,
  kStorageTypePersistent,
  kStorageTypeSyncable,
  kStorageTypeQuotaNotManaged,
  kStorageTypeUnknown,
  kStorageTypeLast = kStorageTypeUnknown
};

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_COMMON_QUOTA_STORAGE_TYPE_H_
