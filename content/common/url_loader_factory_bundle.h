// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_URL_LOADER_FACTORY_BUNDLE_H_
#define CONTENT_COMMON_URL_LOADER_FACTORY_BUNDLE_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/common/url_loader_factory.mojom.h"

class GURL;

namespace mojo {
template <typename, typename>
struct StructTraits;
}

namespace content {

namespace mojom {
class URLLoaderFactoryBundleDataView;
}

// Holds the internal state of a URLLoaderFactoryBundle in a form that is safe
// to pass across sequences.
struct CONTENT_EXPORT URLLoaderFactoryBundleInfo {
  URLLoaderFactoryBundleInfo(URLLoaderFactoryBundleInfo&&);
  URLLoaderFactoryBundleInfo(
      mojom::URLLoaderFactoryPtrInfo default_factory_info,
      bool is_reconnectable_network_service_factory,
      std::map<std::string, mojom::URLLoaderFactoryPtrInfo> factories_info);
  ~URLLoaderFactoryBundleInfo();

  mojom::URLLoaderFactoryPtrInfo default_factory_info;
  bool is_reconnectable_network_service_factory;
  std::map<std::string, mojom::URLLoaderFactoryPtrInfo> factories_info;
};

// Encapsulates a collection of URLLoaderFactoryPtrs which can be usd to acquire
// loaders for various types of resource requests.
class CONTENT_EXPORT URLLoaderFactoryBundle {
 public:
  URLLoaderFactoryBundle();
  URLLoaderFactoryBundle(URLLoaderFactoryBundle&&);
  explicit URLLoaderFactoryBundle(URLLoaderFactoryBundleInfo info);
  ~URLLoaderFactoryBundle();

  URLLoaderFactoryBundle& operator=(URLLoaderFactoryBundle&&);

  // Sets the default factory to use when no registered factories match a given
  // |url|. Set |is_reconnectable_network_service_factory| to true if the
  // default factory is Network Service-backed and can be re-created on
  // connection error.
  void SetDefaultFactory(mojom::URLLoaderFactoryPtr factory,
                         bool is_reconnectable_network_service_factory);

  // Provides the getter to re-create |default_factory_| on connection error.
  // Will only be used when |is_reconnectable_network_service_factory_| is true.
  using NetworkServiceFactoryGetter =
      base::RepeatingCallback<void(mojom::URLLoaderFactoryRequest)>;
  void SetNetworkServiceFactoryGetter(NetworkServiceFactoryGetter);

  // Registers a new factory to handle requests matching scheme |scheme|.
  void RegisterFactory(const base::StringPiece& scheme,
                       mojom::URLLoaderFactoryPtr factory);

  // Returns a factory which can be used to acquire a loader for |url|. If no
  // registered factory matches |url|'s scheme, the default factory is used. It
  // is undefined behavior to call this when no default factory is set.
  mojom::URLLoaderFactory* GetFactoryForRequest(const GURL& url);

  // Passes out a structure which captures the internal state of this bundle in
  // a form that is safe to pass across sequences. Effectively resets |this|
  // to have no registered factories.
  URLLoaderFactoryBundleInfo PassInfo();

  // Creates a clone of this bundle which can be passed to and owned by another
  // consumer. The clone operates identically to but independent from the
  // original (this) bundle.
  URLLoaderFactoryBundle Clone();

  // Call |FlushForTesting()| on Network Service related interfaces. For test
  // use only.
  void FlushNetworkInterfaceForTesting();

 private:
  friend struct mojo::StructTraits<mojom::URLLoaderFactoryBundleDataView,
                                   URLLoaderFactoryBundle>;

  mojom::URLLoaderFactoryPtr default_factory_;

  // True if |default_factory_| is a Network Service-backed factory and can be
  // re-created on connection error.
  bool is_reconnectable_network_service_factory_;

  // Used to re-create Network Service-backed |default_factory_| on connection
  // error. The getter won't be passed across mojo.
  NetworkServiceFactoryGetter network_service_factory_getter_;

  std::map<std::string, mojom::URLLoaderFactoryPtr> factories_;

  DISALLOW_COPY_AND_ASSIGN(URLLoaderFactoryBundle);
};

}  // namespace content

#endif  // CONTENT_COMMON_URL_LOADER_FACTORY_BUNDLE_H_
