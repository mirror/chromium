// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/gpu/SharedGpuContext.h"

#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "platform/CrossThreadFunctional.h"
#include "platform/WaitableEvent.h"
#include "platform/WebTaskRunner.h"
#include "public/platform/Platform.h"
#include "public/platform/WebGraphicsContext3DProvider.h"

namespace blink {

SharedGpuContext* SharedGpuContext::GetInstanceForCurrentThread() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<SharedGpuContext>,
                                  thread_specific_instance, ());
  return thread_specific_instance;
}

SharedGpuContext::SharedGpuContext() {}

void SharedGpuContext::CreateContextProviderOnMainThread(
    WaitableEvent* waitable_event,
    bool* using_software_compositing) {
  DCHECK(IsMainThread());
  Platform::ContextAttributes context_attributes;
  context_attributes.web_gl_version = 1;  // GLES2
  Platform::GraphicsInfo graphics_info;

  // This has no share group, so it only fails if the GpuChannel is not
  // able to be established. In that case gpu compositing will be disabled. If
  // non-null then we can determine if software compositing is being used on
  // this thread.
  std::unique_ptr<WebGraphicsContext3DProvider> context_provider =
      Platform::Current()->CreateOffscreenGraphicsContext3DProvider(
          context_attributes, WebURL(), nullptr, &graphics_info);
  *using_software_compositing =
      !context_provider || context_provider->UsingSoftwareCompositing();
  SetContextProvider(std::move(context_provider));
  if (waitable_event)
    waitable_event->Signal();
}

WeakPtr<WebGraphicsContext3DProviderWrapper>
SharedGpuContext::ContextProviderWrapper(bool* using_software_compositing) {
  SharedGpuContext* this_ptr = GetInstanceForCurrentThread();
  this_ptr->CreateContextProviderIfNeeded(using_software_compositing);
  if (!this_ptr->context_provider_wrapper_)
    return nullptr;
  return this_ptr->context_provider_wrapper_->CreateWeakPtr();
}

void SharedGpuContext::SetContextProvider(
    std::unique_ptr<WebGraphicsContext3DProvider> context_provider) {
  if (context_provider) {
    context_provider_wrapper_ = WTF::WrapUnique(
        new WebGraphicsContext3DProviderWrapper(std::move(context_provider)));
  } else {
    context_provider_creation_failed_ = true;
  }
}

void SharedGpuContext::CreateContextProviderIfNeeded(
    bool* using_software_compositing) {
  // To prevent perpetual retries.
  if (context_provider_creation_failed_)
    return;

  if (context_provider_wrapper_ &&
      context_provider_wrapper_->ContextProvider()
              ->ContextGL()
              ->GetGraphicsResetStatusKHR() == GL_NO_ERROR)
    return;

  if (context_provider_factory_) {
    // This path should only be used in unit tests
    SetContextProvider(context_provider_factory_());
  } else if (IsMainThread()) {
    std::unique_ptr<WebGraphicsContext3DProvider> provider =
        blink::Platform::Current()
            ->CreateSharedOffscreenGraphicsContext3DProvider();
    *using_software_compositing =
        !provider || provider->UsingSoftwareCompositing();
    SetContextProvider(std::move(provider));
  } else {
    // This synchronous round-trip to the main thread is the reason why
    // SharedGpuContext encasulates the context provider: so we only have to do
    // this once per thread.
    WaitableEvent waitable_event;
    RefPtr<WebTaskRunner> task_runner =
        Platform::Current()->MainThread()->GetWebTaskRunner();
    task_runner->PostTask(
        BLINK_FROM_HERE,
        CrossThreadBind(&SharedGpuContext::CreateContextProviderOnMainThread,
                        CrossThreadUnretained(this),
                        CrossThreadUnretained(&waitable_event),
                        CrossThreadUnretained(using_software_compositing)));
    waitable_event.Wait();
    if (context_provider_wrapper_ &&
        !context_provider_wrapper_->ContextProvider()->BindToCurrentThread()) {
      context_provider_wrapper_ = nullptr;
      // In this case we don't know if software compositing will be used so
      // we assume not. Since the context is lost we'll have to make another
      // one to find out.
    }
  }
}

void SharedGpuContext::SetContextProviderFactoryForTesting(
    ContextProviderFactory factory) {
  SharedGpuContext* this_ptr = GetInstanceForCurrentThread();
  this_ptr->context_provider_wrapper_.reset();
  this_ptr->context_provider_factory_ = factory;
}

bool SharedGpuContext::IsValidWithoutRestoring() {
  SharedGpuContext* this_ptr = GetInstanceForCurrentThread();
  if (!this_ptr->context_provider_wrapper_)
    return false;
  return this_ptr->context_provider_wrapper_->ContextProvider()
             ->ContextGL()
             ->GetGraphicsResetStatusKHR() == GL_NO_ERROR;
}

bool SharedGpuContext::AllowSoftwareToAcceleratedCanvasUpgrade() {
  SharedGpuContext* this_ptr = GetInstanceForCurrentThread();
  // TODO(kbr): Should we use this information here?
  bool using_software_compositing;
  this_ptr->CreateContextProviderIfNeeded(&using_software_compositing);
  if (!this_ptr->context_provider_wrapper_)
    return false;
  return this_ptr->context_provider_wrapper_->ContextProvider()
      ->GetCapabilities()
      .software_to_accelerated_canvas_upgrade;
}

}  // blink
