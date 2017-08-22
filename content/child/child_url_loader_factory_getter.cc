// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/child_url_loader_factory_getter.h"

#include "base/feature_list.h"
#include "content/public/common/content_features.h"

namespace content {

ChildURLLoaderFactoryGetter::Info::Info(
    mojom::URLLoaderFactoryPtrInfo network_loader_factory_info,
    mojom::URLLoaderFactoryPtrInfo blob_loader_factory_info)
    : network_loader_factory_info_(std::move(network_loader_factory_info)),
      blob_loader_factory_info_(std::move(blob_loader_factory_info)) {}

ChildURLLoaderFactoryGetter::Info::~Info() = default;

scoped_refptr<ChildURLLoaderFactoryGetter>
ChildURLLoaderFactoryGetter::Info::Bind() {
  DCHECK(network_loader_factory_info_.is_valid());
  mojom::URLLoaderFactoryPtr network_loader_factory;
  mojom::URLLoaderFactoryPtr blob_loader_factory;
  network_loader_factory.Bind(std::move(network_loader_factory_info_));
  blob_loader_factory.Bind(std::move(blob_loader_factory_info_));
  return base::MakeRefCounted<ChildURLLoaderFactoryGetter>(
      std::move(network_loader_factory), std::move(blob_loader_factory));
}

ChildURLLoaderFactoryGetter::ChildURLLoaderFactoryGetter(
    PossiblyAssociatedURLLoaderFactory network_loader_factory,
    URLLoaderFactoryGetterCallback blob_loader_factory_getter)
    : network_loader_factory_(std::move(network_loader_factory)),
      blob_loader_factory_getter_(std::move(blob_loader_factory_getter)) {}

ChildURLLoaderFactoryGetter::ChildURLLoaderFactoryGetter(
    PossiblyAssociatedURLLoaderFactory network_loader_factory,
    PossiblyAssociatedURLLoaderFactory blob_loader_factory)
    : network_loader_factory_(std::move(network_loader_factory)),
      blob_loader_factory_(std::move(blob_loader_factory)) {}

std::unique_ptr<ChildURLLoaderFactoryGetter::Info>
ChildURLLoaderFactoryGetter::GetClonedInfo() {
  mojom::URLLoaderFactoryPtrInfo network_loader_factory_info;
  GetNetworkLoaderFactory()->Clone(
      mojo::MakeRequest(&network_loader_factory_info));

  mojom::URLLoaderFactoryPtrInfo blob_loader_factory_info;
  GetBlobLoaderFactory()->Clone(mojo::MakeRequest(&blob_loader_factory_info));

  return base::MakeUnique<Info>(std::move(network_loader_factory_info),
                                std::move(blob_loader_factory_info));
}

mojom::URLLoaderFactory*
ChildURLLoaderFactoryGetter::GetNetworkLoaderFactory() {
  return network_loader_factory_.get();
}

mojom::URLLoaderFactory* ChildURLLoaderFactoryGetter::GetBlobLoaderFactory() {
  if (!base::FeatureList::IsEnabled(features::kNetworkService)) {
    return GetNetworkLoaderFactory();
  }

  if (!blob_loader_factory_) {
    DCHECK(!blob_loader_factory_getter_.is_null());
    blob_loader_factory_ = std::move(blob_loader_factory_getter_).Run();
  }
  return blob_loader_factory_.get();
}

ChildURLLoaderFactoryGetter::~ChildURLLoaderFactoryGetter() = default;

}  // namespace content
