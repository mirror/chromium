// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/raster_decoder.h"

#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/command_buffer/common/context_result.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/command_buffer/service/gl_utils.h"
#include "gpu/command_buffer/service/query_manager.h"
#include "gpu/command_buffer/service/shader_translator.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_surface.h"

namespace gpu {
namespace gles2 {

RasterDecoder::RasterDecoder(GLES2DecoderClient* client,
                             CommandBufferServiceBase* command_buffer_service,
                             Outputter* outputter,
                             ContextGroup* group)
    : GLES2Decoder(command_buffer_service, outputter),
      group_(group),
      weak_ptr_factory_(this) {}

RasterDecoder::~RasterDecoder() {}

base::WeakPtr<GLES2Decoder> RasterDecoder::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

gpu::ContextResult RasterDecoder::Initialize(
    const scoped_refptr<gl::GLSurface>& surface,
    const scoped_refptr<gl::GLContext>& context,
    bool offscreen,
    const DisallowedFeatures& disallowed_features,
    const ContextCreationAttribHelper& attrib_helper) {
  TRACE_EVENT0("gpu", "RasterDecoder::Initialize");
  DCHECK(context->IsCurrent(surface.get()));
  DCHECK(!context_.get());

  set_initialized();

  if (!offscreen) {
    return gpu::ContextResult::kFatalFailure;
  }

  if (group_->gpu_preferences().enable_gpu_debugging)
    set_debug(true);

  if (group_->gpu_preferences().enable_gpu_command_logging)
    set_log_commands(true);

  surface_ = surface;
  context_ = context;

  auto result =
      group_->Initialize(this, attrib_helper.context_type, disallowed_features);
  if (result != gpu::ContextResult::kSuccess) {
    group_ =
        nullptr;  // Must not destroy ContextGroup if it is not initialized.
    Destroy(true);
    return result;
  }
  CHECK_GL_ERROR();

  query_manager_.reset(new QueryManager(this, group_->feature_info()));

  return gpu::ContextResult::kSuccess;
}

void RasterDecoder::Destroy(bool have_context) {
  if (query_manager_.get()) {
    query_manager_->Destroy(have_context);
    query_manager_.reset();
  }
}

void RasterDecoder::SetSurface(const scoped_refptr<gl::GLSurface>& surface) {
  NOTIMPLEMENTED();
}
void RasterDecoder::ReleaseSurface() {
  NOTIMPLEMENTED();
}

void RasterDecoder::TakeFrontBuffer(const Mailbox& mailbox) {
  NOTIMPLEMENTED();
}
void RasterDecoder::ReturnFrontBuffer(const Mailbox& mailbox, bool is_lost) {
  NOTIMPLEMENTED();
}
bool RasterDecoder::ResizeOffscreenFramebuffer(const gfx::Size& size) {
  NOTIMPLEMENTED();
  return true;
}

// Make this decoder's GL context current.
bool RasterDecoder::MakeCurrent() {
  DCHECK(surface_);
  if (!context_.get())
    return false;

  if (WasContextLost()) {
    LOG(ERROR) << "  GLES2DecoderImpl: Trying to make lost context current.";
    return false;
  }

  if (!context_->MakeCurrent(surface_.get())) {
    LOG(ERROR) << "  GLES2DecoderImpl: Context lost during MakeCurrent.";
    MarkContextLost(error::kMakeCurrentFailed);
    group_->LoseContexts(error::kUnknown);
    return false;
  }
  return true;
}

GLES2Util* RasterDecoder::GetGLES2Util() {
  NOTIMPLEMENTED();
  return nullptr;
}

gl::GLContext* RasterDecoder::GetGLContext() {
  return context_.get();
}

ContextGroup* RasterDecoder::GetContextGroup() {
  NOTIMPLEMENTED();
  return nullptr;
}

const FeatureInfo* RasterDecoder::GetFeatureInfo() const {
  NOTIMPLEMENTED();
  return nullptr;
}

Capabilities RasterDecoder::GetCapabilities() {
  gpu::Capabilities caps;
  caps.supports_oop_raster = true;
  return caps;
}

void RasterDecoder::RestoreState(const ContextState* prev_state) {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreActiveTexture() const {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreAllTextureUnitAndSamplerBindings(
    const ContextState* prev_state) const {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreActiveTextureUnitBinding(unsigned int target) const {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreBufferBinding(unsigned int target) {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreBufferBindings() const {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreFramebufferBindings() const {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreRenderbufferBindings() {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreGlobalState() const {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreProgramBindings() const {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreTextureState(unsigned service_id) const {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreTextureUnitBindings(unsigned unit) const {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreVertexAttribArray(unsigned index) {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreAllExternalTextureBindingsIfNeeded() {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreDeviceWindowRectangles() const {
  NOTIMPLEMENTED();
}

void RasterDecoder::ClearAllAttributes() const {
  NOTIMPLEMENTED();
}

void RasterDecoder::RestoreAllAttributes() const {
  NOTIMPLEMENTED();
}

void RasterDecoder::SetIgnoreCachedStateForTest(bool ignore) {
  NOTIMPLEMENTED();
}

void RasterDecoder::SetForceShaderNameHashingForTest(bool force) {
  NOTIMPLEMENTED();
}

uint32_t RasterDecoder::GetAndClearBackbufferClearBitsForTest() {
  NOTIMPLEMENTED();
  return 0;
}

size_t RasterDecoder::GetSavedBackTextureCountForTest() {
  NOTIMPLEMENTED();
  return 0;
}

size_t RasterDecoder::GetCreatedBackTextureCountForTest() {
  NOTIMPLEMENTED();
  return 0;
}

QueryManager* RasterDecoder::GetQueryManager() {
  return query_manager_.get();
}

GpuFenceManager* RasterDecoder::GetGpuFenceManager() {
  NOTIMPLEMENTED();
  return nullptr;
}

FramebufferManager* RasterDecoder::GetFramebufferManager() {
  NOTIMPLEMENTED();
  return nullptr;
}

TransformFeedbackManager* RasterDecoder::GetTransformFeedbackManager() {
  NOTIMPLEMENTED();
  return nullptr;
}

VertexArrayManager* RasterDecoder::GetVertexArrayManager() {
  NOTIMPLEMENTED();
  return nullptr;
}

ImageManager* RasterDecoder::GetImageManagerForTest() {
  NOTIMPLEMENTED();
  return nullptr;
}

bool RasterDecoder::HasPendingQueries() const {
  return query_manager_.get() && query_manager_->HavePendingQueries();
}

void RasterDecoder::ProcessPendingQueries(bool did_finish) {
  if (!query_manager_.get())
    return;
  query_manager_->ProcessPendingQueries(did_finish);
}

bool RasterDecoder::HasMoreIdleWork() const {
  NOTIMPLEMENTED();
  return false;
}

void RasterDecoder::PerformIdleWork() {
  NOTIMPLEMENTED();
}

bool RasterDecoder::HasPollingWork() const {
  NOTIMPLEMENTED();
  return false;
}

void RasterDecoder::PerformPollingWork() {
  NOTIMPLEMENTED();
}

bool RasterDecoder::GetServiceTextureId(uint32_t client_texture_id,
                                        uint32_t* service_texture_id) {
  NOTIMPLEMENTED();
  return false;
}

TextureBase* RasterDecoder::GetTextureBase(uint32_t client_id) {
  NOTIMPLEMENTED();
  return nullptr;
}

bool RasterDecoder::ClearLevel(Texture* texture,
                               unsigned target,
                               int level,
                               unsigned format,
                               unsigned type,
                               int xoffset,
                               int yoffset,
                               int width,
                               int height) {
  NOTIMPLEMENTED();
  return false;
}

bool RasterDecoder::ClearCompressedTextureLevel(Texture* texture,
                                                unsigned target,
                                                int level,
                                                unsigned format,
                                                int width,
                                                int height) {
  NOTIMPLEMENTED();
  return false;
}

bool RasterDecoder::IsCompressedTextureFormat(unsigned format) {
  NOTIMPLEMENTED();
  return false;
}

bool RasterDecoder::ClearLevel3D(Texture* texture,
                                 unsigned target,
                                 int level,
                                 unsigned format,
                                 unsigned type,
                                 int width,
                                 int height,
                                 int depth) {
  NOTIMPLEMENTED();
  return false;
}

ErrorState* RasterDecoder::GetErrorState() {
  NOTIMPLEMENTED();
  return nullptr;
}

void RasterDecoder::WaitForReadPixels(base::Closure callback) {
  NOTIMPLEMENTED();
}

bool RasterDecoder::WasContextLost() const {
  return false;
}

bool RasterDecoder::WasContextLostByRobustnessExtension() const {
  NOTIMPLEMENTED();
  return false;
}

void RasterDecoder::MarkContextLost(error::ContextLostReason reason) {
  NOTIMPLEMENTED();
}

bool RasterDecoder::CheckResetStatus() {
  NOTIMPLEMENTED();
  return false;
}

Logger* RasterDecoder::GetLogger() {
  NOTIMPLEMENTED();
  return nullptr;
}

void RasterDecoder::BeginDecoding() {
  NOTIMPLEMENTED();
  query_manager_->ProcessFrameBeginUpdates();
}

void RasterDecoder::EndDecoding() {
  NOTIMPLEMENTED();
}

error::Error RasterDecoder::DoCommands(unsigned int num_commands,
                                       const volatile void* buffer,
                                       int num_entries,
                                       int* entries_processed) {
  *entries_processed = num_entries;
  return error::kNoError;
}

const ContextState* RasterDecoder::GetContextState() {
  NOTIMPLEMENTED();
  return nullptr;
}

scoped_refptr<ShaderTranslatorInterface> RasterDecoder::GetTranslator(
    unsigned int type) {
  NOTIMPLEMENTED();
  return nullptr;
}

void RasterDecoder::BindImage(uint32_t client_texture_id,
                              uint32_t texture_target,
                              gl::GLImage* image,
                              bool can_bind_to_sampler) {
  NOTIMPLEMENTED();
}

}  // namespace gles2
}  // namespace gpu
