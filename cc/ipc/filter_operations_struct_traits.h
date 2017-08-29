// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_IPC_FILTER_OPERATIONS_STRUCT_TRAITS_H_
#define CC_IPC_FILTER_OPERATIONS_STRUCT_TRAITS_H_

#include "cc/ipc/filter_operations.mojom-shared.h"
#include "ui/gfx/filter_operations.h"

namespace mojo {

template <>
struct StructTraits<cc::mojom::FilterOperationsDataView,
                    gfx::FilterOperations> {
  static const std::vector<gfx::FilterOperation>& operations(
      const gfx::FilterOperations& operations) {
    return operations.operations();
  }

  static bool Read(cc::mojom::FilterOperationsDataView data,
                   gfx::FilterOperations* out) {
    std::vector<gfx::FilterOperation> operations;
    if (!data.ReadOperations(&operations))
      return false;
    *out = gfx::FilterOperations(std::move(operations));
    return true;
  }
};

}  // namespace mojo

#endif  // CC_IPC_FILTER_OPERATIONS_STRUCT_TRAITS_H_
