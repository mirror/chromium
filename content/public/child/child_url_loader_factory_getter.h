// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_CHILD_CHILD_URL_LOADER_FACTORY_GETTER_H_
#define CONTENT_PUBLIC_CHILD_CHILD_URL_LOADER_FACTORY_GETTER_H_

#include "base/memory/ref_counted.h"
#include "content/public/common/url_loader_factory.mojom.h"

class GURL;

namespace content {

// ChildProcess version's URLLoaderFactoryGetter, i.e. a getter that holds
// on to URLLoaderFactory's for a given loading context (e.g. a frame)
// and allows code to access them.
class ChildURLLoaderFactoryGetter
    : public base::RefCounted<ChildURLLoaderFactoryGetter> {
 public:
  // Info class stores necessary information to create a clone of
  // ChildURLLoaderFactoryGetter in worker thread.
  class Info {
   public:
    Info(mojom::URLLoaderFactoryPtrInfo network_loader_factory_info,
         mojom::URLLoaderFactoryPtrInfo non_network_loader_factory_info);
    Info(Info&& other);
    ~Info();

    scoped_refptr<ChildURLLoaderFactoryGetter> Bind();

   private:
    mojom::URLLoaderFactoryPtrInfo network_loader_factory_info_;
    mojom::URLLoaderFactoryPtrInfo non_network_loader_factory_info_;
  };

  virtual Info GetClonedInfo() = 0;

  // Returns the URLLoaderFactory that can handle the given |request_url|. If
  // the request is a non-network URL and a non-network loader factory has been
  // established, that factory will be returned. Otherwise, if non-null,
  // |default_factory| is returned. Finally if all else fails, the established
  // network loader factory is returned.
  virtual mojom::URLLoaderFactory* GetFactoryForURL(
      const GURL& request_url,
      mojom::URLLoaderFactory* default_factory = nullptr) = 0;

  virtual mojom::URLLoaderFactory* GetNetworkLoaderFactory() = 0;
  virtual mojom::URLLoaderFactory* GetNonNetworkLoaderFactory() = 0;

 protected:
  friend class base::RefCounted<ChildURLLoaderFactoryGetter>;
  virtual ~ChildURLLoaderFactoryGetter() = default;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_CHILD_CHILD_URL_LOADER_FACTORY_GETTER_H_
