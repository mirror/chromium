// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_SERVICE_WORKER_WEB_SERVICE_WORKER_REGISTRATION_IMPL_H_
#define CONTENT_CHILD_SERVICE_WORKER_WEB_SERVICE_WORKER_REGISTRATION_IMPL_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerRegistration.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/service_worker_registration.mojom.h"

namespace blink {
class WebServiceWorkerRegistrationProxy;
}

namespace content {

class WebServiceWorkerImpl;

// Each instance corresponds to one ServiceWorkerRegistration object in JS
// context, and is held by ServiceWorkerRegistration object in Blink's C++ layer
// via WebServiceWorkerRegistration::Handle.
//
// Each instance is created at the first time being connected with corresponding
// ServiceWorkerRegistrationHandle  instance in the browser process, is
// destroyed when the |binding_| Mojo connection is closed due to destruction of
// corresponding ServiceWorkerRegistrationHandle. During its lifetime, even if
// no any blink::WebServiceWorkerRegistration::Handle holding it, it still keeps
// alive waiting to be reused next time, but with a nullptr |info_|.
//
// Each instance holds one mojo connection of interface
// blink::mojom::ServiceWorkerRegistrationObjectHost inside |info_|, so that
// corresponding ServiceWorkerRegistrationHandle doesn't go away in the browser
// process while the ServiceWorkerRegistration object is alive.
class CONTENT_EXPORT WebServiceWorkerRegistrationImpl
    : public blink::mojom::ServiceWorkerRegistrationObject,
      public blink::WebServiceWorkerRegistration,
      public base::RefCounted<WebServiceWorkerRegistrationImpl,
                              WebServiceWorkerRegistrationImpl> {
 public:
  explicit WebServiceWorkerRegistrationImpl(
      blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info);

  void Attach(blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info);

  void SetInstalling(const scoped_refptr<WebServiceWorkerImpl>& service_worker);
  void SetWaiting(const scoped_refptr<WebServiceWorkerImpl>& service_worker);
  void SetActive(const scoped_refptr<WebServiceWorkerImpl>& service_worker);

  void OnUpdateFound();

  // blink::WebServiceWorkerRegistration overrides.
  void SetProxy(blink::WebServiceWorkerRegistrationProxy* proxy) override;
  blink::WebServiceWorkerRegistrationProxy* Proxy() override;
  blink::WebURL Scope() const override;
  void Update(
      blink::WebServiceWorkerProvider* provider,
      std::unique_ptr<WebServiceWorkerUpdateCallbacks> callbacks) override;
  void Unregister(blink::WebServiceWorkerProvider* provider,
                  std::unique_ptr<WebServiceWorkerUnregistrationCallbacks>
                      callbacks) override;
  void EnableNavigationPreload(
      bool enable,
      blink::WebServiceWorkerProvider* provider,
      std::unique_ptr<WebEnableNavigationPreloadCallbacks> callbacks) override;
  void GetNavigationPreloadState(
      blink::WebServiceWorkerProvider* provider,
      std::unique_ptr<WebGetNavigationPreloadStateCallbacks> callbacks)
      override;
  void SetNavigationPreloadHeader(
      const blink::WebString& value,
      blink::WebServiceWorkerProvider* provider,
      std::unique_ptr<WebSetNavigationPreloadHeaderCallbacks> callbacks)
      override;
  int64_t RegistrationId() const override;

  using WebServiceWorkerRegistrationHandle =
      blink::WebServiceWorkerRegistration::Handle;

  // Creates WebServiceWorkerRegistrationHandle object that owns a reference to
  // the given WebServiceWorkerRegistrationImpl object.
  static std::unique_ptr<WebServiceWorkerRegistrationHandle> CreateHandle(
      scoped_refptr<WebServiceWorkerRegistrationImpl> registration);

 private:
  friend class base::RefCounted<WebServiceWorkerRegistrationImpl,
                                WebServiceWorkerRegistrationImpl>;
  ~WebServiceWorkerRegistrationImpl() override;

  // RefCounted traits implementation, rather than delete |x| directly, calls
  // |x->DetachAndMaybeDestroy()| to notify that the last reference to it has
  // gone away.
  static void Destruct(const WebServiceWorkerRegistrationImpl* x);

  enum QueuedTaskType {
    INSTALLING,
    WAITING,
    ACTIVE,
    UPDATE_FOUND,
  };

  struct QueuedTask {
    QueuedTask(QueuedTaskType type,
               const scoped_refptr<WebServiceWorkerImpl>& worker);
    QueuedTask(const QueuedTask& other);
    ~QueuedTask();
    QueuedTaskType type;
    scoped_refptr<WebServiceWorkerImpl> worker;
  };

  void RunQueuedTasks();
  void DetachAndMaybeDestroy();
  void OnConnectionError();

  // This is the key to map with remote
  // content::ServiceWorkerRegistrationHandle.
  const int handle_id_;
  // This is initialized by the contructor with |info| passed from the remote
  // content::ServiceWorkerRegistrationHandle just created in the browser
  // process. |info_| will be reset to nullptr by DetachAndMaybeDestroy() when
  // there is no any blink::WebServiceWorkerRegistration::Handle referencing
  // |this|. After that if another Mojo connection from the same remote
  // content::ServiceWorkerRegistrationHandle is passed here again(e.g.
  // WebServiceWorkerProviderImpl::OnDidGetRegistration()), |info_| will be set
  // valid value again by Attach().
  // |info_->host_ptr_info| holds a Mojo connection end point retaining an
  // reference to the remote content::ServiceWorkerRegistrationHandle to control
  // its lifetime.
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info_;
  blink::WebServiceWorkerRegistrationProxy* proxy_;

  // This keeps the Mojo binding to serve its other Mojo endpoint(i.e. the
  // caller end) held by the content::ServiceWorkerRegistrationHandle in the
  // browser process, is bound with |info_->request| by the constructor.
  // This also controls lifetime of |this|, its connection error handler will
  // delete |this|.
  mojo::AssociatedBinding<blink::mojom::ServiceWorkerRegistrationObject>
      binding_;

  std::vector<QueuedTask> queued_tasks_;

  DISALLOW_COPY_AND_ASSIGN(WebServiceWorkerRegistrationImpl);
};

}  // namespace content

#endif  // CONTENT_CHILD_SERVICE_WORKER_WEB_SERVICE_WORKER_REGISTRATION_IMPL_H_
