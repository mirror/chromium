// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_worker/service_worker_fetch_response_struct_traits.h"

#include "content/public/common/referrer_struct_traits.h"
#include "ipc/ipc_message_utils.h"
#include "mojo/common/time_struct_traits.h"
#include "url/mojo/url_gurl_struct_traits.h"

namespace mojo {

bool StructTraits<blink::mojom::FetchAPIResponseDataView,
                  content::ServiceWorkerResponse>::
    Read(blink::mojom::FetchAPIResponseDataView data,
         content::ServiceWorkerResponse* out) {
  std::unordered_map<std::string, std::string> headers;
  base::Optional<std::string> blob_uuid;
  if (!data.ReadUrlList(&out->url_list) ||
      !data.ReadStatusText(&out->status_text) ||
      !data.ReadResponseType(&out->response_type) ||
      !data.ReadHeaders(&headers) || !data.ReadBlobUuid(&blob_uuid) ||
      !data.ReadError(&out->error) ||
      !data.ReadResponseTime(&out->response_time) ||
      !data.ReadCacheStorageCacheName(&out->cache_storage_cache_name) ||
      !data.ReadCorsExposedHeaderNames(&out->cors_exposed_header_names)) {
    return false;
  }

  out->status_code = data.status_code();
  out->blob_size = data.blob_size();
#if 0
  // TODO(cmumford): Figure where this is set. Move to the out param?
  out->is_in_cache_storage = data.is_in_cache_storage();
#endif
  out->headers.insert(headers.begin(), headers.end());
  if (blob_uuid) {
    out->blob_uuid = blob_uuid.value();
    out->blob_size = data.blob_size();
  }

  return true;
}

std::map<std::string, std::string>
StructTraits<blink::mojom::FetchAPIResponseDataView,
             content::ServiceWorkerResponse>::
    headers(const content::ServiceWorkerResponse& response) {
  std::map<std::string, std::string> headers;

  for (const auto& it : response.headers)
    headers[it.first] = it.second;

  return headers;
}

}  // namespace mojo
