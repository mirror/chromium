// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/gles2_conform_support/egl/context.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "gpu/command_buffer/client/gles2_lib.h"
#include "gpu/command_buffer/client/transfer_buffer.h"
#include "gpu/command_buffer/common/value_state.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/command_buffer/service/memory_tracking.h"
#include "gpu/command_buffer/service/transfer_buffer_manager.h"
#include "gpu/command_buffer/service/valuebuffer_manager.h"
#include "gpu/gles2_conform_support/egl/config.h"
#include "gpu/gles2_conform_support/egl/display.h"
#include "gpu/gles2_conform_support/egl/surface.h"
#include "gpu/gles2_conform_support/egl/thread_state.h"

// The slight complexification in this file comes from following properties:
// 1) Command buffer connection (context) can not be established without a
// GLSurface. EGL Context can be created independent of a surface.  This is why
// the connection is created only during first MakeCurrent.
// 2) Command buffer MakeCurrent calls need the real gl context and surface be
// current.
// 3) Client can change real EGL context behind the scenes and then still expect
// command buffer MakeCurrent re-set the command buffer context. This is why all
// MakeCurrent calls must actually reset the real context, even though command
// buffer current context does not change.
// 4) EGL context can be destroyed without surface, but command buffer would
// need the surface to run various cleanups. If context is destroyed
// surfaceless, the context is marked lost before destruction. This is avoided
// if possible, since command buffer at the time of writing prints out debug
// text in this case.

namespace {
const int32_t kCommandBufferSize = 1024 * 1024;
const int32_t kTransferBufferSize = 512 * 1024;
const bool kBindGeneratesResources = true;
const bool kLoseContextWhenOutOfMemory = false;
const bool kSupportClientSideArrays = true;
}

namespace egl {
Context::Context(Display* display, const Config* config)
    : display_(display),
      config_(config),
      is_current_in_some_thread_(false),
      is_destroyed_(false) {}

Context::~Context() {
  // We might not have a surface, so we must lose the context.  Cleanup will
  // execute GL commands otherwise. TODO: if shared contexts are ever
  // implemented, this will leak the GL resources. For pbuffer contexts, one
  // could track the last current surface or create a surface for destroying
  // purposes only. Other option would be to make the service usable without
  // surface.
  if (HasService()) {
    if (!WasServiceContextLost())
      MarkServiceContextLost();
    DestroyService();
  }
}

void Context::MarkDestroyed() {
  is_destroyed_ = true;
}

void Context::FlushAndSwapBuffers(gfx::GLSurface* current_surface) {
  DCHECK(HasService() && is_current_in_some_thread_);
  if (!Flush(current_surface))
    return;
  current_surface->SwapBuffers();
}

bool Context::MakeCurrent(Context* current_context,
                          gfx::GLSurface* current_surface,
                          Context* new_context,
                          gfx::GLSurface* new_surface) {
  if (!new_context && !current_context) {
    return true;
  }

  bool cleanup_old_current_context = false;
  if (current_context) {
    if (current_context->Flush(current_surface))
      cleanup_old_current_context = new_context != current_context;
  }

  if (new_context) {
    if (!new_context->IsCompatibleSurface(new_surface))
      return false;

    if (new_context->HasService()) {
      if (new_context->WasServiceContextLost())
        return false;
      if (new_context != current_context) {
        // If Flush did not set the current context, set it now. Otherwise
        // calling into the decoder is not ok.
        if (!new_context->gl_context_->MakeCurrent(new_surface)) {
          new_context->MarkServiceContextLost();
          return false;
        }
      }
      if (new_context != current_context || new_surface != current_surface)
        new_context->decoder_->SetSurface(new_surface);
      if (!new_context->decoder_->MakeCurrent()) {
        new_context->MarkServiceContextLost();
        return false;
      }
    } else {
      if (!new_context->CreateService(new_surface)) {
        return false;
      }
    }
  }

  // The current_surface will be released when MakeCurrent succeeds.
  // Cleanup in this case only.
  if (cleanup_old_current_context) {
    if (current_context->is_destroyed_ && current_surface != new_surface) {
      current_context->gl_context_->MakeCurrent(current_surface);
      // If we are releasing the context and we have one ref, it means that the
      // ref will be lost and the object will be destroyed.  Destroy the service
      // explicitly here, so that cleanup can happen and client GL
      // implementation does not print errors.
      current_context->DestroyService();
    } else {
      current_context->decoder_->ReleaseSurface();
    }
  }

  return true;
}

bool Context::ValidateAttributeList(const EGLint* attrib_list) {
  if (attrib_list) {
    for (int i = 0; attrib_list[i] != EGL_NONE; attrib_list += 2) {
      switch (attrib_list[i]) {
        case EGL_CONTEXT_CLIENT_VERSION:
          break;
        default:
          return false;
      }
    }
  }
  return true;
}

void Context::SetGpuControlClient(gpu::GpuControlClient*) {
  // The client is not currently called, so don't store it.
}

gpu::Capabilities Context::GetCapabilities() {
  return decoder_->GetCapabilities();
}

int32_t Context::CreateImage(ClientBuffer buffer,
                             size_t width,
                             size_t height,
                             unsigned internalformat) {
  NOTIMPLEMENTED();
  return -1;
}

void Context::DestroyImage(int32_t id) {
  NOTIMPLEMENTED();
}

int32_t Context::CreateGpuMemoryBufferImage(size_t width,
                                            size_t height,
                                            unsigned internalformat,
                                            unsigned usage) {
  NOTIMPLEMENTED();
  return -1;
}

void Context::SignalQuery(uint32_t query, const base::Closure& callback) {
  NOTIMPLEMENTED();
}

void Context::SetLock(base::Lock*) {
  NOTIMPLEMENTED();
}

bool Context::IsGpuChannelLost() {
  NOTIMPLEMENTED();
  return false;
}

void Context::EnsureWorkVisible() {
  // This is only relevant for out-of-process command buffers.
}

gpu::CommandBufferNamespace Context::GetNamespaceID() const {
  return gpu::CommandBufferNamespace::IN_PROCESS;
}

gpu::CommandBufferId Context::GetCommandBufferID() const {
  return gpu::CommandBufferId();
}

int32_t Context::GetExtraCommandBufferData() const {
  return 0;
}

uint64_t Context::GenerateFenceSyncRelease() {
  return display_->GenerateFenceSyncRelease();
}

bool Context::IsFenceSyncRelease(uint64_t release) {
  return display_->IsFenceSyncRelease(release);
}

bool Context::IsFenceSyncFlushed(uint64_t release) {
  return display_->IsFenceSyncFlushed(release);
}

bool Context::IsFenceSyncFlushReceived(uint64_t release) {
  return display_->IsFenceSyncFlushReceived(release);
}

void Context::SignalSyncToken(const gpu::SyncToken& sync_token,
                              const base::Closure& callback) {
  NOTIMPLEMENTED();
}

bool Context::CanWaitUnverifiedSyncToken(const gpu::SyncToken* sync_token) {
  return false;
}

void Context::ApplyCurrentContext(gfx::GLSurface* current_surface) {
  DCHECK(HasService());
  // The current_surface will be the same as
  // the surface of the decoder. We can not DCHECK as there is
  // no accessor.
  if (!WasServiceContextLost()) {
    if (!gl_context_->MakeCurrent(current_surface))
      MarkServiceContextLost();
  }
  gles2::SetGLContext(client_gl_context_.get());
}

void Context::ApplyContextReleased() {
  gles2::SetGLContext(nullptr);
}

bool Context::CreateService(gfx::GLSurface* gl_surface) {
  scoped_refptr<gpu::TransferBufferManager> transfer_buffer_manager(
      new gpu::TransferBufferManager(nullptr));
  transfer_buffer_manager->Initialize();

  scoped_ptr<gpu::CommandBufferService> command_buffer(
      new gpu::CommandBufferService(transfer_buffer_manager.get()));
  if (!command_buffer->Initialize())
    return false;

  scoped_refptr<gpu::gles2::ContextGroup> group(new gpu::gles2::ContextGroup(
      gpu_preferences_, nullptr, nullptr,
      new gpu::gles2::ShaderTranslatorCache(gpu_preferences_),
      new gpu::gles2::FramebufferCompletenessCache, nullptr, nullptr, nullptr,
      true));

  scoped_ptr<gpu::gles2::GLES2Decoder> decoder(
      gpu::gles2::GLES2Decoder::Create(group.get()));
  if (!decoder.get())
    return false;

  scoped_ptr<gpu::CommandExecutor> command_executor(new gpu::CommandExecutor(
      command_buffer.get(), decoder.get(), decoder.get()));

  decoder->set_engine(command_executor.get());

  scoped_refptr<gfx::GLContext> gl_context(gfx::GLContext::CreateGLContext(
      nullptr, gl_surface, gfx::PreferDiscreteGpu));
  if (!gl_context)
    return false;

  gl_context->MakeCurrent(gl_surface);

  gpu::gles2::ContextCreationAttribHelper helper;
  config_->GetAttrib(EGL_ALPHA_SIZE, &helper.alpha_size);
  config_->GetAttrib(EGL_BLUE_SIZE, &helper.blue_size);
  config_->GetAttrib(EGL_GREEN_SIZE, &helper.green_size);
  config_->GetAttrib(EGL_RED_SIZE, &helper.red_size);
  config_->GetAttrib(EGL_DEPTH_SIZE, &helper.depth_size);
  config_->GetAttrib(EGL_STENCIL_SIZE, &helper.stencil_size);
  config_->GetAttrib(EGL_SAMPLES, &helper.samples);
  config_->GetAttrib(EGL_SAMPLE_BUFFERS, &helper.sample_buffers);

  helper.buffer_preserved = false;
  helper.bind_generates_resource = kBindGeneratesResources;
  helper.fail_if_major_perf_caveat = false;
  helper.lose_context_when_out_of_memory = kLoseContextWhenOutOfMemory;
  helper.context_type = gpu::gles2::CONTEXT_TYPE_OPENGLES2;
  std::vector<int32_t> attribs;
  helper.Serialize(&attribs);

  if (!decoder->Initialize(gl_surface, gl_context.get(),
                           gl_surface->IsOffscreen(), gl_surface->GetSize(),
                           gpu::gles2::DisallowedFeatures(), attribs)) {
    return false;
  }

  command_buffer->SetPutOffsetChangeCallback(
      base::Bind(&gpu::CommandExecutor::PutChanged,
                 base::Unretained(command_executor.get())));
  command_buffer->SetGetBufferChangeCallback(
      base::Bind(&gpu::CommandExecutor::SetGetBuffer,
                 base::Unretained(command_executor.get())));

  scoped_ptr<gpu::gles2::GLES2CmdHelper> gles2_cmd_helper(
      new gpu::gles2::GLES2CmdHelper(command_buffer.get()));
  if (!gles2_cmd_helper->Initialize(kCommandBufferSize)) {
    decoder->Destroy(true);
    return false;
  }

  scoped_ptr<gpu::TransferBuffer> transfer_buffer(
      new gpu::TransferBuffer(gles2_cmd_helper.get()));

  gles2_cmd_helper_.reset(gles2_cmd_helper.release());
  transfer_buffer_.reset(transfer_buffer.release());
  command_buffer_.reset(command_buffer.release());
  command_executor_.reset(command_executor.release());
  decoder_.reset(decoder.release());
  gl_context_ = gl_context.get();

  scoped_ptr<gpu::gles2::GLES2Implementation> context(
      new gpu::gles2::GLES2Implementation(
          gles2_cmd_helper_.get(), nullptr, transfer_buffer_.get(),
          kBindGeneratesResources, kLoseContextWhenOutOfMemory,
          kSupportClientSideArrays, this));

  if (!context->Initialize(kTransferBufferSize, kTransferBufferSize / 2,
                           kTransferBufferSize * 2,
                           gpu::gles2::GLES2Implementation::kNoLimit)) {
    DestroyService();
    return false;
  }

  context->EnableFeatureCHROMIUM("pepper3d_allow_buffers_on_multiple_targets");
  context->EnableFeatureCHROMIUM("pepper3d_support_fixed_attribs");
  client_gl_context_.reset(context.release());
  return true;
}

void Context::DestroyService() {
  DCHECK(HasService());
  bool have_context = !WasServiceContextLost();
  // The client gl interface might still be set to current global
  // interface. This will be cleaned up in ApplyContextReleased
  // with AutoCurrentContextRestore.
  client_gl_context_.reset();
  gl_context_ = nullptr;

  transfer_buffer_.reset();
  command_executor_.reset();
  if (decoder_)
    decoder_->Destroy(have_context);
  gles2_cmd_helper_.reset();
  command_buffer_.reset();
}

bool Context::HasService() const {
  return decoder_ != nullptr;
}

void Context::MarkServiceContextLost() {
  decoder_->MarkContextLost(gpu::error::kMakeCurrentFailed);
}

bool Context::WasServiceContextLost() const {
  return decoder_->WasContextLost();
}

bool Context::IsCompatibleSurface(gfx::GLSurface* gl_surface) {
  EGLint value = EGL_NONE;
  config_->GetAttrib(EGL_SURFACE_TYPE, &value);
  bool config_is_offscreen = (value & EGL_PBUFFER_BIT) != 0;
  return gl_surface->IsOffscreen() == config_is_offscreen;
}

bool Context::Flush(gfx::GLSurface* gl_surface) {
  if (WasServiceContextLost())
    return false;
  if (!gl_context_->MakeCurrent(gl_surface)) {
    MarkServiceContextLost();
    return false;
  }
  client_gl_context_->Flush();
  return true;
}

}  // namespace egl
