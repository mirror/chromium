// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MOJO_BLINK_INTERFACE_PROVIDER_IMPL_H_
#define CONTENT_RENDERER_MOJO_BLINK_INTERFACE_PROVIDER_IMPL_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "third_party/WebKit/public/platform/InterfaceProvider.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace service_manager {
class Connector;
}

namespace content {

class RenderFrame;

// An implementation of blink::InterfaceProvider that forwards to a
// service_manager::InterfaceProvider. In general this class is not thread-safe,
// but GetInterface() is safe to call from any thread.
class BlinkInterfaceProviderImpl : public blink::InterfaceProvider {
 public:
  // Creates a BlinkInterfaceProviderImpl which wraps a Connector. Calls to
  // GetInterface are plumbed through as BindInterface requests against
  // content_browser.
  //
  // This interface should be considered deprecated in favor of document-scoped
  // interface requests (see below.)
  explicit BlinkInterfaceProviderImpl(
      base::WeakPtr<service_manager::Connector> connector);

  // Creates a BlinkInterfaceProviderImpl which is bound to a single frame.
  // Calls to GetInterface are sent to the browser over a pipe which is bound
  // in sync with navigation commits, ensuring that all interface requests sent
  // through this provider are scoped to a single document.
  //
  // |frame| MUST outlive this object.
  explicit BlinkInterfaceProviderImpl(RenderFrame* frame);

  ~BlinkInterfaceProviderImpl();

  // blink::InterfaceProvider override.
  void GetInterface(const char* name,
                    mojo::ScopedMessagePipeHandle handle) override;

 private:
  void GetInterfaceInternal(const std::string& name,
                            mojo::ScopedMessagePipeHandle handle);

  const base::WeakPtr<service_manager::Connector> connector_;
  RenderFrame* const frame_ = nullptr;

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;

  // Should only be accessed by Web Worker threads that are using the
  // blink::Platform-level interface provider.
  base::WeakPtr<BlinkInterfaceProviderImpl> weak_ptr_;
  base::WeakPtrFactory<BlinkInterfaceProviderImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BlinkInterfaceProviderImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MOJO_BLINK_INTERFACE_PROVIDER_IMPL_H_
