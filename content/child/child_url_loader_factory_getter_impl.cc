// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/child_url_loader_factory_getter_impl.h"

#include "base/feature_list.h"
#include "content/public/common/content_features.h"
#include "content/public/common/url_utils.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"

namespace content {

ChildURLLoaderFactoryGetter::Info::Info(
    mojom::URLLoaderFactoryPtrInfo network_loader_factory_info,
    mojom::URLLoaderFactoryPtrInfo non_network_loader_factory_info)
    : network_loader_factory_info_(std::move(network_loader_factory_info)),
      non_network_loader_factory_info_(
          std::move(non_network_loader_factory_info)) {}

ChildURLLoaderFactoryGetter::Info::Info(Info&& other)
    : network_loader_factory_info_(
          std::move(other.network_loader_factory_info_)),
      non_network_loader_factory_info_(
          std::move(other.non_network_loader_factory_info_)) {}

ChildURLLoaderFactoryGetter::Info::~Info() = default;

scoped_refptr<ChildURLLoaderFactoryGetter>
ChildURLLoaderFactoryGetter::Info::Bind() {
  DCHECK(network_loader_factory_info_.is_valid());
  mojom::URLLoaderFactoryPtr network_loader_factory;
  mojom::URLLoaderFactoryPtr non_network_loader_factory;
  network_loader_factory.Bind(std::move(network_loader_factory_info_));
  non_network_loader_factory.Bind(std::move(non_network_loader_factory_info_));
  return base::MakeRefCounted<ChildURLLoaderFactoryGetterImpl>(
      std::move(network_loader_factory), std::move(non_network_loader_factory));
}

ChildURLLoaderFactoryGetterImpl::ChildURLLoaderFactoryGetterImpl() = default;

ChildURLLoaderFactoryGetterImpl::ChildURLLoaderFactoryGetterImpl(
    PossiblyAssociatedURLLoaderFactory network_loader_factory,
    URLLoaderFactoryGetterCallback non_network_loader_factory_getter)
    : network_loader_factory_(std::move(network_loader_factory)),
      non_network_loader_factory_getter_(
          std::move(non_network_loader_factory_getter)) {}

ChildURLLoaderFactoryGetterImpl::ChildURLLoaderFactoryGetterImpl(
    PossiblyAssociatedURLLoaderFactory network_loader_factory,
    PossiblyAssociatedURLLoaderFactory non_network_loader_factory)
    : network_loader_factory_(std::move(network_loader_factory)),
      non_network_loader_factory_(std::move(non_network_loader_factory)) {}

ChildURLLoaderFactoryGetterImpl::Info
ChildURLLoaderFactoryGetterImpl::GetClonedInfo() {
  mojom::URLLoaderFactoryPtrInfo network_loader_factory_info;
  GetNetworkLoaderFactory()->Clone(
      mojo::MakeRequest(&network_loader_factory_info));

  mojom::URLLoaderFactoryPtrInfo non_network_loader_factory_info;
  GetNonNetworkLoaderFactory()->Clone(
      mojo::MakeRequest(&non_network_loader_factory_info));

  return Info(std::move(network_loader_factory_info),
              std::move(non_network_loader_factory_info));
}

mojom::URLLoaderFactory* ChildURLLoaderFactoryGetterImpl::GetFactoryForURL(
    const GURL& url,
    mojom::URLLoaderFactory* default_factory) {
  if (base::FeatureList::IsEnabled(features::kNetworkService) &&
      !ShouldUseNetworkURLLoaderFactory(url)) {
    return GetNonNetworkLoaderFactory();
  }
  if (default_factory)
    return default_factory;
  return GetNetworkLoaderFactory();
}

mojom::URLLoaderFactory*
ChildURLLoaderFactoryGetterImpl::GetNetworkLoaderFactory() {
  return network_loader_factory_.get();
}

mojom::URLLoaderFactory*
ChildURLLoaderFactoryGetterImpl::GetNonNetworkLoaderFactory() {
  if (!non_network_loader_factory_) {
    if (!non_network_loader_factory_getter_)
      return GetNetworkLoaderFactory();
    non_network_loader_factory_ =
        std::move(non_network_loader_factory_getter_).Run();
  }
  return non_network_loader_factory_.get();
}

ChildURLLoaderFactoryGetterImpl::~ChildURLLoaderFactoryGetterImpl() = default;

}  // namespace content
