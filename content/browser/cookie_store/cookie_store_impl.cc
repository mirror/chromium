// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/cookie_store/cookie_store_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/cookies/cookie_options.h"
#include "net/cookies/cookie_store.h"
#include "net/url_request/url_request_context.h"

namespace content {

CookieStoreImpl::CookieStoreImpl(BrowserContext* browser_context,
                                 int renderer_process_id)
    : resource_context_(browser_context->GetResourceContext()),
      renderer_process_id_(renderer_process_id) {}

void CookieStoreImpl::CreateMojoService(
    BrowserContext* browser_context,
    int renderer_process_id,
    blink::mojom::CookieStoreRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<CookieStoreImpl>(browser_context, renderer_process_id),
      std::move(request));
}

void CookieStoreImpl::GetAll(
    const GURL& url,
    const GURL& first_party_for_cookies,
    const std::string& name,
    blink::mojom::CookieStoreGetOptionsPtr options,
    blink::mojom::CookieStore::GetAllCallback callback) {
  std::vector<blink::mojom::CookieInfoPtr> response;
  response.emplace_back(base::in_place, "answer", "42");
  std::move(callback).Run(std::move(response));

  (void)(renderer_process_id_);
  (void)(resource_context_);

  /*
    ChildProcessSecurityPolicyImpl* policy =
        ChildProcessSecurityPolicyImpl::GetInstance();
    if (!policy->CanAccessDataForOrigin(renderer_process_id_, url)) {
      mojo::ReportBadMessage("bad bad renderer");
      std::move(callback).Run(response);
      return;
    }

    net::CookieOptions net_options;
    if (net::registry_controlled_domains::SameDomainOrHost(
        url, first_party_for_cookies,
        net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
      // TODO(mkwst): This check ought to further distinguish between frames
      // initiated in a strict or lax same-site context.
      net_options.set_same_site_cookie_mode(
          net::CookieOptions::SameSiteCookieMode::INCLUDE_STRICT_AND_LAX);
    } else {
      net_options.set_same_site_cookie_mode(
          net::CookieOptions::SameSiteCookieMode::DO_NOT_INCLUDE);
    }

    // TODO(pwnall): Replicate the call to OverrideRequestContextForURL() in
    //               RenderFrameMessageFilter::GetRequestContextForURL().
    net::URLRequestContext* context = request_context_->GetURLRequestContext();
    context->cookie_store()->GetCookieListWithOptionsAsync(
        url, options,
        base::Bind(&RenderFrameMessageFilter::CheckPolicyForCookies, this,
                   render_frame_id, url, first_party_for_cookies,
                   base::Passed(&callback)));
  */
}

void CookieStoreImpl::Set(const GURL& url,
                          const GURL& first_party_for_cookies,
                          const std::string& name,
                          const std::string& value,
                          blink::mojom::CookieStoreSetOptionsPtr options,
                          blink::mojom::CookieStore::SetCallback callback) {
  std::move(callback).Run(true);
}

}  // namespace content
