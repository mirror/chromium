// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_INTERFACES_CDM_PROXY_TYPEMAP_STRUCT_TRAITS_H_
#define MEDIA_MOJO_INTERFACES_CDM_PROXY_TYPEMAP_STRUCT_TRAITS_H_

#include "media/cdm/cdm_proxy.h"

#include <vector>

#include "media/base/ipc/media_param_traits.h"
#include "media/mojo/interfaces/cdm_proxy.mojom.h"
#include "mojo/public/cpp/bindings/struct_traits.h"

namespace mojo {
template <>
class StructTraits<media::mojom::KeyInfoDataView, media::KeyInfo> {
 public:
  static uint32_t crypto_session_id(const media::KeyInfo& k) {
    return k.crypto_session_id;
  }
  static const std::vector<uint8_t>& key_id(const media::KeyInfo& k) {
    return k.key_id;
  }
  static const std::vector<uint8_t>& key_blob(const media::KeyInfo& k) {
    return k.key_blob;
  }
  static bool is_usable_key(const media::KeyInfo& k) { return k.is_usable_key; }
  static bool Read(media::mojom::KeyInfoDataView input, media::KeyInfo* output);
};
}  // namespace mojo

#endif  // MEDIA_MOJO_INTERFACES_CDM_PROXY_TYPEMAP_STRUCT_TRAITS_H_