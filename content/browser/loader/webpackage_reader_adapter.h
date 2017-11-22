// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_WEBPACKAGE_READER_ADAPTER_H_
#define CONTENT_BROWSER_LOADER_WEBPACKAGE_READER_ADAPTER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/public/common/url_loader.mojom.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "url/gurl.h"
#include "wpkg/webpackage_reader.h"

namespace content {

class URLLoaderFactoryGetter;

struct WebPackageManifest {
  WebPackageManifest();
  ~WebPackageManifest();

  // We use 'origin' as start_url for now.
  GURL start_url;
};

class WebPackageReaderAdapterClient {
 public:
  virtual ~WebPackageReaderAdapterClient() {}

  // Called at most once.
  virtual void OnFoundManifest(const WebPackageManifest& manifest) = 0;

  virtual void OnFoundResource(const ResourceRequest& request) = 0;
};

class WebPackageReaderAdapter : public wpkg::WebPackageReaderClient {
 public:
  using ResourceCallback =
      base::OnceCallback<void(const ResourceResponseHead& response_head,
                              mojo::ScopedDataPipeConsumerHandle body)>;

  // |package_url|, |loader_factory_getter| are only
  // for the dummy settings.
  WebPackageReaderAdapter(
      WebPackageReaderAdapterClient* client,
      const GURL& package_url,
      scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter);
  ~WebPackageReaderAdapter() override;

  // This will take |body|.
  void Consume(mojo::ScopedDataPipeConsumerHandle body);

  // Synchronously returns |false| if this package doesn't have the resource
  // for the given |resource_request|. Otherwise |callback| will be fired
  // with a valid |response_head| and |body|.
  bool GetResource(const ResourceRequest& resource_request,
                   ResourceCallback callback);

  WebPackageReaderAdapterClient* client() { return client_; }

 private:
  // wpkg::WebPackageReaderClient:
  // TODO(kinuko): Implement.
  void OnOrigin(const std::string& origin) override {}
  void OnResourceRequest(const wpkg::HttpHeaders& request_headers,
                         int request_id) override {}
  void OnResourceResponse(int request_id,
                          const wpkg::HttpHeaders& response_headers) override {}
  void OnDataReceived(int request_id, const void* data, size_t size) override {}
  void OnNotifyFinished(int request_id) override {}
  void OnEnd() override {}

  WebPackageReaderAdapterClient* client_;  // Not owned.

  // Only for dummy contents.
  GURL package_base_url_;
  scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter_;

  std::unique_ptr<wpkg::WebPackageReader> reader_;

  base::WeakPtrFactory<WebPackageReaderAdapter> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(WebPackageReaderAdapter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_WEBPACKAGE_READER_ADAPTER_H_
