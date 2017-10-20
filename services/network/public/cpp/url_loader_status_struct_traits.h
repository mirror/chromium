// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_URL_LOADER_STATUS_STRUCT_TRAITS_H_
#define SERVICES_NETWORK_PUBLIC_CPP_URL_LOADER_STATUS_STRUCT_TRAITS_H_

#include "mojo/public/cpp/bindings/struct_traits.h"
#include "services/network/public/cpp/url_loader_status.h"
#include "services/network/public/interfaces/url_loader.mojom.h"

namespace mojo {

template <>
struct StructTraits<network::mojom::URLLoaderStatusDataView,
                    network::URLLoaderStatus> {
  using DataView = network::mojom::URLLoaderStatusDataView;
  using Impl = network::URLLoaderStatus;
  static int error_code(const Impl& status) { return status.error_code; }

  static bool exists_in_cache(const Impl& status) {
    return status.exists_in_cache;
  }

  static base::TimeTicks completion_time(const Impl& status) {
    return status.completion_time;
  }

  static int64_t encoded_data_length(const Impl& status) {
    return status.encoded_data_length;
  }

  static int64_t encoded_body_length(const Impl& status) {
    return status.encoded_body_length;
  }

  static int64_t decoded_body_length(const Impl& status) {
    return status.decoded_body_length;
  }

  static bool Read(DataView view, Impl* impl);
};

}  // namespace mojo

#endif  // SERVICES_NETWORK_PUBLIC_CPP_URL_LOADER_STATUS_STRUCT_TRAITS_H_
