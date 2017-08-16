// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/cookies/canonical_cookie.h"
#include "third_party/WebKit/public/platform/modules/cookie_store/cookie_store.mojom.h"
#include "url/gurl.h"

namespace net {

class URLRequestContext;
class URLRequestContextGetter;

}  // namespace net

namespace content {

class BrowserContext;

// Implements the CookieStore interface in the Async Cookies API.
//
// Instances of this class must be created and used on the I/O thread. Instances
// are created by CreateMojoService() and are bound to the lifetimes of the
// mojo connections that they serve, via mojo::StrongBinding.
class CONTENT_EXPORT CookieStoreImpl : public blink::mojom::CookieStore {
 public:
  ~CookieStoreImpl() override;

  static void CreateMojoService(
      BrowserContext* browser_context,
      scoped_refptr<net::URLRequestContextGetter> request_context,
      int render_process_id,
      int render_frame_id,
      blink::mojom::CookieStoreRequest request);

  // Implements CookieStore.getAll() in the Async Cookies API.
  //
  // This method is also used as the backend for CookieStore.get() and
  // CookieStore.has().
  void GetAll(const GURL& url,
              const GURL& site_for_cookies,
              blink::mojom::CookieStoreGetOptionsPtr options,
              blink::mojom::CookieStore::GetAllCallback callback) override;

  // Implements CookieStore.set() in the Async Cookies API.
  //
  // This method is also used as the backend for CookieStore.remove().
  void Set(const GURL& url,
           const GURL& site_for_cookies,
           blink::mojom::CookieStoreSetOptionsPtr options,
           blink::mojom::CookieStore::SetCallback callback) override;

 private:
  // Use CreateMojoService() instead of invoking the constructor directly.
  CookieStoreImpl(BrowserContext* browser_context,
                  scoped_refptr<net::URLRequestContextGetter> request_context,
                  int render_process_id,
                  int render_frame_id);
  net::URLRequestContext* GetRequestContextForURL(const GURL& url);

  // Feeds a net::CookieList to a GetAll() callback.
  void CookieListToGetAllCallback(
      const GURL& url,
      const GURL& site_for_cookies,
      blink::mojom::CookieStoreGetOptionsPtr options,
      blink::mojom::CookieStore::GetAllCallback callback,
      const net::CookieList& cookie_list);

  BrowserContext* browser_context_;
  scoped_refptr<net::URLRequestContextGetter> request_context_;
  int render_process_id_;
  int render_frame_id_;

  // Needed so that callbacks don't get called after the instance is deleted,
  // if the mojo connection goes away.
  base::WeakPtrFactory<CookieStoreImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CookieStoreImpl);
};

}  // namespace content
