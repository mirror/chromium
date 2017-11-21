// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_MOJO_SYNCER_STRUCT_TRAITS_H_
#define COMPONENTS_SYNC_MOJO_SYNCER_STRUCT_TRAITS_H_

#include <string>

#include "components/sync/model/string_ordinal.h"
#include "components/sync/mojo/syncer.mojom-shared.h"

namespace mojo {

template <>
struct StructTraits<syncer::mojom::StringOrdinalDataView,
                    syncer::StringOrdinal> {
  static const std::string bytes(const syncer::StringOrdinal& ordinal) {
    return ordinal.IsValid() ? ordinal.ToInternalValue() : "";
  }
  static bool Read(syncer::mojom::StringOrdinalDataView data,
                   syncer::StringOrdinal* out);
};

}  // namespace mojo

#endif  // COMPONENTS_SYNC_MOJO_SYNCER_STRUCT_TRAITS_H_
