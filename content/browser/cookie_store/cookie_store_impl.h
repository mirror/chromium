// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/WebKit/public/platform/modules/cookie_store/cookie_store.mojom.h"
#include "url/gurl.h"

namespace content {

class BrowserContext;
class ResourceContext;

class CONTENT_EXPORT CookieStoreImpl
    : NON_EXPORTED_BASE(public blink::mojom::CookieStore) {
 public:
  CookieStoreImpl(BrowserContext* resource_context, int renderer_process_id);

  static void CreateMojoService(BrowserContext* resource_context,
                                int renderer_process_id,
                                blink::mojom::CookieStoreRequest request);

  void GetAll(const GURL& url,
              const GURL& first_party_for_cookies,
              const std::string& name,
              blink::mojom::CookieStoreGetOptionsPtr options,
              blink::mojom::CookieStore::GetAllCallback callback) override;

  void Set(const GURL& url,
           const GURL& first_party_for_cookies,
           const std::string& name,
           const std::string& value,
           blink::mojom::CookieStoreSetOptionsPtr options,
           blink::mojom::CookieStore::SetCallback callback) override;

 private:
  ResourceContext* resource_context_;
  int renderer_process_id_;

  DISALLOW_COPY_AND_ASSIGN(CookieStoreImpl);
};

}  // namespace content
