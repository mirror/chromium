// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_IMPL_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_IMPL_H_

#include "content/browser/background_fetch/background_fetch_delegate.h"

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/download_interrupt_reasons.h"

namespace net {
class URLRequestContextGetter;
}

namespace content {

class BrowserContext;
class DownloadItem;

class CONTENT_EXPORT BackgroundFetchDelegateImpl
    : public BackgroundFetchDelegate {
 public:
  BackgroundFetchDelegateImpl(
      BrowserContext* browser_context,
      scoped_refptr<net::URLRequestContextGetter> request_context);

  ~BackgroundFetchDelegateImpl() override;

  void DownloadUrl(const std::string& guid,
                   const std::string& method,
                   const GURL& url,
                   const net::NetworkTrafficAnnotationTag& traffic_annotation,
                   const net::HttpRequestHeaders& headers) override;

  void SetDelegateClient(base::WeakPtr<Client> client) override;

 private:
  void DidStartRequest(DownloadItem* download_item,
                       DownloadInterruptReason interrupt_reason);

  BrowserContext* browser_context_;

  base::WeakPtr<Client> client_;

  scoped_refptr<net::URLRequestContextGetter> request_context_;

  base::WeakPtrFactory<BackgroundFetchDelegateImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchDelegateImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_IMPL_H_
