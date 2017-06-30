// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_COMMON_QUOTA_QUOTA_PARAM_TRAITS_H_
#define STORAGE_COMMON_QUOTA_QUOTA_PARAM_TRAITS_H_

#include <stddef.h>

#include "mojo/public/cpp/bindings/struct_traits.h"
#include "storage/public/interfaces/quota.mojom.h"

namespace mojo {

template <>
struct EnumTraits<quota::mojom::StorageType, storage::StorageType> {
  static quota::mojom::StorageType ToMojom(storage::StorageType input);
  static bool FromMojom(quota::mojom::StorageType input,
                        storage::StorageType* output);
};

template <>
struct EnumTraits<quota::mojom::QuotaStatusCode, storage::QuotaStatusCode> {
  static quota::mojom::QuotaStatusCode ToMojom(storage::QuotaStatusCode input);
  static bool FromMojom(quota::mojom::QuotaStatusCode input,
                        storage::QuotaStatusCode* output);
};

}  // namespace mojo

#endif  // STORAGE_COMMON_QUOTA_QUOTA_PARAM_TRAITS_H_
