// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/interfaces/http_request_headers_struct_traits.h"

namespace mojo {

// static
bool StructTraits<net::mojom::HttpRequestHeaderKeyValuePairDataView,
                  net::HttpRequestHeaders::HeaderKeyValuePair>::
    Read(net::mojom::HttpRequestHeaderKeyValuePairDataView data,
         net::HttpRequestHeaders::HeaderKeyValuePair* item) {
  if (!data.ReadKey(&item->key))
    return false;
  if (!data.ReadValue(&item->value))
    return false;
  return true;
}

// static
bool StructTraits<
    net::mojom::HttpRequestHeadersDataView,
    net::HttpRequestHeaders>::Read(net::mojom::HttpRequestHeadersDataView data,
                                   net::HttpRequestHeaders* headers) {
  ArrayDataView<net::mojom::HttpRequestHeaderKeyValuePairDataView> data_view;
  net::HttpRequestHeaders::HeaderVector header_vector;
  data.GetHeadersDataView(&data_view);
  header_vector.resize(data_view.size());
  for (size_t i = 0; i < data_view.size(); ++i) {
    if (!data_view.Read(i, &header_vector[i]))
      return false;
  }
  headers->Swap(&header_vector);
  return true;
}

}  // namespace mojo
