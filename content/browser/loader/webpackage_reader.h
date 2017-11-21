// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_WEBPACKAGE_READER_H_
#define CONTENT_BROWSER_LOADER_WEBPACKAGE_READER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/public/common/url_loader.mojom.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "url/gurl.h"

namespace content {

class URLLoaderFactoryGetter;

// XXX All dummy for now.

struct WebPackageManifest {
  WebPackageManifest();
  ~WebPackageManifest();

  GURL start_url;
};

class WebPackageReaderClient {
 public:
  virtual ~WebPackageReaderClient() {}

  // Called at most once.
  virtual void OnFoundManifest(const WebPackageManifest& manifest) = 0;

  // Maybe called multiple times. WebPackageReader.CreateResourceLoader()
  // should return true with a valid loader for the request that is notified
  // by this method.
  virtual void OnFoundResource(const ResourceRequest& request) = 0;
};

class WebPackageReader {
 public:
  // |package_url|, |loader_factory_getter| are only
  // for the dummy settings.
  WebPackageReader(WebPackageReaderClient* client,
                   const GURL& package_url,
                   scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter);
  ~WebPackageReader();

  void Consume(mojo::ScopedDataPipeConsumerHandle* body);

  // XXX Just for now. We should probably just have a method that returns
  // ResourceResponse and ScopedDataPipeConsumerHandle for a given resource.
  // Returns |false| if this package doesn't have an index for the
  // |request|.url. Otherwise |loader_request| and |loader_client| are
  // taken and bound to the actual loader.
  bool CreateResourceLoader(
      mojom::URLLoaderRequest* loader_request,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const ResourceRequest& resource_request,
      mojom::URLLoaderClientPtr* loader_client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation);

  WebPackageReaderClient* client() { return client_; }

 private:
  void CallOnFoundManifest();

  WebPackageReaderClient* client_;  // Not owned.

  // Only for dummy contents.
  GURL package_base_url_;
  scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter_;

  base::WeakPtrFactory<WebPackageReader> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(WebPackageReader);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_WEBPACKAGE_READER_H_
