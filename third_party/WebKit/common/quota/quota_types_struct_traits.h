// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_QUOTA_QUOTA_TYPES_STRUCT_TRAITS_H_
#define THIRD_PARTY_WEBKIT_COMMON_QUOTA_QUOTA_TYPES_STRUCT_TRAITS_H_

#include "mojo/public/cpp/bindings/enum_traits.h"
#include "third_party/WebKit/common/quota/quota_types.mojom.h"

namespace mojo {

template <>
struct EnumTraits<blink::mojom::StorageType, blink::mojom::StorageType> {
  static blink::mojom::StorageType ToMojom(
      blink::mojom::StorageType storage_type) {
    switch (storage_type) {
      case blink::mojom::StorageType::kTemporary:
        return blink::mojom::StorageType::kTemporary;
      case blink::mojom::StorageType::kPersistent:
        return blink::mojom::StorageType::kPersistent;
      case blink::mojom::StorageType::kSyncable:
        return blink::mojom::StorageType::kSyncable;
      case blink::mojom::StorageType::kQuotaNotManaged:
        return blink::mojom::StorageType::kQuotaNotManaged;
      case blink::mojom::StorageType::kUnknown:
        return blink::mojom::StorageType::kUnknown;
    }
    NOTREACHED();
    return blink::mojom::StorageType::kUnknown;
  }

  static bool FromMojom(blink::mojom::StorageType storage_type,
                        blink::mojom::StorageType* out) {
    switch (storage_type) {
      case blink::mojom::StorageType::kTemporary:
        *out = blink::mojom::StorageType::kTemporary;
        return true;
      case blink::mojom::StorageType::kPersistent:
        *out = blink::mojom::StorageType::kPersistent;
        return true;
      case blink::mojom::StorageType::kSyncable:
        *out = blink::mojom::StorageType::kSyncable;
        return true;
      case blink::mojom::StorageType::kQuotaNotManaged:
        *out = blink::mojom::StorageType::kQuotaNotManaged;
        return true;
      case blink::mojom::StorageType::kUnknown:
        *out = blink::mojom::StorageType::kUnknown;
        return true;
    }
    NOTREACHED();
    *out = blink::mojom::StorageType::kUnknown;
    return false;
  }
};

template <>
struct EnumTraits<blink::mojom::QuotaStatusCode,
                  blink::mojom::QuotaStatusCode> {
  static blink::mojom::QuotaStatusCode ToMojom(
      blink::mojom::QuotaStatusCode status_code) {
    switch (status_code) {
      case blink::mojom::QuotaStatusCode::kOk:
        return blink::mojom::QuotaStatusCode::kOk;
      case blink::mojom::QuotaStatusCode::kErrorNotSupported:
        return blink::mojom::QuotaStatusCode::kErrorNotSupported;
      case blink::mojom::QuotaStatusCode::kErrorInvalidModification:
        return blink::mojom::QuotaStatusCode::kErrorInvalidModification;
      case blink::mojom::QuotaStatusCode::kErrorInvalidAccess:
        return blink::mojom::QuotaStatusCode::kErrorInvalidAccess;
      case blink::mojom::QuotaStatusCode::kErrorAbort:
        return blink::mojom::QuotaStatusCode::kErrorAbort;
      case blink::mojom::QuotaStatusCode::kUnknown:
        return blink::mojom::QuotaStatusCode::kUnknown;
    }
    NOTREACHED();
    return blink::mojom::QuotaStatusCode::kUnknown;
  }

  static bool FromMojom(blink::mojom::QuotaStatusCode status_code,
                        blink::mojom::QuotaStatusCode* out) {
    switch (status_code) {
      case blink::mojom::QuotaStatusCode::kOk:
        *out = blink::mojom::QuotaStatusCode::kOk;
        return true;
      case blink::mojom::QuotaStatusCode::kErrorNotSupported:
        *out = blink::mojom::QuotaStatusCode::kErrorNotSupported;
        return true;
      case blink::mojom::QuotaStatusCode::kErrorInvalidModification:
        *out = blink::mojom::QuotaStatusCode::kErrorInvalidModification;
        return true;
      case blink::mojom::QuotaStatusCode::kErrorInvalidAccess:
        *out = blink::mojom::QuotaStatusCode::kErrorInvalidAccess;
        return true;
      case blink::mojom::QuotaStatusCode::kErrorAbort:
        *out = blink::mojom::QuotaStatusCode::kErrorAbort;
        return true;
      case blink::mojom::QuotaStatusCode::kUnknown:
        *out = blink::mojom::QuotaStatusCode::kUnknown;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

}  // namespace mojo

#endif  // THIRD_PARTY_WEBKIT_COMMON_QUOTA_QUOTA_TYPES_STRUCT_TRAITS_H_
