// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/url_loader_factory_getter.h"

#include "base/bind.h"
#include "content/browser/storage_partition_impl.h"
#include "content/public/common/network_service.mojom.h"

namespace content {

URLLoaderFactoryGetter::URLLoaderFactoryGetter() {}

void URLLoaderFactoryGetter::Initialize(
    const base::WeakPtr<StoragePartitionImpl>& partition) {
  DCHECK(partition);
  partition_ = partition;

  mojom::URLLoaderFactoryPtr blob_factory;
  partition_->GetBlobURLLoaderFactory()->HandleRequest(
      mojo::MakeRequest(&blob_factory));

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&URLLoaderFactoryGetter::InitializeOnIOThread, this,
                     blob_factory.PassInterface()));
}

mojom::URLLoaderFactory* URLLoaderFactoryGetter::GetNetworkFactory() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (test_factory_)
    return test_factory_;

  if (!network_factory_.is_bound() || network_factory_.encountered_error()) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            &URLLoaderFactoryGetter::HandleNetworkFactoryRequestOnUIThread,
            this, mojo::MakeRequest(&network_factory_)));
  }
  return network_factory_.get();
}

mojom::URLLoaderFactory* URLLoaderFactoryGetter::GetBlobFactory() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return blob_factory_.get();
}

void URLLoaderFactoryGetter::SetNetworkFactoryForTesting(
    mojom::URLLoaderFactory* test_factory) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!test_factory_ || !test_factory);
  test_factory_ = test_factory;
}

void URLLoaderFactoryGetter::FlushNetworkInterfaceOnIOThreadForTesting() {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    base::RunLoop run_loop;
    BrowserThread::PostTaskAndReply(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&URLLoaderFactoryGetter::FlushNetworkInterfaceForTesting,
                       this),
        run_loop.QuitClosure());
    run_loop.Run();
    return;
  }

  FlushNetworkInterfaceForTesting();
}

void URLLoaderFactoryGetter::FlushNetworkInterfaceForTesting() {
  if (network_factory_)
    network_factory_.FlushForTesting();
}

URLLoaderFactoryGetter::~URLLoaderFactoryGetter() {}

void URLLoaderFactoryGetter::InitializeOnIOThread(
    mojom::URLLoaderFactoryPtrInfo blob_factory) {
  blob_factory_.Bind(std::move(blob_factory));
}

void URLLoaderFactoryGetter::HandleNetworkFactoryRequestOnUIThread(
    mojom::URLLoaderFactoryRequest network_factory_request) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(partition_)
      << "URLLoaderFactoryGetter shouldn't outlive StoragePartition";
  partition_->GetNetworkContext()->CreateURLLoaderFactory(
      std::move(network_factory_request), 0);
}

}  // namespace content
