// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/shared_url_loader_factory.h"

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"

namespace content {

namespace {

// Wraps a SingleRequestLoader callback as a SharedURLLoaderFactory with some
// built-in sequence safety. The factory may only invoke
// |CreateLoaderAndStart()| at most once, and when invoked the wrapped
// SingleRequestLoader will be run on the same sequence on which this factory
// was constructed.
class SingleRequestLoaderWrapperFactory : public SharedURLLoaderFactory {
 public:
  class LoaderHolder : public base::RefCountedThreadSafe<LoaderHolder> {
   public:
    LoaderHolder(SingleRequestLoader loader,
                 scoped_refptr<base::SequencedTaskRunner> loader_sequence)
        : loader_(std::move(loader)),
          loader_sequence_(std::move(loader_sequence)) {}

    void StartLoad(network::mojom::URLLoaderRequest loader_request,
                   network::mojom::URLLoaderClientPtrInfo client_info) {
      if (!loader_sequence_->RunsTasksInCurrentSequence()) {
        loader_sequence_->PostTask(
            FROM_HERE,
            base::BindOnce(&LoaderHolder::StartLoad, this,
                           std::move(loader_request), std::move(client_info)));
        return;
      }

      DCHECK(loader_);
      std::move(loader_).Run(
          std::move(loader_request),
          network::mojom::URLLoaderClientPtr(std::move(client_info)));
    }

   private:
    friend class base::RefCountedThreadSafe<LoaderHolder>;

    ~LoaderHolder() {
      if (loader_) {
        loader_sequence_->PostTask(
            FROM_HERE, base::BindOnce(&DropLoader, std::move(loader_)));
      }
    }

    static void DropLoader(SingleRequestLoader loader) {}

    SingleRequestLoader loader_;
    const scoped_refptr<base::SequencedTaskRunner> loader_sequence_;

    DISALLOW_COPY_AND_ASSIGN(LoaderHolder);
  };

  class FactoryInfo : public SharedURLLoaderFactoryInfo {
   public:
    explicit FactoryInfo(scoped_refptr<LoaderHolder> loader_holder)
        : loader_holder_(std::move(loader_holder)) {}
    ~FactoryInfo() override = default;

    // SharedURLLoaderFactoryInfo:
    scoped_refptr<SharedURLLoaderFactory> CreateFactory() override {
      DCHECK(loader_holder_);
      return base::MakeRefCounted<SingleRequestLoaderWrapperFactory>(
          std::move(loader_holder_));
    }

   private:
    scoped_refptr<LoaderHolder> loader_holder_;

    DISALLOW_COPY_AND_ASSIGN(FactoryInfo);
  };

  explicit SingleRequestLoaderWrapperFactory(SingleRequestLoader loader)
      : SingleRequestLoaderWrapperFactory(base::MakeRefCounted<LoaderHolder>(
            std::move(loader),
            base::SequencedTaskRunnerHandle::Get())) {}

  explicit SingleRequestLoaderWrapperFactory(
      scoped_refptr<LoaderHolder> loader_holder)
      : loader_holder_(std::move(loader_holder)) {}

  // SharedURLLoaderFactory:
  void CreateLoaderAndStart(
      network::mojom::URLLoaderRequest loader,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      network::mojom::URLLoaderClientPtr client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      const Constraints& constraints) override {
    loader_holder_->StartLoad(std::move(loader), client.PassInterface());
  }

  std::unique_ptr<SharedURLLoaderFactoryInfo> Clone() override {
    return std::make_unique<FactoryInfo>(loader_holder_);
  }

 private:
  ~SingleRequestLoaderWrapperFactory() override = default;

  const scoped_refptr<LoaderHolder> loader_holder_;

  DISALLOW_COPY_AND_ASSIGN(SingleRequestLoaderWrapperFactory);
};

}  // namespace

// static
const SharedURLLoaderFactory::Constraints
    SharedURLLoaderFactory::kDefaultConstraints;

SharedURLLoaderFactoryInfo::SharedURLLoaderFactoryInfo() = default;

SharedURLLoaderFactoryInfo::~SharedURLLoaderFactoryInfo() = default;

// static
scoped_refptr<SharedURLLoaderFactory> SharedURLLoaderFactory::Create(
    std::unique_ptr<SharedURLLoaderFactoryInfo> info) {
  return info->CreateFactory();
}

// static
scoped_refptr<SharedURLLoaderFactory>
SharedURLLoaderFactory::CreateForSingleRequest(SingleRequestLoader loader) {
  return base::MakeRefCounted<SingleRequestLoaderWrapperFactory>(
      std::move(loader));
}

SharedURLLoaderFactory::~SharedURLLoaderFactory() = default;

}  // namespace content
