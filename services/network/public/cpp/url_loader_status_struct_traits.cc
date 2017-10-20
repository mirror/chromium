// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/url_loader_status_struct_traits.h"

#include "ipc/ipc_message_utils.h"
#include "mojo/common/common_custom_types_struct_traits.h"

namespace mojo {

bool StructTraits<network::mojom::URLLoaderStatusDataView,
                  network::URLLoaderStatus>::Read(DataView view, Impl* impl) {
  impl->error_code = view.error_code();
  impl->exists_in_cache = view.exists_in_cache();
  if (!view.ReadCompletionTime(&impl->completion_time))
    return false;
  impl->encoded_data_length = view.encoded_data_length();
  impl->encoded_body_length = view.encoded_body_length();
  impl->decoded_body_length = view.decoded_body_length();
  return true;
}

}  // namespace mojo
