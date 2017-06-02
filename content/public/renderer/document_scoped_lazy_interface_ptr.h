// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_DOCUMENT_SCOPED_LAZY_INTERFACE_PTR_H_
#define CONTENT_PUBLIC_RENDERER_DOCUMENT_SCOPED_LAZY_INTERFACE_PTR_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace content {

// A template helper for an InterfacePtr lazily acquired from a RenderFrame's
// remote interface provider. The InterfacePtr is strictly document-scoped,
// meaning it is automatically reset on new-document navigation so that
// subsequent dereferences of this object will reacquire the interface.
//
// If the associated RenderFrame is destroyed at any point, this permanently
// starts operating as a dead-end InterfacePtr which is safe to call into but
// whose messages are ultimately routed into the abyss.
//
// It is always safe to dereference this object to make interface calls, with
// the guarantee that those calls will either be routed to the correct
// per-document interface endpoint, or to nowhere at all if the frame is on its
// way out of existence.
template <typename Interface>
class DocumentScopedLazyInterfacePtr {
 public:
  // This must outlive |frame|. Typically instances of
  // DocumentScopedLazyInterfacePtr are owned by other RenderFrameObservers.
  explicit DocumentScopedLazyInterfacePtr(RenderFrame* frame)
      : weak_factory_(this) {
    if (!frame)
      BindDeadProxy();
    else
      observer_ = new FrameObserver(frame, weak_factory_.GetWeakPtr());
  }

  ~DocumentScopedLazyInterfacePtr() {
    if (observer_ && observer_->render_frame()) {
      // If we're being destroyed while our observer is still alive and well, we
      // can go ahead proactively and remove the observer. Note that if
      // |observer_| is non-null but |observer_->render_frame()| is already
      // null, we do nothing because we must be within the stack of this render
      // frame's destructor, and therefore |observer_| is about to be deleted
      // anyway.
      delete observer_;
    }
  }

  Interface* get() { return GetPtr().get(); }
  Interface* operator->() { return GetPtr().get(); }

  mojo::InterfacePtr<Interface>& GetPtr() {
    if (!proxy_) {
      DCHECK(observer_ && observer_->render_frame());
      observer_->render_frame()->GetRemoteInterfaces()->GetInterface(&proxy_);
    }
    return proxy_;
  }

  // Vends a WeakPtr to the Interface for the currently bound proxy. The WeakPtr
  // will remain valid as long as the current document remains alive. Any
  // new-document navigation will invalidate all previously vended WeakPtrs.
  base::WeakPtr<Interface> GetWeakProxy() {
    auto& proxy = GetPtr();
    if (!weak_proxy_factory_.has_value())
      weak_proxy_factory_.emplace(proxy.get());
    return weak_proxy_factory_->GetWeakPtr();
  }

  // Overrides the proxy used by this object. Beware that if this object is
  // constructed over a valid RenderFrame (as opposed to nullptr), a navigation
  // of that frame will still trample this proxy.
  void SetProxyForTesting(mojo::InterfacePtr<Interface> proxy) {
    Reset();
    proxy_ = std::move(proxy);
  }

 private:
  // An observer for this InterfacePtr. Resets the internal state on navigation.
  // The DocumentScopedLazyInterfacePtr is not a RFO itself because it's
  // typically owned by some other RFO, and there are subtle issues with having
  // one RFO own another outright.
  class FrameObserver : public RenderFrameObserver {
   public:
    FrameObserver(
        RenderFrame* frame,
        base::WeakPtr<DocumentScopedLazyInterfacePtr<Interface>> weak_interface)
        : RenderFrameObserver(frame), weak_interface_(weak_interface) {}
    ~FrameObserver() override {}

   private:
    // RenderFrameObserver:
    void OnDestruct() override {
      if (weak_interface_) {
        weak_interface_->observer_ = nullptr;
        weak_interface_->BindDeadProxy();
      }

      delete this;
    }

    void DidCommitProvisionalLoad(bool is_new_navigation,
                                  bool is_same_document_navigation) override {
      if (!weak_interface_)
        delete this;
      else if (!is_same_document_navigation)
        weak_interface_->Reset();
    }

    base::WeakPtr<DocumentScopedLazyInterfacePtr<Interface>> weak_interface_;

    DISALLOW_COPY_AND_ASSIGN(FrameObserver);
  };

  // Permanently bind to a dead-end proxy which remains safe to call into.
  void BindDeadProxy() {
    Reset();
    mojo::MakeRequest(&proxy_);
  }

  void Reset() {
    weak_proxy_factory_.reset();
    proxy_.reset();
  }

  // The observer watching the RenderFrame on behalf of this interface. May be
  // null if the frame was never valid or has been destroyed. Not strictly
  // owned, but may be deleted in our own destructor under certain conditions.
  FrameObserver* observer_ = nullptr;

  mojo::InterfacePtr<Interface> proxy_;
  base::Optional<base::WeakPtrFactory<Interface>> weak_proxy_factory_;
  base::WeakPtrFactory<DocumentScopedLazyInterfacePtr<Interface>> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DocumentScopedLazyInterfacePtr);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_DOCUMENT_SCOPED_LAZY_INTERFACE_PTR_H_
