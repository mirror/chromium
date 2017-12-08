// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_QUOTA_MESSAGES_STRUCT_TRAITS_H_
#define THIRD_PARTY_WEBKIT_COMMON_QUOTA_MESSAGES_STRUCT_TRAITS_H_

#include "mojo/public/cpp/bindings/enum_traits.h"
#include "third_party/WebKit/common/quota/quota_status_code.h"
#include "third_party/WebKit/common/quota/quota_types.mojom.h"
#include "third_party/WebKit/common/quota/storage_type.h"

namespace mojo {

template <>
struct EnumTraits<blink::mojom::StorageType, blink::StorageType> {
  static blink::mojom::StorageType ToMojom(blink::StorageType storage_type) {
    switch (storage_type) {
      case blink::kStorageTypeTemporary:
        return blink::mojom::StorageType::kTemporary;
      case blink::kStorageTypePersistent:
        return blink::mojom::StorageType::kPersistent;
      case blink::kStorageTypeSyncable:
        return blink::mojom::StorageType::kSyncable;
      case blink::kStorageTypeQuotaNotManaged:
        return blink::mojom::StorageType::kQuotaNotManaged;
      case blink::kStorageTypeUnknown:
        return blink::mojom::StorageType::kUnknown;
    }
    NOTREACHED();
    return blink::mojom::StorageType::kUnknown;
  }

  static bool FromMojom(blink::mojom::StorageType storage_type,
                        blink::StorageType* out) {
    switch (storage_type) {
      case blink::mojom::StorageType::kTemporary:
        *out = blink::kStorageTypeTemporary;
        return true;
      case blink::mojom::StorageType::kPersistent:
        *out = blink::kStorageTypePersistent;
        return true;
      case blink::mojom::StorageType::kSyncable:
        *out = blink::kStorageTypeSyncable;
        return true;
      case blink::mojom::StorageType::kQuotaNotManaged:
        *out = blink::kStorageTypeQuotaNotManaged;
        return true;
      case blink::mojom::StorageType::kUnknown:
        *out = blink::kStorageTypeUnknown;
        return true;
    }
    NOTREACHED();
    *out = blink::kStorageTypeUnknown;
    return false;
  }
};

template <>
struct EnumTraits<blink::mojom::QuotaStatusCode, blink::QuotaStatusCode> {
  static blink::mojom::QuotaStatusCode ToMojom(
      blink::QuotaStatusCode status_code) {
    switch (status_code) {
      case blink::kQuotaStatusOk:
        return blink::mojom::QuotaStatusCode::kOk;
      case blink::kQuotaErrorNotSupported:
        return blink::mojom::QuotaStatusCode::kErrorNotSupported;
      case blink::kQuotaErrorInvalidModification:
        return blink::mojom::QuotaStatusCode::kErrorInvalidModification;
      case blink::kQuotaErrorInvalidAccess:
        return blink::mojom::QuotaStatusCode::kErrorInvalidAccess;
      case blink::kQuotaErrorAbort:
        return blink::mojom::QuotaStatusCode::kErrorAbort;
      case blink::kQuotaStatusUnknown:
        return blink::mojom::QuotaStatusCode::kUnknown;
    }
    NOTREACHED();
    return blink::mojom::QuotaStatusCode::kUnknown;
  }

  static bool FromMojom(blink::mojom::QuotaStatusCode status_code,
                        blink::QuotaStatusCode* out) {
    switch (status_code) {
      case blink::mojom::QuotaStatusCode::kOk:
        *out = blink::kQuotaStatusOk;
        return true;
      case blink::mojom::QuotaStatusCode::kErrorNotSupported:
        *out = blink::kQuotaErrorNotSupported;
        return true;
      case blink::mojom::QuotaStatusCode::kErrorInvalidModification:
        *out = blink::kQuotaErrorInvalidModification;
        return true;
      case blink::mojom::QuotaStatusCode::kErrorInvalidAccess:
        *out = blink::kQuotaErrorInvalidAccess;
        return true;
      case blink::mojom::QuotaStatusCode::kErrorAbort:
        *out = blink::kQuotaErrorAbort;
        return true;
      case blink::mojom::QuotaStatusCode::kUnknown:
        *out = blink::kQuotaStatusUnknown;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

}  // namespace mojo

#endif  // THIRD_PARTY_WEBKIT_COMMON_QUOTA_MESSAGES_STRUCT_TRAITS_H_
