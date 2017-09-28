// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/restricted_cookie_manager_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_options.h"
#include "net/cookies/cookie_store.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace content {

RestrictedCookieManagerImpl::RestrictedCookieManagerImpl(
    BrowserContext* browser_context,
    scoped_refptr<net::URLRequestContextGetter> request_context,
    int render_process_id,
    int render_frame_id)
    : browser_context_(browser_context),
      request_context_(std::move(request_context)),
      render_process_id_(render_process_id),
      render_frame_id_(render_frame_id),
      weak_ptr_factory_(this) {}

RestrictedCookieManagerImpl::~RestrictedCookieManagerImpl() = default;

void RestrictedCookieManagerImpl::CreateMojoService(
    BrowserContext* browser_context,
    scoped_refptr<net::URLRequestContextGetter> request_context,
    int render_process_id,
    int render_frame_id,
    network::mojom::CookieManagerRequest request) {
  mojo::MakeStrongBinding(base::WrapUnique(new RestrictedCookieManagerImpl(
                              browser_context, std::move(request_context),
                              render_process_id, render_frame_id)),
                          std::move(request));
}

void RestrictedCookieManagerImpl::GetAllForUrl(
    const GURL& url,
    const GURL& site_for_cookies,
    network::mojom::CookieManagerGetOptionsPtr options,
    GetAllForUrlCallback callback) {
  ChildProcessSecurityPolicyImpl* policy =
      ChildProcessSecurityPolicyImpl::GetInstance();
  if (!policy->CanAccessDataForOrigin(render_process_id_, url)) {
    mojo::ReportBadMessage("Renderer attempted to access forbidden origin");
    std::move(callback).Run(std::vector<net::CanonicalCookie>());
    return;
  }

  net::CookieOptions net_options;
  if (net::registry_controlled_domains::SameDomainOrHost(
          url, site_for_cookies,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    // TODO(mkwst): This check ought to further distinguish between frames
    // initiated in a strict or lax same-site context.
    net_options.set_same_site_cookie_mode(
        net::CookieOptions::SameSiteCookieMode::INCLUDE_STRICT_AND_LAX);
  } else {
    net_options.set_same_site_cookie_mode(
        net::CookieOptions::SameSiteCookieMode::DO_NOT_INCLUDE);
  }

  GetRequestContextForURL(url)->cookie_store()->GetCookieListWithOptionsAsync(
      url, net_options,
      base::BindOnce(
          &RestrictedCookieManagerImpl::CookieListToGetAllForUrlCallback,
          weak_ptr_factory_.GetWeakPtr(), url, site_for_cookies,
          std::move(options), std::move(callback)));
}

net::URLRequestContext* RestrictedCookieManagerImpl::GetRequestContextForURL(
    const GURL& url) {
  // TODO(pwnall): Replicate the call to OverrideRequestContextForURL() in
  //               RenderFrameMessageFilter::GetRequestContextForURL().
  // ResourceContext* resource_context = browser_context->GetResourceContext();
  return request_context_->GetURLRequestContext();
}

void RestrictedCookieManagerImpl::CookieListToGetAllForUrlCallback(
    const GURL& url,
    const GURL& site_for_cookies,
    network::mojom::CookieManagerGetOptionsPtr options,
    GetAllForUrlCallback callback,
    const net::CookieList& cookie_list) {
  // TODO(pwnall): Replicate the security checks in
  //               RenderFrameMessageFilter::CheckPolicyForCookies
  (void)(browser_context_);
  (void)(render_frame_id_);

  std::vector<net::CanonicalCookie> result;
  result.reserve(cookie_list.size());
  for (size_t i = 0; i < cookie_list.size(); ++i) {
    // TODO(pwnall): Parsing and canonicalization for net::CanonicalCookie.
    const net::CanonicalCookie& cookie = cookie_list[i];
    // TODO(pwnall): Check cookie against options.
    result.emplace_back(base::in_place, cookie);
  }
  std::move(callback).Run(std::move(result));
}

void RestrictedCookieManagerImpl::SetCanonicalCookie(
    const net::CanonicalCookie& cookie,
    const GURL& url,
    const GURL& site_for_cookies,
    SetCanonicalCookieCallback callback) {
  ChildProcessSecurityPolicyImpl* policy =
      ChildProcessSecurityPolicyImpl::GetInstance();
  if (!policy->CanAccessDataForOrigin(render_process_id_, url)) {
    mojo::ReportBadMessage("Renderer attempted to access forbidden origin");
    std::move(callback).Run(false);
    return;
  }

  // TODO(pwnall): Validate the CanonicalCookie fields.
  // TODO(pwnall): Replicate the AllowSetCookie check in
  //               RenderFrameMessageFilter::SetCookie.
  base::Time now = base::Time::NowFromSystemTime();
  net::CookieSameSite cookie_same_site_mode = net::CookieSameSite::STRICT_MODE;
  net::CookiePriority cookie_priority = net::COOKIE_PRIORITY_DEFAULT;
  auto sanitized_cookie = base::MakeUnique<net::CanonicalCookie>(
      cookie.Name(), cookie.Value(), cookie.Domain(), cookie.Path(), now,
      cookie.ExpiryDate(), now, cookie.IsSecure(), cookie.IsHttpOnly(),
      cookie_same_site_mode, cookie_priority);

  bool secure_source = true;
  bool modify_http_only = true;
  net::URLRequestContext* context = GetRequestContextForURL(url);
  context->cookie_store()->SetCanonicalCookieAsync(
      std::move(sanitized_cookie), secure_source, modify_http_only,
      std::move(callback));
}

}  // namespace content
