// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/cookies/canonical_cookie.h"
#include "third_party/WebKit/public/platform/modules/cookie_store/cookie_store.mojom.h"
#include "url/gurl.h"

namespace net {

class URLRequestContext;
class URLRequestContextGetter;

};  // namespace net

namespace content {

class BrowserContext;

class CONTENT_EXPORT CookieStoreImpl
    : NON_EXPORTED_BASE(public blink::mojom::CookieStore) {
 public:
  CookieStoreImpl(BrowserContext* resource_context,
                  scoped_refptr<net::URLRequestContextGetter> request_context,
                  int render_process_id,
                  int render_frame_id);
  ~CookieStoreImpl() override;

  static void CreateMojoService(
      BrowserContext* browser_context,
      scoped_refptr<net::URLRequestContextGetter> request_context,
      int render_process_id,
      int render_frame_id,
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
  net::URLRequestContext* GetRequestContextForURL(const GURL& url);

  // Must run on the I/O thread.
  void DoGetAll(const GURL& url,
                const GURL& first_party_for_cookies,
                const std::string& name,
                blink::mojom::CookieStoreGetOptionsPtr options,
                scoped_refptr<base::SequencedTaskRunner> callback_runner,
                blink::mojom::CookieStore::GetAllCallback callback);
  // Converts net::CookieList to the format used by GetAllCallback.
  //
  // Must run on the I/O thread.
  void CookieListToGetAllCallback(
      const GURL& url,
      const GURL& first_party_for_cookies,
      const std::string& name,
      blink::mojom::CookieStoreGetOptionsPtr options,
      scoped_refptr<base::SequencedTaskRunner> callback_runner,
      blink::mojom::CookieStore::GetAllCallback callback,
      const net::CookieList& cookie_list);

  // Must run on the I/O thread.
  void DoSet(const GURL& url,
             const GURL& first_party_for_cookies,
             const std::string& name,
             const std::string& value,
             blink::mojom::CookieStoreSetOptionsPtr options,
             scoped_refptr<base::SequencedTaskRunner> callback_runner,
             blink::mojom::CookieStore::SetCallback callback);
  // Converts net::CookieList to the format used by GetAllCallback.
  //
  // Must run on the I/O thread.
  void CookieStoreSuccessToSetCallback(
      scoped_refptr<base::SequencedTaskRunner> callback_runner,
      blink::mojom::CookieStore::SetCallback callback,
      bool success);

  BrowserContext* browser_context_;
  scoped_refptr<net::URLRequestContextGetter> request_context_;
  int render_process_id_, render_frame_id_;

  DISALLOW_COPY_AND_ASSIGN(CookieStoreImpl);
};

}  // namespace content
