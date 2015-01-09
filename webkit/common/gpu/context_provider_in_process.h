// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_COMMON_GPU_CONTEXT_PROVIDER_IN_PROCESS_H_
#define WEBKIT_COMMON_GPU_CONTEXT_PROVIDER_IN_PROCESS_H_

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_checker.h"
#include "gpu/blink/webgraphicscontext3d_in_process_command_buffer_impl.h"
#include "webkit/common/gpu/context_provider_web_context.h"
#include "webkit/common/gpu/webkit_gpu_export.h"

namespace blink { class WebGraphicsContext3D; }

namespace webkit {
namespace gpu {
class GrContextForWebGraphicsContext3D;

class WEBKIT_GPU_EXPORT ContextProviderInProcess
    : NON_EXPORTED_BASE(public ContextProviderWebContext) {
 public:
  static scoped_refptr<ContextProviderInProcess> Create(
      scoped_ptr<gpu_blink::WebGraphicsContext3DInProcessCommandBufferImpl>
          context3d,
      const std::string& debug_name);

  // Uses default attributes for creating an offscreen context.
  static scoped_refptr<ContextProviderInProcess> CreateOffscreen(
      bool lose_context_when_out_of_memory);

  blink::WebGraphicsContext3D* WebContext3D() override;

  bool BindToCurrentThread() override;
  Capabilities ContextCapabilities() override;
  ::gpu::gles2::GLES2Interface* ContextGL() override;
  ::gpu::ContextSupport* ContextSupport() override;
  class GrContext* GrContext() override;
  bool IsContextLost() override;
  void VerifyContexts() override;
  void DeleteCachedResources() override;
  bool DestroyedOnMainThread() override;
  void SetLostContextCallback(
      const LostContextCallback& lost_context_callback) override;
  void SetMemoryPolicyChangedCallback(
      const MemoryPolicyChangedCallback& memory_policy_changed_callback)
      override;

 protected:
  ContextProviderInProcess(
      scoped_ptr<gpu_blink::WebGraphicsContext3DInProcessCommandBufferImpl>
          context3d,
      const std::string& debug_name);
  ~ContextProviderInProcess() override;

  void OnLostContext();

 private:
  void InitializeCapabilities();

  base::ThreadChecker main_thread_checker_;
  base::ThreadChecker context_thread_checker_;

  scoped_ptr<gpu_blink::WebGraphicsContext3DInProcessCommandBufferImpl>
      context3d_;
  scoped_ptr<webkit::gpu::GrContextForWebGraphicsContext3D> gr_context_;

  LostContextCallback lost_context_callback_;

  base::Lock destroyed_lock_;
  bool destroyed_;

  std::string debug_name_;
  class LostContextCallbackProxy;
  scoped_ptr<LostContextCallbackProxy> lost_context_callback_proxy_;

  cc::ContextProvider::Capabilities capabilities_;

  DISALLOW_COPY_AND_ASSIGN(ContextProviderInProcess);
};

}  // namespace gpu
}  // namespace webkit

#endif  // WEBKIT_COMMON_GPU_CONTEXT_PROVIDER_IN_PROCESS_H_
