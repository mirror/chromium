// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/cookie_store/cookie_store_impl.h"

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

CookieStoreImpl::CookieStoreImpl(
    BrowserContext* browser_context,
    scoped_refptr<net::URLRequestContextGetter> request_context,
    int render_process_id,
    int render_frame_id)
    : browser_context_(browser_context),
      request_context_(std::move(request_context)),
      render_process_id_(render_process_id),
      render_frame_id_(render_frame_id),
      weak_ptr_factory_(this) {}

CookieStoreImpl::~CookieStoreImpl() = default;

void CookieStoreImpl::CreateMojoService(
    BrowserContext* browser_context,
    scoped_refptr<net::URLRequestContextGetter> request_context,
    int render_process_id,
    int render_frame_id,
    blink::mojom::CookieStoreRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<CookieStoreImpl>(
                              browser_context, std::move(request_context),
                              render_process_id, render_frame_id),
                          std::move(request));
}

void CookieStoreImpl::GetAll(
    const GURL& url,
    const GURL& first_party_for_cookies,
    const std::string& name,
    blink::mojom::CookieStoreGetOptionsPtr options,
    blink::mojom::CookieStore::GetAllCallback callback) {
  ChildProcessSecurityPolicyImpl* policy =
      ChildProcessSecurityPolicyImpl::GetInstance();
  if (!policy->CanAccessDataForOrigin(render_process_id_, url)) {
    mojo::ReportBadMessage("naughty renderer");
    std::move(callback).Run(std::vector<blink::mojom::CookieInfoPtr>());
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

  GetRequestContextForURL(url)->cookie_store()->GetCookieListWithOptionsAsync(
      url, net_options,
      base::BindOnce(&CookieStoreImpl::CookieListToGetAllCallback,
                     weak_ptr_factory_.GetWeakPtr(), url,
                     first_party_for_cookies, name, std::move(options),
                     std::move(callback)));
}

net::URLRequestContext* CookieStoreImpl::GetRequestContextForURL(
    const GURL& url) {
  // TODO(pwnall): Replicate the call to OverrideRequestContextForURL() in
  //               RenderFrameMessageFilter::GetRequestContextForURL().
  // ResourceContext* resource_context = browser_context->GetResourceContext();
  return request_context_->GetURLRequestContext();
}

void CookieStoreImpl::CookieListToGetAllCallback(
    const GURL& url,
    const GURL& first_party_for_cookies,
    const std::string& name,
    blink::mojom::CookieStoreGetOptionsPtr options,
    blink::mojom::CookieStore::GetAllCallback callback,
    const net::CookieList& cookie_list) {
  // TODO(pwnall): Replicate the security checks in
  //               RenderFrameMessageFilter::CheckPolicyForCookies
  (void)(browser_context_);
  (void)(render_frame_id_);

  std::vector<blink::mojom::CookieInfoPtr> result;
  result.reserve(cookie_list.size());
  for (size_t i = 0; i < cookie_list.size(); ++i) {
    const net::CanonicalCookie& cookie = cookie_list[i];
    // TODO(pwnall): Check cookie against options.
    result.emplace_back(base::in_place, cookie.Name(), cookie.Value());
  }
  std::move(callback).Run(std::move(result));
}

void CookieStoreImpl::Set(const GURL& url,
                          const GURL& first_party_for_cookies,
                          const std::string& name,
                          const std::string& value,
                          blink::mojom::CookieStoreSetOptionsPtr options,
                          blink::mojom::CookieStore::SetCallback callback) {
  ChildProcessSecurityPolicyImpl* policy =
      ChildProcessSecurityPolicyImpl::GetInstance();
  if (!policy->CanAccessDataForOrigin(render_process_id_, url)) {
    mojo::ReportBadMessage("naughty renderer");
    std::move(callback).Run(false);
    return;
  }

  // TODO(pwnall): Replicate the AllowSetCookie check in
  //               RenderFrameMessageFilter::SetCookie.
  base::Time now = base::Time::NowFromSystemTime();
  base::Time expires_at = options->expires;
  net::CookieSameSite cookie_same_site_mode = net::CookieSameSite::STRICT_MODE;
  net::CookiePriority cookie_priority = net::COOKIE_PRIORITY_DEFAULT;
  std::unique_ptr<net::CanonicalCookie> cookie =
      base::MakeUnique<net::CanonicalCookie>(
          name, value, options->domain, options->path, now, expires_at, now,
          options->secure, options->http_only, cookie_same_site_mode,
          cookie_priority);

  bool secure_source = true;
  bool modify_http_only = true;
  net::URLRequestContext* context = GetRequestContextForURL(url);
  context->cookie_store()->SetCanonicalCookieAsync(
      std::move(cookie), secure_source, modify_http_only, std::move(callback));
}

}  // namespace content
