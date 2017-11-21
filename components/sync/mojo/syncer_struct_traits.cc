// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/mojo/syncer_struct_traits.h"

namespace mojo {

using StringOrdinalStructTraits =
    StructTraits<syncer::mojom::StringOrdinalDataView, syncer::StringOrdinal>;

// static
bool StringOrdinalStructTraits::Read(syncer::mojom::StringOrdinalDataView data,
                                     syncer::StringOrdinal* out) {
  std::string bytes;
  if (!data.ReadBytes(&bytes))
    return false;
  *out = syncer::StringOrdinal(bytes);
  return true;
}

}  // namespace mojo
