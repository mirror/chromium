// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_QUOTA_MESSAGES_STRUCT_TRAITS_H_
#define CONTENT_COMMON_QUOTA_MESSAGES_STRUCT_TRAITS_H_

#include "content/common/quota_messages.mojom.h"
#include "mojo/public/cpp/bindings/enum_traits.h"
#include "storage/common/quota/quota_status_code.h"
#include "storage/common/quota/quota_types.h"

namespace mojo {

template <>
struct EnumTraits<content::mojom::StorageType, storage::StorageType> {
  static content::mojom::StorageType ToMojom(
      storage::StorageType storage_type) {
    switch (storage_type) {
      case storage::kStorageTypeTemporary:
        return content::mojom::StorageType::Temporary;
      case storage::kStorageTypePersistent:
        return content::mojom::StorageType::Persistent;
      case storage::kStorageTypeSyncable:
        return content::mojom::StorageType::Syncable;
      case storage::kStorageTypeQuotaNotManaged:
        return content::mojom::StorageType::QuotaNotManaged;
      case storage::kStorageTypeUnknown:
        return content::mojom::StorageType::Unknown;
    }
    NOTREACHED();
    return content::mojom::StorageType::Unknown;
  }

  static bool FromMojom(content::mojom::StorageType storage_type,
                        storage::StorageType* out) {
    switch (storage_type) {
      case content::mojom::StorageType::Temporary:
        *out = storage::kStorageTypeTemporary;
        return true;
      case content::mojom::StorageType::Persistent:
        *out = storage::kStorageTypePersistent;
        return true;
      case content::mojom::StorageType::Syncable:
        *out = storage::kStorageTypeSyncable;
        return true;
      case content::mojom::StorageType::QuotaNotManaged:
        *out = storage::kStorageTypeQuotaNotManaged;
        return true;
      case content::mojom::StorageType::Unknown:
        *out = storage::kStorageTypeUnknown;
        return true;
    }
    NOTREACHED();
    *out = storage::kStorageTypeUnknown;
    return false;
  }
};

template <>
struct EnumTraits<content::mojom::QuotaStatusCode, storage::QuotaStatusCode> {
  static content::mojom::QuotaStatusCode ToMojom(
      storage::QuotaStatusCode status_code) {
    switch (status_code) {
      case storage::kQuotaStatusOk:
        return content::mojom::QuotaStatusCode::Ok;
      case storage::kQuotaErrorNotSupported:
        return content::mojom::QuotaStatusCode::ErrorNotSupported;
      case storage::kQuotaErrorInvalidModification:
        return content::mojom::QuotaStatusCode::ErrorInvalidModification;
      case storage::kQuotaErrorInvalidAccess:
        return content::mojom::QuotaStatusCode::ErrorInvalidAccess;
      case storage::kQuotaErrorAbort:
        return content::mojom::QuotaStatusCode::ErrorAbort;
      case storage::kQuotaStatusUnknown:
        return content::mojom::QuotaStatusCode::Unknown;
    }
    NOTREACHED();
    return content::mojom::QuotaStatusCode::Unknown;
  }

  static bool FromMojom(content::mojom::QuotaStatusCode status_code,
                        storage::QuotaStatusCode* out) {
    switch (status_code) {
      case content::mojom::QuotaStatusCode::Ok:
        *out = storage::kQuotaStatusOk;
        return true;
      case content::mojom::QuotaStatusCode::ErrorNotSupported:
        *out = storage::kQuotaErrorNotSupported;
        return true;
      case content::mojom::QuotaStatusCode::ErrorInvalidModification:
        *out = storage::kQuotaErrorInvalidModification;
        return true;
      case content::mojom::QuotaStatusCode::ErrorInvalidAccess:
        *out = storage::kQuotaErrorInvalidAccess;
        return true;
      case content::mojom::QuotaStatusCode::ErrorAbort:
        *out = storage::kQuotaErrorAbort;
        return true;
      case content::mojom::QuotaStatusCode::Unknown:
        *out = storage::kQuotaStatusUnknown;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

}  // namespace mojo

#endif  // CONTENT_COMMON_QUOTA_MESSAGES_STRUCT_TRAITS_H_
