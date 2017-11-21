// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/interfaces/cdm_proxy_typemap_struct_traits.h"

namespace mojo {

// static
bool StructTraits<media::mojom::KeyInfoDataView, media::KeyInfo>::Read(
    media::mojom::KeyInfoDataView input,
    media::KeyInfo* output) {
  output->crypto_session_id = input.crypto_session_id();
  if (!input.ReadKeyId(&output->key_id))
    return false;
  if (!input.ReadKeyBlob(&output->key_blob))
    return false;
  output->is_usable_key = input.is_usable_key();
  return true;
};

}  // namespace mojo