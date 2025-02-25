// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INFO_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INFO_H_

#include <stdint.h>

#include <vector>

#include "base/callback.h"
#include "base/time/time.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_types.h"
#include "third_party/WebKit/common/service_worker/service_worker_provider_type.mojom.h"
#include "url/gurl.h"

namespace content {

enum class EmbeddedWorkerStatus;

struct CONTENT_EXPORT ServiceWorkerVersionInfo {
 public:
  struct CONTENT_EXPORT ClientInfo {
   public:
    ClientInfo();
    ClientInfo(int process_id,
               int route_id,
               const base::Callback<WebContents*(void)>& web_contents_getter,
               blink::mojom::ServiceWorkerProviderType type);
    ClientInfo(const ClientInfo& other);
    ~ClientInfo();
    int process_id;
    int route_id;
    // |web_contents_getter| is only set for PlzNavigate.
    base::Callback<WebContents*(void)> web_contents_getter;
    blink::mojom::ServiceWorkerProviderType type;
  };

  ServiceWorkerVersionInfo();
  ServiceWorkerVersionInfo(
      EmbeddedWorkerStatus running_status,
      ServiceWorkerVersion::Status status,
      ServiceWorkerVersion::FetchHandlerExistence fetch_handler_existence,
      const GURL& script_url,
      int64_t registration_id,
      int64_t version_id,
      int process_id,
      int thread_id,
      int devtools_agent_route_id);
  ServiceWorkerVersionInfo(const ServiceWorkerVersionInfo& other);
  ~ServiceWorkerVersionInfo();

  EmbeddedWorkerStatus running_status;
  ServiceWorkerVersion::Status status;
  ServiceWorkerVersion::FetchHandlerExistence fetch_handler_existence;
  blink::mojom::NavigationPreloadState navigation_preload_state;
  GURL script_url;
  int64_t registration_id;
  int64_t version_id;
  int process_id;
  int thread_id;
  int devtools_agent_route_id;
  base::Time script_response_time;
  base::Time script_last_modified;
  std::map<std::string, ClientInfo> clients;
};

struct CONTENT_EXPORT ServiceWorkerRegistrationInfo {
 public:
  enum DeleteFlag { IS_NOT_DELETED, IS_DELETED };
  ServiceWorkerRegistrationInfo();
  ServiceWorkerRegistrationInfo(const GURL& pattern,
                                int64_t registration_id,
                                DeleteFlag delete_flag);
  ServiceWorkerRegistrationInfo(
      const GURL& pattern,
      blink::mojom::ServiceWorkerUpdateViaCache update_via_cache,
      int64_t registration_id,
      DeleteFlag delete_flag,
      const ServiceWorkerVersionInfo& active_version,
      const ServiceWorkerVersionInfo& waiting_version,
      const ServiceWorkerVersionInfo& installing_version,
      int64_t stored_version_size_bytes,
      bool navigation_preload_enabled,
      size_t navigation_preload_header_length);
  ServiceWorkerRegistrationInfo(const ServiceWorkerRegistrationInfo& other);
  ~ServiceWorkerRegistrationInfo();

  GURL pattern;
  blink::mojom::ServiceWorkerUpdateViaCache update_via_cache;
  int64_t registration_id;
  DeleteFlag delete_flag;
  ServiceWorkerVersionInfo active_version;
  ServiceWorkerVersionInfo waiting_version;
  ServiceWorkerVersionInfo installing_version;

  int64_t stored_version_size_bytes;
  bool navigation_preload_enabled;
  size_t navigation_preload_header_length;
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INFO_H_
