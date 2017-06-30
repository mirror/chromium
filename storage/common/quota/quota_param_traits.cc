// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/common/quota/quota_param_traits.h"

namespace mojo {

// StorageType
static_assert(storage::StorageType::kStorageTypeTemporary ==
                  static_cast<storage::StorageType>(
                      quota::mojom::StorageType::kStorageTypeTemporary),
              "StorageType enums must match, kStorageTypeTemporary");

static_assert(storage::StorageType::kStorageTypePersistent ==
                  static_cast<storage::StorageType>(
                      quota::mojom::StorageType::kStorageTypePersistent),
              "StorageType enums must match, kStorageTypePersistent");

static_assert(storage::StorageType::kStorageTypeSyncable ==
                  static_cast<storage::StorageType>(
                      quota::mojom::StorageType::kStorageTypeSyncable),
              "StorageType enums must match, kStorageTypeSyncable");

static_assert(storage::StorageType::kStorageTypeQuotaNotManaged ==
                  static_cast<storage::StorageType>(
                      quota::mojom::StorageType::kStorageTypeQuotaNotManaged),
              "StorageType enums must match, kStorageTypeQuotaNotManaged");

static_assert(storage::StorageType::kStorageTypeUnknown ==
                  static_cast<storage::StorageType>(
                      quota::mojom::StorageType::kStorageTypeUnknown),
              "StorageType enums must match, kStorageTypeUnknown");

static_assert(storage::StorageType::kStorageTypeLast ==
                  static_cast<storage::StorageType>(
                      quota::mojom::StorageType::kStorageTypeLast),
              "StorageType enums must match, kStorageTypeLast");

// QuotaStatusCode
static_assert(storage::QuotaStatusCode::kQuotaStatusOk ==
                  static_cast<storage::QuotaStatusCode>(
                      quota::mojom::QuotaStatusCode::kQuotaStatusOk),
              "QuotaStatusCode enums must match, kQuotaStatusOk");

static_assert(storage::QuotaStatusCode::kQuotaErrorNotSupported ==
                  static_cast<storage::QuotaStatusCode>(
                      quota::mojom::QuotaStatusCode::kQuotaErrorNotSupported),
              "QuotaStatusCode enums must match, NETWORK");

static_assert(
    storage::QuotaStatusCode::kQuotaErrorInvalidModification ==
        static_cast<storage::QuotaStatusCode>(
            quota::mojom::QuotaStatusCode::kQuotaErrorInvalidModification),
    "QuotaStatusCode enums must match, kQuotaErrorInvalidModification");

static_assert(storage::QuotaStatusCode::kQuotaErrorInvalidAccess ==
                  static_cast<storage::QuotaStatusCode>(
                      quota::mojom::QuotaStatusCode::kQuotaErrorInvalidAccess),
              "QuotaStatusCode enums must match, kQuotaErrorInvalidAccess");

static_assert(storage::QuotaStatusCode::kQuotaErrorAbort ==
                  static_cast<storage::QuotaStatusCode>(
                      quota::mojom::QuotaStatusCode::kQuotaErrorAbort),
              "QuotaStatusCode enums must match, kQuotaErrorAbort");

static_assert(storage::QuotaStatusCode::kQuotaStatusUnknown ==
                  static_cast<storage::QuotaStatusCode>(
                      quota::mojom::QuotaStatusCode::kQuotaStatusUnknown),
              "QuotaStatusCode enums must match, kQuotaStatusUnknown");

static_assert(storage::QuotaStatusCode::kQuotaStatusLast ==
                  static_cast<storage::QuotaStatusCode>(
                      quota::mojom::QuotaStatusCode::kQuotaStatusLast),
              "QuotaStatusCode enums must match, kQuotaStatusLast");

// static
quota::mojom::StorageType
EnumTraits<quota::mojom::StorageType, storage::StorageType>::ToMojom(
    storage::StorageType input) {
  if (input >= storage::StorageType::kStoragetTypeTemporary &&
      input <= storage::StorageType::kStorageTypeLast) {
    return static_cast<quota::mojom::StorageType>(input);
  }

  NOTREACHED();
  return quota::mojom::StorageType::kStorageTypeLast;
}

// static
bool EnumTraits<quota::mojom::StorageType, storage::StorageType>::FromMojom(
    quota::mojom::StorageType input,
    storage::StorageType* output) {
  if (input >= quota::mojom::StorageType::kStorageTypeTemporary &&
      input <= quota::mojom::StorageType::kStorageTypeLast) {
    *output = static_cast<storage::StorageType>(input);
    return true;
  }

  NOTREACHED();
  return false;
}

// static
quota::mojom::QuotaStatusCode
EnumTraits<quota::mojom::QuotaStatusCode, storage::QuotaStatusCode>::ToMojom(
    storage::QuotaStatusCode input) {
  if (input >= storage::QuotaStatusCode::kQuotaStatusOk &&
      input <= storage::QuotaStatusCode::kQuotaStatusLast) {
    return static_cast<quota::mojom::QuotaStatusCode>(input);
  }

  NOTREACHED();
  return quota::mojom::QuotaStatusCode::kQuotaStatusLast;
}

// static
bool EnumTraits<quota::mojom::QuotaStatusCode, storage::QuotaStatusCode>::
    FromMojom(quota::mojom::QuotaStatusCode input,
              storage::QuotaStatusCode* output) {
  if (input >= quota::mojom::QuotaStatusCode::kQuotaStatusOk &&
      input <= quota::mojom::QuotaStatusCode::kQuotaStatusLast) {
    *output = static_cast<storage::QuotaStatusCode>(input);
    return true;
  }

  NOTREACHED();
  return false;
}

}  // namespace mojo
