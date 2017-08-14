// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_SERVICE_WORKER_SERVICE_WORKER_PROVIDER_CONTEXT_H_
#define CONTENT_CHILD_SERVICE_WORKER_SERVICE_WORKER_PROVIDER_CONTEXT_H_

#include <memory>
#include <set>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner_helpers.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_provider_interfaces.mojom.h"
#include "content/common/service_worker/service_worker_types.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerProviderClient.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace content {

class ServiceWorkerHandleReference;
class ServiceWorkerRegistrationHandleReference;
struct ServiceWorkerProviderContextDeleter;
class ThreadSafeSender;

// ServiceWorkerProviderContext has different roles depending on if it's for a
// "controllee" (a Document or Worker execution context), or a "controller" (a
// service worker execution context). The roles are described below.
//
// WebServiceWorkerProviderImpl has a ServiceWorkerProviderContext.  Because
// WebServiceWorkerProviderImpl is used for both controllees and controllers,
// this class allows WebServiceWorkerProviderImpl to implement different
// behaviors for controllees vs controllers.
//
// Created and destructed on the main thread. Unless otherwise noted, all
// methods are called on the main thread. The lifetime of this class is equals
// to the corresponding ServiceWorkerNetworkProvider.
//
// The role of this class varies for controllees and controllers:
//  - For controllees, this is used for keeping the associated registration and
//    the controller alive to create controllee's ServiceWorkerContainer. The
//    references to them are kept until OnDisassociateRegistration() is called
//    or OnSetControllerServiceWorker() is called with an invalid worker info.
//  - For controllers, this is used for keeping the associated registration and
//    its versions alive to create controller's ServiceWorkerGlobalScope. The
//    references to them are kept until OnDisassociateRegistration() is called.
//
// These operations are actually done in delegate classes owned by this class:
// ControlleeDelegate and ControllerDelegate.
class CONTENT_EXPORT ServiceWorkerProviderContext
    : public base::RefCountedThreadSafe<ServiceWorkerProviderContext,
                                        ServiceWorkerProviderContextDeleter>,
      NON_EXPORTED_BASE(public mojom::ServiceWorkerProvider) {
 public:
  // |provider_id| specifies which host will receive the message from this
  // provider. |provider_type| changes the behavior of this provider
  // context. |request| is an endpoint which is connected to
  // content::ServiceWorkerProviderHost which notifies changes of the
  // registration's and workers' status. |request| is bound with |binding_|.
  ServiceWorkerProviderContext(
      int provider_id,
      ServiceWorkerProviderType provider_type,
      mojom::ServiceWorkerProviderAssociatedRequest request,
      ThreadSafeSender* thread_safe_sender);

  // Implementation of mojom::ServiceWorkerProvider
  void AssociateRegistration(
      const ServiceWorkerRegistrationObjectInfo& info,
      const ServiceWorkerVersionAttributes& attrs) override;
  void DisassociateRegistration() override;
  void SetControllerServiceWorker(const ServiceWorkerObjectInfo& info,
                                  const std::vector<uint32_t>& used_features,
                                  bool should_notify_controllerchange) override;
  void MessageToDocument(mojom::MessageToDocumentParamsPtr params) override;

  // Called on the worker thread. Used for initializing
  // ServiceWorkerGlobalScope.
  void GetAssociatedRegistration(ServiceWorkerRegistrationObjectInfo* info,
                                 ServiceWorkerVersionAttributes* attrs);

  // May be called on the main or worker thread.
  bool HasAssociatedRegistration();

  void set_provider_client(blink::WebServiceWorkerProviderClient* client) {
    provider_client_ = client;
  }

  int provider_id() const { return provider_id_; }

  ServiceWorkerHandleReference* controller();
  void CountFeature(uint32_t feature);
  const std::set<uint32_t>& used_features() const { return used_features_; }

 private:
  friend class base::DeleteHelper<ServiceWorkerProviderContext>;
  friend class base::RefCountedThreadSafe<ServiceWorkerProviderContext,
                                          ServiceWorkerProviderContextDeleter>;
  friend struct ServiceWorkerProviderContextDeleter;

  class Delegate;
  class ControlleeDelegate;
  class ControllerDelegate;

  ~ServiceWorkerProviderContext() override;
  void DestructOnMainThread() const;

  // Assumes that the given object information retains an interprocess handle
  // reference passed from the browser process, and adopts it.
  std::unique_ptr<ServiceWorkerRegistrationHandleReference> Adopt(
      const ServiceWorkerRegistrationObjectInfo& info);
  std::unique_ptr<ServiceWorkerHandleReference> Adopt(
      const ServiceWorkerObjectInfo& info);

  const int provider_id_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  scoped_refptr<ThreadSafeSender> thread_safe_sender_;
  // Mojo binding for the |request| passed to the constructor. This keeps the
  // connection to the content::ServiceWorkerProviderHost in the browser process
  // alive.
  mojo::AssociatedBinding<mojom::ServiceWorkerProvider> binding_;

  std::unique_ptr<Delegate> delegate_;

  blink::WebServiceWorkerProviderClient* provider_client_;

  // Only used for controllee contexts.
  std::set<uint32_t> used_features_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerProviderContext);
};

struct ServiceWorkerProviderContextDeleter {
  static void Destruct(const ServiceWorkerProviderContext* context) {
    context->DestructOnMainThread();
  }
};

}  // namespace content

#endif  // CONTENT_CHILD_SERVICE_WORKER_SERVICE_WORKER_PROVIDER_CONTEXT_H_
