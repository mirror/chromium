// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/raster_decoder.h"

#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "cc/paint/paint_op_buffer.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/command_buffer/common/context_result.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "gpu/command_buffer/service/error_state.h"
#include "gpu/command_buffer/service/gl_stream_texture_image.h"
#include "gpu/command_buffer/service/gl_utils.h"
#include "gpu/command_buffer/service/gles2_cmd_copy_tex_image.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/command_buffer/service/query_manager.h"
#include "gpu/command_buffer/service/shader_translator.h"
#include "third_party/angle/src/image_util/loadimage.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkSurfaceProps.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_version_info.h"
#include "ui/gl/init/create_gr_gl_interface.h"

// Local versions of the SET_GL_ERROR macros
#define LOCAL_SET_GL_ERROR(error, function_name, msg) \
  ERRORSTATE_SET_GL_ERROR(state_.GetErrorState(), error, function_name, msg)
#define LOCAL_SET_GL_ERROR_INVALID_ENUM(function_name, value, label)          \
  ERRORSTATE_SET_GL_ERROR_INVALID_ENUM(state_.GetErrorState(), function_name, \
                                       value, label)
#define LOCAL_COPY_REAL_GL_ERRORS_TO_WRAPPER(function_name)         \
  ERRORSTATE_COPY_REAL_GL_ERRORS_TO_WRAPPER(state_.GetErrorState(), \
                                            function_name)
#define LOCAL_PEEK_GL_ERROR(function_name) \
  ERRORSTATE_PEEK_GL_ERROR(state_.GetErrorState(), function_name)
#define LOCAL_CLEAR_REAL_GL_ERRORS(function_name) \
  ERRORSTATE_CLEAR_REAL_GL_ERRORS(state_.GetErrorState(), function_name)
#define LOCAL_PERFORMANCE_WARNING(msg) \
  PerformanceWarning(__FILE__, __LINE__, msg)
#define LOCAL_RENDER_WARNING(msg) RenderWarning(__FILE__, __LINE__, msg)

namespace gpu {
namespace gles2 {

namespace {

const int kASTCBlockSize = 16;
const int kS3TCBlockWidth = 4;
const int kS3TCBlockHeight = 4;
const int kS3TCDXT1BlockSize = 8;
const int kS3TCDXT3AndDXT5BlockSize = 16;
const int kEACAndETC2BlockSize = 4;

typedef struct {
  int blockWidth;
  int blockHeight;
} ASTCBlockArray;

const ASTCBlockArray kASTCBlockArray[] = {
    {4, 4}, /* GL_COMPRESSED_RGBA_ASTC_4x4_KHR */
    {5, 4}, /* and GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR */
    {5, 5},  {6, 5},  {6, 6},  {8, 5},   {8, 6},   {8, 8},
    {10, 5}, {10, 6}, {10, 8}, {10, 10}, {12, 10}, {12, 12}};

bool CheckETCFormatSupport(const FeatureInfo& feature_info) {
  const gl::GLVersionInfo& version_info = feature_info.gl_version_info();
  return version_info.IsAtLeastGL(4, 3) || version_info.IsAtLeastGLES(3, 0) ||
         feature_info.feature_flags().arb_es3_compatibility;
}

using CompressedFormatSupportCheck = bool (*)(const FeatureInfo&);
using CompressedFormatDecompressionFunction = void (*)(size_t width,
                                                       size_t height,
                                                       size_t depth,
                                                       const uint8_t* input,
                                                       size_t inputRowPitch,
                                                       size_t inputDepthPitch,
                                                       uint8_t* output,
                                                       size_t outputRowPitch,
                                                       size_t outputDepthPitch);
struct CompressedFormatInfo {
  GLenum format;
  uint32_t block_size;
  uint32_t bytes_per_block;
  CompressedFormatSupportCheck support_check;
  CompressedFormatDecompressionFunction decompression_function;
  GLenum decompressed_internal_format;
  GLenum decompressed_format;
  GLenum decompressed_type;
};

const CompressedFormatInfo kCompressedFormatInfoArray[] = {
    {
        GL_COMPRESSED_R11_EAC, 4, 8, CheckETCFormatSupport,
        angle::LoadEACR11ToR8, GL_R8, GL_RED, GL_UNSIGNED_BYTE,
    },
    {
        GL_COMPRESSED_SIGNED_R11_EAC, 4, 8, CheckETCFormatSupport,
        angle::LoadEACR11SToR8, GL_R8_SNORM, GL_RED, GL_BYTE,
    },
    {
        GL_COMPRESSED_RG11_EAC, 4, 16, CheckETCFormatSupport,
        angle::LoadEACRG11ToRG8, GL_RG8, GL_RG, GL_UNSIGNED_BYTE,
    },
    {
        GL_COMPRESSED_SIGNED_RG11_EAC, 4, 16, CheckETCFormatSupport,
        angle::LoadEACRG11SToRG8, GL_RG8_SNORM, GL_RG, GL_BYTE,
    },
    {
        GL_COMPRESSED_RGB8_ETC2, 4, 8, CheckETCFormatSupport,
        angle::LoadETC2RGB8ToRGBA8, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
    },
    {
        GL_COMPRESSED_SRGB8_ETC2, 4, 8, CheckETCFormatSupport,
        angle::LoadETC2SRGB8ToRGBA8, GL_SRGB8_ALPHA8, GL_SRGB_ALPHA,
        GL_UNSIGNED_BYTE,
    },
    {
        GL_COMPRESSED_RGBA8_ETC2_EAC, 4, 16, CheckETCFormatSupport,
        angle::LoadETC2RGBA8ToRGBA8, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
    },
    {
        GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, 4, 8,
        CheckETCFormatSupport, angle::LoadETC2RGB8A1ToRGBA8, GL_RGBA8, GL_RGBA,
        GL_UNSIGNED_BYTE,
    },
    {
        GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, 4, 16, CheckETCFormatSupport,
        angle::LoadETC2SRGBA8ToSRGBA8, GL_SRGB8_ALPHA8, GL_SRGB_ALPHA,
        GL_UNSIGNED_BYTE,
    },
    {
        GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, 4, 8,
        CheckETCFormatSupport, angle::LoadETC2SRGB8A1ToRGBA8, GL_SRGB8_ALPHA8,
        GL_SRGB_ALPHA, GL_UNSIGNED_BYTE,
    },
};

const CompressedFormatInfo* GetCompressedFormatInfo(GLenum format) {
  for (size_t i = 0; i < arraysize(kCompressedFormatInfoArray); i++) {
    if (kCompressedFormatInfoArray[i].format == format) {
      return &kCompressedFormatInfoArray[i];
    }
  }
  return nullptr;
}

// This class prevents any GL errors that occur when it is in scope from
// being reported to the client.
class ScopedGLErrorSuppressor {
 public:
  explicit ScopedGLErrorSuppressor(const char* function_name,
                                   ErrorState* error_state);
  ~ScopedGLErrorSuppressor();

 private:
  const char* function_name_;
  ErrorState* error_state_;
  DISALLOW_COPY_AND_ASSIGN(ScopedGLErrorSuppressor);
};

ScopedGLErrorSuppressor::ScopedGLErrorSuppressor(const char* function_name,
                                                 ErrorState* error_state)
    : function_name_(function_name), error_state_(error_state) {
  ERRORSTATE_COPY_REAL_GL_ERRORS_TO_WRAPPER(error_state_, function_name_);
}

ScopedGLErrorSuppressor::~ScopedGLErrorSuppressor() {
  ERRORSTATE_CLEAR_REAL_GL_ERRORS(error_state_, function_name_);
}

void RestoreCurrentTextureBindings(ContextState* state,
                                   GLenum target,
                                   GLuint texture_unit) {
  DCHECK(!state->texture_units.empty());
  DCHECK_LT(texture_unit, state->texture_units.size());
  TextureUnit& info = state->texture_units[texture_unit];
  GLuint last_id;
  TextureRef* texture_ref = info.GetInfoForTarget(target);
  if (texture_ref) {
    last_id = texture_ref->service_id();
  } else {
    last_id = 0;
  }

  state->api()->glBindTextureFn(target, last_id);
}

class ScopedTextureBinder {
 public:
  explicit ScopedTextureBinder(ContextState* state, GLuint id, GLenum target);
  ~ScopedTextureBinder();

 private:
  ContextState* state_;
  GLenum target_;
  DISALLOW_COPY_AND_ASSIGN(ScopedTextureBinder);
};

ScopedTextureBinder::ScopedTextureBinder(ContextState* state,
                                         GLuint id,
                                         GLenum target)
    : state_(state), target_(target) {
  ScopedGLErrorSuppressor suppressor("ScopedTextureBinder::ctor",
                                     state_->GetErrorState());

  // TODO(apatrick): Check if there are any other states that need to be reset
  // before binding a new texture.
  auto* api = state->api();
  api->glActiveTextureFn(GL_TEXTURE0);
  api->glBindTextureFn(target, id);
}

ScopedTextureBinder::~ScopedTextureBinder() {
  ScopedGLErrorSuppressor suppressor("ScopedTextureBinder::dtor",
                                     state_->GetErrorState());
  RestoreCurrentTextureBindings(state_, target_, 0);
  state_->RestoreActiveTexture();
}

}  // namespace

RasterDecoder::CommandInfo RasterDecoder::command_info[] = {
#define GLES2_CMD_OP(name)                                   \
  {                                                          \
      nullptr, cmds::name::kArgFlags, cmds::name::cmd_flags, \
      sizeof(cmds::name) / sizeof(CommandBufferEntry) - 1,   \
  }, /* NOLINT */
    GLES2_COMMAND_LIST(GLES2_CMD_OP)
#undef GLES2_CMD_OP
};

RasterDecoder::RasterDecoder(GLES2DecoderClient* client,
                             CommandBufferServiceBase* command_buffer_service,
                             Outputter* outputter,
                             ContextGroup* group)
    : GLES2Decoder(command_buffer_service, outputter),
      client_(client),
      group_(group),
      validators_(group_->feature_info()->validators()),
      feature_info_(group_->feature_info()),
      commands_to_process_(0),
      current_decoder_error_(error::kNoError),
      logger_(&debug_marker_manager_, client),
      state_(group_->feature_info(), this, &logger_),
      weak_ptr_factory_(this) {
  command_info[265 - 256].cmd_handler = &RasterDecoder::HandleBindTexture;
  command_info[308 - 256].cmd_handler =
      &RasterDecoder::HandleDeleteTexturesImmediate;
  command_info[322 - 256].cmd_handler = &RasterDecoder::HandleFlush;
  command_info[332 - 256].cmd_handler =
      &RasterDecoder::HandleGenTexturesImmediate;
  command_info[362 - 256].cmd_handler = &RasterDecoder::HandleGetString;
  command_info[419 - 256].cmd_handler = &RasterDecoder::HandleTexParameteri;
  command_info[481 - 256].cmd_handler = &RasterDecoder::HandleTexStorage2DEXT;
  command_info[482 - 256].cmd_handler =
      &RasterDecoder::HandleGenQueriesEXTImmediate;
  command_info[483 - 256].cmd_handler =
      &RasterDecoder::HandleDeleteQueriesEXTImmediate;
  command_info[514 - 256].cmd_handler =
      &RasterDecoder::HandleCopySubTextureCHROMIUM;
  command_info[520 - 256].cmd_handler =
      &RasterDecoder::HandleCreateAndConsumeTextureINTERNAL;
  command_info[522 - 256].cmd_handler =
      &RasterDecoder::HandleBindTexImage2DCHROMIUM;
  command_info[524 - 256].cmd_handler =
      &RasterDecoder::HandleReleaseTexImage2DCHROMIUM;
  command_info[525 - 256].cmd_handler =
      &RasterDecoder::HandleTraceBeginCHROMIUM;
  command_info[526 - 256].cmd_handler = &RasterDecoder::HandleTraceEndCHROMIUM;
  command_info[529 - 256].cmd_handler =
      &RasterDecoder::HandleInsertFenceSyncCHROMIUM;
  command_info[530 - 256].cmd_handler =
      &RasterDecoder::HandleWaitSyncTokenCHROMIUM;
  command_info[539 - 256].cmd_handler =
      &RasterDecoder::HandleFlushDriverCachesCHROMIUM;
  command_info[580 - 256].cmd_handler =
      &RasterDecoder::HandleBeginRasterCHROMIUM;
  command_info[581 - 256].cmd_handler = &RasterDecoder::HandleRasterCHROMIUM;
  command_info[582 - 256].cmd_handler = &RasterDecoder::HandleEndRasterCHROMIUM;
}

RasterDecoder::~RasterDecoder() {}

void RasterDecoder::OnContextLostError() {
  NOTIMPLEMENTED();
}
void RasterDecoder::OnOutOfMemoryError() {
  NOTIMPLEMENTED();
}

base::WeakPtr<GLES2Decoder> RasterDecoder::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

// Initializes the graphics context. Can create an offscreen
// decoder with a frame buffer that can be referenced from the parent.
// Takes ownership of GLContext.
// Parameters:
//  surface: the GL surface to render to.
//  context: the GL context to render to.
//  offscreen: whether to make the context offscreen or not. When FBO 0 is
//      bound, offscreen contexts render to an internal buffer, onscreen ones
//      to the surface.
//  offscreen_size: the size if the GL context is offscreen.
// Returns:
//   true if successful.
gpu::ContextResult RasterDecoder::Initialize(
    const scoped_refptr<gl::GLSurface>& surface,
    const scoped_refptr<gl::GLContext>& context,
    bool offscreen,
    const DisallowedFeatures& disallowed_features,
    const ContextCreationAttribHelper& attrib_helper) {
  TRACE_EVENT0("gpu", "RasterDecoder::Initialize");
  DCHECK(context->IsCurrent(surface.get()));
  DCHECK(!context_.get());

  state_.set_api(gl::g_current_gl_context);

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

  // Setup texture units.
  state_.texture_units.resize(group_->max_texture_units());
  state_.sampler_units.resize(group_->max_texture_units());
  for (uint32_t tt = 0; tt < state_.texture_units.size(); ++tt) {
    state_.api()->glActiveTextureFn(GL_TEXTURE0 + tt);
    // We want the last bind to be 2D.
    TextureRef* ref;
    if (feature_info_->feature_flags().oes_egl_image_external ||
        feature_info_->feature_flags().nv_egl_stream_consumer_external) {
      ref = group_->texture_manager()->GetDefaultTextureInfo(
          GL_TEXTURE_EXTERNAL_OES);
      state_.texture_units[tt].bound_texture_external_oes = ref;
      state_.api()->glBindTextureFn(GL_TEXTURE_EXTERNAL_OES,
                                    ref ? ref->service_id() : 0);
    }
    if (feature_info_->feature_flags().arb_texture_rectangle) {
      ref = group_->texture_manager()->GetDefaultTextureInfo(
          GL_TEXTURE_RECTANGLE_ARB);
      state_.texture_units[tt].bound_texture_rectangle_arb = ref;
      state_.api()->glBindTextureFn(GL_TEXTURE_RECTANGLE_ARB,
                                    ref ? ref->service_id() : 0);
    }
    ref = group_->texture_manager()->GetDefaultTextureInfo(GL_TEXTURE_CUBE_MAP);
    state_.texture_units[tt].bound_texture_cube_map = ref;
    state_.api()->glBindTextureFn(GL_TEXTURE_CUBE_MAP,
                                  ref ? ref->service_id() : 0);
    ref = group_->texture_manager()->GetDefaultTextureInfo(GL_TEXTURE_2D);
    state_.texture_units[tt].bound_texture_2d = ref;
    state_.api()->glBindTextureFn(GL_TEXTURE_2D, ref ? ref->service_id() : 0);
  }
  state_.api()->glActiveTextureFn(GL_TEXTURE0);
  CHECK_GL_ERROR();

  sk_sp<const GrGLInterface> interface(
      gl::init::CreateGrGLInterface(gl_version_info()));
  supports_oop_raster_ = true;
  if (!interface) {
    DLOG(ERROR) << "OOP raster support disabled: GrGLInterface creation "
                   "failed.";
    supports_oop_raster_ = false;
  }

  if (supports_oop_raster_) {
    gr_context_ = sk_sp<GrContext>(
        GrContext::Create(kOpenGL_GrBackend,
                          reinterpret_cast<GrBackendContext>(interface.get())));
    if (gr_context_) {
      // TODO(enne): this cache is for this decoder only and each decoder has
      // its own cache.  This is pretty unfortunate.  This really needs to be
      // rethought before shipping.  Most likely a different command buffer
      // context for raster-in-gpu, with a shared gl context / gr context
      // that different decoders can use.
      static const int kMaxGaneshResourceCacheCount = 8196;
      static const size_t kMaxGaneshResourceCacheBytes = 96 * 1024 * 1024;
      gr_context_->setResourceCacheLimits(kMaxGaneshResourceCacheCount,
                                          kMaxGaneshResourceCacheBytes);
    } else {
      bool was_lost = CheckResetStatus();
      if (was_lost) {
        DLOG(ERROR)
            << "ContextResult::kTransientFailure: Could not create GrContext";
        return gpu::ContextResult::kTransientFailure;
      }
      DLOG(ERROR) << "OOP raster support disabled: GrContext creation "
                     "failed.";
      supports_oop_raster_ = false;
    }
  }

  return gpu::ContextResult::kSuccess;
}

// Destroys the graphics context.
void RasterDecoder::Destroy(bool have_context) {
  NOTIMPLEMENTED();
  if (query_manager_.get()) {
    query_manager_->Destroy(have_context);
    query_manager_.reset();
  }
}

// Set the surface associated with the default FBO.
void RasterDecoder::SetSurface(const scoped_refptr<gl::GLSurface>& surface) {
  NOTIMPLEMENTED();
}
// Releases the surface associated with the GL context.
// The decoder should not be used until a new surface is set.
void RasterDecoder::ReleaseSurface() {
  NOTIMPLEMENTED();
}

void RasterDecoder::TakeFrontBuffer(const Mailbox& mailbox) {
  NOTIMPLEMENTED();
}
void RasterDecoder::ReturnFrontBuffer(const Mailbox& mailbox, bool is_lost) {
  NOTIMPLEMENTED();
}

// Resize an offscreen frame buffer.
bool RasterDecoder::ResizeOffscreenFramebuffer(const gfx::Size& size) {
  NOTIMPLEMENTED();
  return true;
}

// Make this decoder's GL context current.
// FIXME(backer): Fraction of logic...
bool RasterDecoder::MakeCurrent() {
  DCHECK(surface_);
  if (!context_.get())
    return false;

  if (WasContextLost()) {
    LOG(ERROR) << "  RasterDecoder: Trying to make lost context current.";
    return false;
  }

  if (!context_->MakeCurrent(surface_.get())) {
    LOG(ERROR) << "  RasterDecoder: Context lost during MakeCurrent.";
    MarkContextLost(error::kMakeCurrentFailed);
    group_->LoseContexts(error::kUnknown);
    return false;
  }
  return true;
}

// Gets the GLES2 Util which holds info.
GLES2Util* RasterDecoder::GetGLES2Util() {
  NOTIMPLEMENTED();
  return nullptr;
}

// Gets the associated GLContext.
gl::GLContext* RasterDecoder::GetGLContext() {
  return context_.get();
}

// Gets the associated ContextGroup
ContextGroup* RasterDecoder::GetContextGroup() {
  NOTIMPLEMENTED();
  return nullptr;
}
const FeatureInfo* RasterDecoder::GetFeatureInfo() const {
  NOTIMPLEMENTED();
  return nullptr;
}

// TODO(backer): So much missing here...
Capabilities RasterDecoder::GetCapabilities() {
  DCHECK(initialized());
  Capabilities caps;
  const gl::GLVersionInfo& version_info = gl_version_info();
  caps.VisitPrecisions([&version_info](
                           GLenum shader, GLenum type,
                           Capabilities::ShaderPrecision* shader_precision) {
    GLint range[2] = {0, 0};
    GLint precision = 0;
    QueryShaderPrecisionFormat(version_info, shader, type, range, &precision);
    shader_precision->min_range = range[0];
    shader_precision->max_range = range[1];
    shader_precision->precision = precision;
  });
  // DoGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
  //               &caps.max_combined_texture_image_units, 1);
  // DoGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE,
  // &caps.max_cube_map_texture_size,
  //               1);
  // DoGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS,
  //               &caps.max_fragment_uniform_vectors, 1);
  // DoGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &caps.max_renderbuffer_size, 1);
  // DoGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &caps.max_texture_image_units,
  // 1); DoGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.max_texture_size, 1);
  // DoGetIntegerv(GL_MAX_VARYING_VECTORS, &caps.max_varying_vectors, 1);
  // DoGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &caps.max_vertex_attribs, 1);
  // DoGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
  //               &caps.max_vertex_texture_image_units, 1);
  // DoGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS,
  // &caps.max_vertex_uniform_vectors,
  //               1);
  // {
  //   GLint dims[2] = {0, 0};
  //   DoGetIntegerv(GL_MAX_VIEWPORT_DIMS, dims, 2);
  //   caps.max_viewport_width = dims[0];
  //   caps.max_viewport_height = dims[1];
  // }
  // DoGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS,
  //               &caps.num_compressed_texture_formats, 1);
  // DoGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS,
  // &caps.num_shader_binary_formats,
  //               1);
  // DoGetIntegerv(GL_BIND_GENERATES_RESOURCE_CHROMIUM,
  //               &caps.bind_generates_resource_chromium, 1);
  // if (feature_info_->IsWebGL2OrES3Context()) {
  //   // TODO(zmo): Note that some parameter values could be more than 32-bit,
  //   // but for now we clamp them to 32-bit max.
  //   DoGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &caps.max_3d_texture_size, 1);
  //   DoGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS,
  //   &caps.max_array_texture_layers,
  //                 1);
  //   DoGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &caps.max_color_attachments, 1);
  //   DoGetInteger64v(GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS,
  //                   &caps.max_combined_fragment_uniform_components, 1);
  //   DoGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS,
  //                 &caps.max_combined_uniform_blocks, 1);
  //   DoGetInteger64v(GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS,
  //                   &caps.max_combined_vertex_uniform_components, 1);
  //   DoGetIntegerv(GL_MAX_DRAW_BUFFERS, &caps.max_draw_buffers, 1);
  //   DoGetInteger64v(GL_MAX_ELEMENT_INDEX, &caps.max_element_index, 1);
  //   DoGetIntegerv(GL_MAX_ELEMENTS_INDICES, &caps.max_elements_indices, 1);
  //   DoGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &caps.max_elements_vertices, 1);
  //   DoGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS,
  //                 &caps.max_fragment_input_components, 1);
  //   DoGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS,
  //                 &caps.max_fragment_uniform_blocks, 1);
  //   DoGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,
  //                 &caps.max_fragment_uniform_components, 1);
  //   DoGetIntegerv(GL_MAX_PROGRAM_TEXEL_OFFSET,
  //   &caps.max_program_texel_offset,
  //                 1);
  //   DoGetInteger64v(GL_MAX_SERVER_WAIT_TIMEOUT,
  //   &caps.max_server_wait_timeout,
  //                   1);
  //   // Work around Linux NVIDIA driver bug where GL_TIMEOUT_IGNORED is
  //   // returned.
  //   if (caps.max_server_wait_timeout < 0)
  //     caps.max_server_wait_timeout = 0;
  //   DoGetFloatv(GL_MAX_TEXTURE_LOD_BIAS, &caps.max_texture_lod_bias, 1);
  //   DoGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS,
  //                 &caps.max_transform_feedback_interleaved_components, 1);
  //   DoGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS,
  //                 &caps.max_transform_feedback_separate_attribs, 1);
  //   DoGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS,
  //                 &caps.max_transform_feedback_separate_components, 1);
  //   DoGetInteger64v(GL_MAX_UNIFORM_BLOCK_SIZE, &caps.max_uniform_block_size,
  //   1); DoGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS,
  //                 &caps.max_uniform_buffer_bindings, 1);
  //   DoGetIntegerv(GL_MAX_VARYING_COMPONENTS, &caps.max_varying_components,
  //   1); DoGetIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS,
  //                 &caps.max_vertex_output_components, 1);
  //   DoGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS,
  //   &caps.max_vertex_uniform_blocks,
  //                 1);
  //   DoGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS,
  //                 &caps.max_vertex_uniform_components, 1);
  //   DoGetIntegerv(GL_MIN_PROGRAM_TEXEL_OFFSET,
  //   &caps.min_program_texel_offset,
  //                 1);
  //   DoGetIntegerv(GL_NUM_EXTENSIONS, &caps.num_extensions, 1);
  //   // TODO(vmiura): Remove GL_NUM_PROGRAM_BINARY_FORMATS.
  //   DoGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS,
  //                 &caps.num_program_binary_formats, 1);
  //   DoGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT,
  //                 &caps.uniform_buffer_offset_alignment, 1);
  //   // TODO(zmo): once we switch to MANGLE, we should query version numbers.
  //   caps.major_version = 3;
  //   caps.minor_version = 0;
  // }
  // if (feature_info_->feature_flags().multisampled_render_to_texture ||
  //     feature_info_->feature_flags().chromium_framebuffer_multisample ||
  //     feature_info_->IsWebGL2OrES3Context()) {
  //   DoGetIntegerv(GL_MAX_SAMPLES, &caps.max_samples, 1);
  // }

  // caps.num_stencil_bits = num_stencil_bits_;

  caps.egl_image_external =
      feature_info_->feature_flags().oes_egl_image_external;
  caps.texture_format_astc =
      feature_info_->feature_flags().ext_texture_format_astc;
  caps.texture_format_atc =
      feature_info_->feature_flags().ext_texture_format_atc;
  caps.texture_format_bgra8888 =
      feature_info_->feature_flags().ext_texture_format_bgra8888;
  caps.texture_format_dxt1 =
      feature_info_->feature_flags().ext_texture_format_dxt1;
  caps.texture_format_dxt5 =
      feature_info_->feature_flags().ext_texture_format_dxt5;
  caps.texture_format_etc1 =
      feature_info_->feature_flags().oes_compressed_etc1_rgb8_texture;
  caps.texture_format_etc1_npot =
      caps.texture_format_etc1 && !workarounds().etc1_power_of_two_only;
  // Whether or not a texture will be bound to an EGLImage is
  // dependent on whether we are using the sync mailbox manager.
  caps.disable_one_component_textures =
      mailbox_manager()->UsesSync() &&
      workarounds().avoid_one_component_egl_images;
  caps.texture_rectangle = feature_info_->feature_flags().arb_texture_rectangle;
  caps.texture_usage = feature_info_->feature_flags().angle_texture_usage;
  caps.texture_storage = feature_info_->feature_flags().ext_texture_storage;
  caps.discard_framebuffer =
      feature_info_->feature_flags().ext_discard_framebuffer;
  caps.sync_query = feature_info_->feature_flags().chromium_sync_query;

// caps.chromium_image_rgb_emulation = ChromiumImageNeedsRGBEmulation();
#if defined(OS_MACOSX)
  // This is unconditionally true on mac, no need to test for it at runtime.
  caps.iosurface = true;
#endif

  // caps.post_sub_buffer = supports_post_sub_buffer_;
  // caps.swap_buffers_with_bounds = supports_swap_buffers_with_bounds_;
  // caps.commit_overlay_planes = supports_commit_overlay_planes_;
  // caps.surfaceless = surfaceless_;
  // bool is_offscreen = !!offscreen_target_frame_buffer_.get();
  // caps.flips_vertically = !is_offscreen && surface_->FlipsVertically();
  // caps.msaa_is_slow = workarounds().msaa_is_slow;
  // caps.avoid_stencil_buffers = workarounds().avoid_stencil_buffers;
  // caps.multisample_compatibility =
  //     feature_info_->feature_flags().ext_multisample_compatibility;
  // caps.dc_layers = supports_dc_layers_;
  // caps.use_dc_overlays_for_video = surface_->UseOverlaysForVideo();

  // caps.blend_equation_advanced =
  //     feature_info_->feature_flags().blend_equation_advanced;
  // caps.blend_equation_advanced_coherent =
  //     feature_info_->feature_flags().blend_equation_advanced_coherent;
  // caps.texture_rg = feature_info_->feature_flags().ext_texture_rg;
  // caps.texture_norm16 = feature_info_->feature_flags().ext_texture_norm16;
  // caps.texture_half_float_linear =
  //     feature_info_->oes_texture_half_float_linear_available();
  // caps.color_buffer_half_float_rgba =
  //     feature_info_->ext_color_buffer_float_available() ||
  //     feature_info_->ext_color_buffer_half_float_available();
  // caps.image_ycbcr_422 =
  //     feature_info_->feature_flags().chromium_image_ycbcr_422;
  // caps.image_ycbcr_420v =
  //     feature_info_->feature_flags().chromium_image_ycbcr_420v;
  // caps.max_copy_texture_chromium_size =
  //     workarounds().max_copy_texture_chromium_size;
  // caps.render_buffer_format_bgra8888 =
  //     feature_info_->feature_flags().ext_render_buffer_format_bgra8888;
  // caps.occlusion_query = feature_info_->feature_flags().occlusion_query;
  // caps.occlusion_query_boolean =
  //     feature_info_->feature_flags().occlusion_query_boolean;
  // caps.timer_queries = query_manager_->GPUTimingAvailable();
  caps.gpu_rasterization =
      group_->gpu_feature_info()
          .status_values[GPU_FEATURE_TYPE_GPU_RASTERIZATION] ==
      kGpuFeatureStatusEnabled;
  // if (workarounds().disable_non_empty_post_sub_buffers_for_onscreen_surfaces
  // &&
  //     !surface_->IsOffscreen()) {
  //   caps.disable_non_empty_post_sub_buffers = true;
  // }
  if (workarounds().broken_egl_image_ref_counting &&
      group_->gpu_preferences().enable_threaded_texture_mailboxes) {
    caps.disable_2d_canvas_copy_on_write = true;
  }
  caps.texture_npot = feature_info_->feature_flags().npot_ok;
  caps.texture_storage_image =
      feature_info_->feature_flags().chromium_texture_storage_image;
  caps.supports_oop_raster = supports_oop_raster_;

  return caps;
}

// Restore States.
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

// Gets the QueryManager for this context.
QueryManager* RasterDecoder::GetQueryManager() {
  return query_manager_.get();
}

GpuFenceManager* RasterDecoder::GetGpuFenceManager() {
  NOTIMPLEMENTED();
  return nullptr;
}

// Gets the FramebufferManager for this context.
FramebufferManager* RasterDecoder::GetFramebufferManager() {
  NOTIMPLEMENTED();
  return nullptr;
}

// Gets the TransformFeedbackManager for this context.
TransformFeedbackManager* RasterDecoder::GetTransformFeedbackManager() {
  NOTIMPLEMENTED();
  return nullptr;
}

// Gets the VertexArrayManager for this context.
VertexArrayManager* RasterDecoder::GetVertexArrayManager() {
  NOTIMPLEMENTED();
  return nullptr;
}

// Gets the ImageManager for this context.
ImageManager* RasterDecoder::GetImageManagerForTest() {
  NOTIMPLEMENTED();
  return nullptr;
}

bool RasterDecoder::GenQueriesEXTHelper(GLsizei n, const GLuint* client_ids) {
  for (GLsizei ii = 0; ii < n; ++ii) {
    if (query_manager_->IsValidQuery(client_ids[ii])) {
      return false;
    }
  }
  query_manager_->GenQueries(n, client_ids);
  return true;
}

void RasterDecoder::DeleteQueriesEXTHelper(GLsizei n,
                                           const volatile GLuint* client_ids) {
  for (GLsizei ii = 0; ii < n; ++ii) {
    GLuint client_id = client_ids[ii];
    query_manager_->RemoveQuery(client_id);
  }
}

// Returns false if there are no pending queries.
bool RasterDecoder::HasPendingQueries() const {
  return query_manager_.get() && query_manager_->HavePendingQueries();
}

// Process any pending queries.
void RasterDecoder::ProcessPendingQueries(bool did_finish) {
  if (!query_manager_.get())
    return;
  query_manager_->ProcessPendingQueries(did_finish);
}

// Returns false if there is no idle work to be made.
bool RasterDecoder::HasMoreIdleWork() const {
  return false;
}

// Perform any idle work that needs to be made.
void RasterDecoder::PerformIdleWork() {
  NOTIMPLEMENTED();
}

// Whether there is state that needs to be regularly polled.
bool RasterDecoder::HasPollingWork() const {
  return false;
}

// Perform necessary polling.
void RasterDecoder::PerformPollingWork() {
  NOTIMPLEMENTED();
}

// Get the service texture ID corresponding to a client texture ID.
// If no such record is found then return false.
// FIXME
bool RasterDecoder::GetServiceTextureId(uint32_t client_texture_id,
                                        uint32_t* service_texture_id) {
  NOTIMPLEMENTED();
  return false;
}

// Gets the texture object associated with the client ID.  null is returned on
// failure or if the texture has not been bound yet.
TextureBase* RasterDecoder::GetTextureBase(uint32_t client_id) {
  NOTIMPLEMENTED();
  return nullptr;
}

// Clears a level sub area of a 2D texture.
// Returns false if a GL error should be generated.
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

// Clears a level sub area of a compressed 2D texture.
// Returns false if a GL error should be generated.
bool RasterDecoder::ClearCompressedTextureLevel(Texture* texture,
                                                unsigned target,
                                                int level,
                                                unsigned format,
                                                int width,
                                                int height) {
  NOTIMPLEMENTED();
  return false;
}

// Clears a level of a 3D texture.
// Returns false if a GL error should be generated.
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
  return state_.GetErrorState();
}

void RasterDecoder::WaitForReadPixels(base::Closure callback) {
  NOTIMPLEMENTED();
}

// Returns true if the context was lost either by GL_ARB_robustness, forced
// context loss or command buffer parse error.
bool RasterDecoder::WasContextLost() const {
  return false;
}

// Returns true if the context was lost specifically by GL_ARB_robustness.
bool RasterDecoder::WasContextLostByRobustnessExtension() const {
  NOTIMPLEMENTED();
  return false;
}

// Lose this context.
void RasterDecoder::MarkContextLost(error::ContextLostReason reason) {
  NOTIMPLEMENTED();
}

// Updates context lost state and returns true if lost. Most callers can use
// WasContextLost() as the GLES2Decoder will update the state internally. But
// if making GL calls directly, to the context then this state would not be
// updated and the caller can use this to determine if their calls failed due
// to context loss.
bool RasterDecoder::CheckResetStatus() {
  NOTIMPLEMENTED();
  return false;
}

Logger* RasterDecoder::GetLogger() {
  NOTIMPLEMENTED();
  return nullptr;
}

// FIXME(backer): No tracing or debugging...
void RasterDecoder::BeginDecoding() {
  query_manager_->ProcessFrameBeginUpdates();
}
void RasterDecoder::EndDecoding() {}

const char* RasterDecoder::GetCommandName(unsigned int command_id) const {
  if (command_id >= kFirstGLES2Command && command_id < kNumCommands) {
    return gles2::GetCommandName(static_cast<CommandId>(command_id));
  }
  return GetCommonCommandName(static_cast<cmd::CommandId>(command_id));
}

error::Error RasterDecoder::DoCommands(unsigned int num_commands,
                                       const volatile void* buffer,
                                       int num_entries,
                                       int* entries_processed) {
  DCHECK(entries_processed);
  commands_to_process_ = num_commands;
  error::Error result = error::kNoError;
  const volatile CommandBufferEntry* cmd_data =
      static_cast<const volatile CommandBufferEntry*>(buffer);
  int process_pos = 0;
  unsigned int command = 0;

  while (process_pos < num_entries && result == error::kNoError &&
         commands_to_process_--) {
    const unsigned int size = cmd_data->value_header.size;
    command = cmd_data->value_header.command;

    if (size == 0) {
      result = error::kInvalidSize;
      break;
    }

    if (static_cast<int>(size) + process_pos > num_entries) {
      result = error::kOutOfBounds;
      break;
    }

    const unsigned int arg_count = size - 1;
    unsigned int command_index = command - kFirstGLES2Command;
    if (command_index < arraysize(command_info)) {
      const CommandInfo& info = command_info[command_index];
      unsigned int info_arg_count = static_cast<unsigned int>(info.arg_count);
      if ((info.arg_flags == cmd::kFixed && arg_count == info_arg_count) ||
          (info.arg_flags == cmd::kAtLeastN && arg_count >= info_arg_count)) {
        uint32_t immediate_data_size = (arg_count - info_arg_count) *
                                       sizeof(CommandBufferEntry);  // NOLINT
        if (info.cmd_handler == nullptr) {
          LOG(ERROR) << "[" << logger_.GetLogPrefix() << "] "
                     << GetCommandName(command) << "(" << command << ", "
                     << command_index << ") is NOTIMPLEMENTED";
        } else {
          // LOG(ERROR) << "[" << logger_.GetLogPrefix() << "] "
          //            << GetCommandName(command) << "(" << command << ", "
          //            << command_index << ") ";
          result = (this->*info.cmd_handler)(immediate_data_size, cmd_data);
        }
      } else {
        result = error::kInvalidArguments;
      }
    } else {
      result = DoCommonCommand(command, arg_count, cmd_data);
    }

    if (result == error::kNoError &&
        current_decoder_error_ != error::kNoError) {
      result = current_decoder_error_;
      current_decoder_error_ = error::kNoError;
    }

    if (result != error::kDeferCommandUntilLater) {
      process_pos += size;
      cmd_data += size;
    }
  }

  *entries_processed = process_pos;

  if (error::IsError(result)) {
    LOG(ERROR) << "Error: " << result << " for Command "
               << GetCommandName(command);
  }

  return result;
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

error::Error RasterDecoder::HandleBindTexture(uint32_t immediate_data_size,
                                              const volatile void* cmd_data) {
  const volatile gles2::cmds::BindTexture& c =
      *static_cast<const volatile gles2::cmds::BindTexture*>(cmd_data);
  GLenum target = static_cast<GLenum>(c.target);
  GLuint client_id = c.texture;
  if (!validators_->texture_bind_target.IsValid(target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glBindTexture", target, "target");
    return error::kNoError;
  }

  TextureRef* texture_ref = NULL;
  GLuint service_id = 0;
  if (client_id != 0) {
    texture_ref = group_->texture_manager()->GetTexture(client_id);
    if (!texture_ref) {
      if (!group_->bind_generates_resource()) {
        LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBindTexture",
                           "id not generated by glGenTextures");
        return error::kNoError;
      }

      // It's a new id so make a texture texture for it.
      state_.api()->glGenTexturesFn(1, &service_id);
      DCHECK_NE(0u, service_id);
      group_->texture_manager()->CreateTexture(client_id, service_id);
      texture_ref = group_->texture_manager()->GetTexture(client_id);
    }
  } else {
    texture_ref = group_->texture_manager()->GetDefaultTextureInfo(target);
  }

  // Check the texture exists
  if (texture_ref) {
    Texture* texture = texture_ref->texture();
    // Check that we are not trying to bind it to a different target.
    if (texture->target() != 0 && texture->target() != target) {
      LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBindTexture",
                         "texture bound to more than 1 target.");
      return error::kNoError;
    }
    // FIXME(backer)
    // LogClientServiceForInfo(texture, client_id, "glBindTexture");

    state_.api()->glBindTextureFn(target, texture->service_id());
    if (texture->target() == 0) {
      group_->texture_manager()->SetTarget(texture_ref, target);
      if (!feature_info_->gl_version_info().BehavesLikeGLES() &&
          feature_info_->gl_version_info().IsAtLeastGL(3, 2)) {
        // In Desktop GL core profile and GL ES, depth textures are always
        // sampled to the RED channel, whereas on Desktop GL compatibility
        // proifle, they are sampled to RED, LUMINANCE, INTENSITY, or ALPHA
        // channel, depending on the DEPTH_TEXTURE_MODE value.
        // In theory we only need to apply this for depth textures, but it is
        // simpler to apply to all textures.
        state_.api()->glTexParameteriFn(target, GL_DEPTH_TEXTURE_MODE, GL_RED);
      }
    }
  } else {
    state_.api()->glBindTextureFn(target, 0);
  }

  TextureUnit& unit = state_.texture_units[state_.active_texture_unit];
  unit.bind_target = target;
  unit.SetInfoForTarget(target, texture_ref);
  return error::kNoError;
}

error::Error RasterDecoder::HandleTraceBeginCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::TraceBeginCHROMIUM& c =
      *static_cast<const volatile gles2::cmds::TraceBeginCHROMIUM*>(cmd_data);
  Bucket* category_bucket = GetBucket(c.category_bucket_id);
  Bucket* name_bucket = GetBucket(c.name_bucket_id);
  if (!category_bucket || category_bucket->size() == 0 || !name_bucket ||
      name_bucket->size() == 0) {
    return error::kInvalidArguments;
  }

  std::string category_name;
  std::string trace_name;
  if (!category_bucket->GetAsString(&category_name) ||
      !name_bucket->GetAsString(&trace_name)) {
    return error::kInvalidArguments;
  }

  debug_marker_manager_.PushGroup(trace_name);
  return error::kNoError;
}

error::Error RasterDecoder::HandleTraceEndCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  debug_marker_manager_.PopGroup();
  return error::kNoError;
}

error::Error RasterDecoder::HandleInsertFenceSyncCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::InsertFenceSyncCHROMIUM& c =
      *static_cast<const volatile gles2::cmds::InsertFenceSyncCHROMIUM*>(
          cmd_data);

  const uint64_t release_count = c.release_count();
  client_->OnFenceSyncRelease(release_count);
  // Exit inner command processing loop so that we check the scheduling state
  // and yield if necessary as we may have unblocked a higher priority context.
  ExitCommandProcessingEarly();
  return error::kNoError;
}

error::Error RasterDecoder::HandleWaitSyncTokenCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::WaitSyncTokenCHROMIUM& c =
      *static_cast<const volatile gles2::cmds::WaitSyncTokenCHROMIUM*>(
          cmd_data);

  const gpu::CommandBufferNamespace kMinNamespaceId =
      gpu::CommandBufferNamespace::INVALID;
  const gpu::CommandBufferNamespace kMaxNamespaceId =
      gpu::CommandBufferNamespace::NUM_COMMAND_BUFFER_NAMESPACES;

  gpu::CommandBufferNamespace namespace_id =
      static_cast<gpu::CommandBufferNamespace>(c.namespace_id);
  if ((namespace_id < static_cast<int32_t>(kMinNamespaceId)) ||
      (namespace_id >= static_cast<int32_t>(kMaxNamespaceId))) {
    namespace_id = gpu::CommandBufferNamespace::INVALID;
  }
  const CommandBufferId command_buffer_id =
      CommandBufferId::FromUnsafeValue(c.command_buffer_id());
  const uint64_t release = c.release_count();

  gpu::SyncToken sync_token;
  sync_token.Set(namespace_id, 0, command_buffer_id, release);
  return client_->OnWaitSyncToken(sync_token) ? error::kDeferCommandUntilLater
                                              : error::kNoError;
}

bool RasterDecoder::GenTexturesHelper(GLsizei n, const GLuint* client_ids) {
  for (GLsizei ii = 0; ii < n; ++ii) {
    if (group_->texture_manager()->GetTexture(client_ids[ii])) {
      return false;
    }
  }
  std::unique_ptr<GLuint[]> service_ids(new GLuint[n]);
  state_.api()->glGenTexturesFn(n, service_ids.get());
  for (GLsizei ii = 0; ii < n; ++ii) {
    group_->texture_manager()->CreateTexture(client_ids[ii], service_ids[ii]);
  }
  return true;
}

error::Error RasterDecoder::HandleCreateAndConsumeTextureINTERNAL(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::CreateAndConsumeTextureINTERNALImmediate& c =
      *static_cast<const volatile gles2::cmds::
                       CreateAndConsumeTextureINTERNALImmediate*>(cmd_data);
  GLuint texture_id = static_cast<GLuint>(c.texture);
  uint32_t data_size;
  if (!GLES2Util::ComputeDataSize<GLbyte, 16>(1, &data_size)) {
    return error::kOutOfBounds;
  }
  if (data_size > immediate_data_size) {
    return error::kOutOfBounds;
  }
  volatile const GLbyte* mailbox = GetImmediateDataAs<volatile const GLbyte*>(
      c, data_size, immediate_data_size);
  if (mailbox == NULL) {
    return error::kOutOfBounds;
  }

  TRACE_EVENT2("gpu", "RasterDecoder::DoCreateAndConsumeTextureINTERNAL",
               "context", logger_.GetLogPrefix(), "mailbox[0]",
               static_cast<unsigned char>(mailbox[0]));
  Mailbox real_mailbox = Mailbox::FromVolatile(
      *reinterpret_cast<const volatile Mailbox*>(mailbox));
  DLOG_IF(ERROR, !real_mailbox.Verify())
      << "CreateAndConsumeTextureCHROMIUM was "
         "passed a mailbox that was not "
         "generated by GenMailboxCHROMIUM.";
  if (!texture_id) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION,
                       "glCreateAndConsumeTextureCHROMIUM",
                       "invalid client id");
    return error::kNoError;
  }

  TextureRef* texture_ref = group_->texture_manager()->GetTexture(texture_id);
  if (texture_ref) {
    // No need to call EnsureTextureForClientId here, the texture already has
    // an associated texture.
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION,
                       "glCreateAndConsumeTextureCHROMIUM",
                       "client id already in use");
    return error::kNoError;
  }
  Texture* texture = static_cast<Texture*>(
      group_->mailbox_manager()->ConsumeTexture(real_mailbox));
  if (!texture) {
    bool result = GenTexturesHelper(1, &texture_id);
    DCHECK(result);
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION,
                       "glCreateAndConsumeTextureCHROMIUM",
                       "invalid mailbox name");
    return error::kNoError;
  }

  texture_ref = group_->texture_manager()->Consume(texture_id, texture);
  return error::kNoError;
}

bool RasterDecoder::IsCompressedTextureFormat(unsigned format) {
  return feature_info_->validators()->compressed_texture_format.IsValid(format);
}

void RasterDecoder::TexStorageImpl(GLenum target,
                                   GLsizei levels,
                                   GLenum internal_format,
                                   GLsizei width,
                                   GLsizei height,
                                   GLsizei depth,
                                   ContextState::Dimension dimension,
                                   const char* function_name) {
  if (levels == 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, function_name, "levels == 0");
    return;
  }
  bool is_compressed_format = IsCompressedTextureFormat(internal_format);
  if (is_compressed_format && target == GL_TEXTURE_3D) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, function_name,
                       "target invalid for format");
    return;
  }
  // The glTexStorage entry points require width, height, and depth to be
  // at least 1, but the other texture entry points (those which use
  // ValidForTarget) do not. So we have to add an extra check here.
  bool is_invalid_texstorage_size = width < 1 || height < 1 || depth < 1;
  if (!texture_manager()->ValidForTarget(target, 0, width, height, depth) ||
      is_invalid_texstorage_size ||
      TextureManager::ComputeMipMapCount(target, width, height, depth) <
          levels) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, function_name,
                       "dimensions out of range");
    return;
  }
  TextureRef* texture_ref =
      texture_manager()->GetTextureInfoForTarget(&state_, target);
  if (!texture_ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, function_name,
                       "unknown texture for target");
    return;
  }
  Texture* texture = texture_ref->texture();
  if (texture->IsImmutable()) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, function_name,
                       "texture is immutable");
    return;
  }

  GLenum format =
      TextureManager::ExtractFormatFromStorageFormat(internal_format);
  GLenum type = TextureManager::ExtractTypeFromStorageFormat(internal_format);

  std::vector<int32_t> level_size(levels);
  {
    GLsizei level_width = width;
    GLsizei level_height = height;
    GLsizei level_depth = depth;
    base::CheckedNumeric<uint32_t> estimated_size(0);
    PixelStoreParams params;
    params.alignment = 1;
    for (int ii = 0; ii < levels; ++ii) {
      uint32_t size;
      if (is_compressed_format) {
        if (!GetCompressedTexSizeInBytes(function_name, level_width,
                                         level_height, level_depth,
                                         internal_format, &level_size[ii])) {
          // GetCompressedTexSizeInBytes() already generates a GL error.
          return;
        }
        size = static_cast<uint32_t>(level_size[ii]);
      } else {
        if (!GLES2Util::ComputeImageDataSizesES3(
                level_width, level_height, level_depth, format, type, params,
                &size, nullptr, nullptr, nullptr, nullptr)) {
          LOCAL_SET_GL_ERROR(GL_OUT_OF_MEMORY, function_name,
                             "dimensions too large");
          return;
        }
      }
      estimated_size += size;
      level_width = std::max(1, level_width >> 1);
      level_height = std::max(1, level_height >> 1);
      if (target == GL_TEXTURE_3D)
        level_depth = std::max(1, level_depth >> 1);
    }
    if (!estimated_size.IsValid() ||
        !EnsureGPUMemoryAvailable(estimated_size.ValueOrDefault(0))) {
      LOCAL_SET_GL_ERROR(GL_OUT_OF_MEMORY, function_name, "out of memory");
      return;
    }
  }

  GLenum compatibility_internal_format = internal_format;
  const CompressedFormatInfo* format_info =
      GetCompressedFormatInfo(internal_format);
  if (format_info != nullptr && !format_info->support_check(*feature_info_)) {
    compatibility_internal_format = format_info->decompressed_internal_format;
  }

  if (workarounds().reset_base_mipmap_level_before_texstorage &&
      texture->base_level() > 0)
    api()->glTexParameteriFn(target, GL_TEXTURE_BASE_LEVEL, 0);

  // TODO(zmo): We might need to emulate TexStorage using TexImage or
  // CompressedTexImage on Mac OSX where we expose ES3 APIs when the underlying
  // driver is lower than 4.2 and ARB_texture_storage extension doesn't exist.
  if (dimension == ContextState::k2D) {
    api()->glTexStorage2DEXTFn(target, levels, compatibility_internal_format,
                               width, height);
  } else {
    api()->glTexStorage3DFn(target, levels, compatibility_internal_format,
                            width, height, depth);
  }
  if (workarounds().reset_base_mipmap_level_before_texstorage &&
      texture->base_level() > 0)
    api()->glTexParameteriFn(target, GL_TEXTURE_BASE_LEVEL,
                             texture->base_level());

  {
    GLsizei level_width = width;
    GLsizei level_height = height;
    GLsizei level_depth = depth;

    GLenum adjusted_internal_format =
        feature_info_->IsWebGL1OrES2Context() ? format : internal_format;
    for (int ii = 0; ii < levels; ++ii) {
      if (target == GL_TEXTURE_CUBE_MAP) {
        for (int jj = 0; jj < 6; ++jj) {
          GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + jj;
          texture_manager()->SetLevelInfo(
              texture_ref, face, ii, adjusted_internal_format, level_width,
              level_height, 1, 0, format, type, gfx::Rect());
        }
      } else {
        texture_manager()->SetLevelInfo(
            texture_ref, target, ii, adjusted_internal_format, level_width,
            level_height, level_depth, 0, format, type, gfx::Rect());
      }
      level_width = std::max(1, level_width >> 1);
      level_height = std::max(1, level_height >> 1);
      if (target == GL_TEXTURE_3D)
        level_depth = std::max(1, level_depth >> 1);
    }
    texture->SetImmutable(true);
  }
}

error::Error RasterDecoder::HandleTexStorage2DEXT(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::TexStorage2DEXT& c =
      *static_cast<const volatile gles2::cmds::TexStorage2DEXT*>(cmd_data);
  if (!features().ext_texture_storage) {
    return error::kUnknownCommand;
  }

  GLenum target = static_cast<GLenum>(c.target);
  GLsizei levels = static_cast<GLsizei>(c.levels);
  GLenum internalFormat = static_cast<GLenum>(c.internalFormat);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  if (!validators_->texture_bind_target.IsValid(target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glTexStorage2DEXT", target, "target");
    return error::kNoError;
  }
  if (levels < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2DEXT", "levels < 0");
    return error::kNoError;
  }
  if (!validators_->texture_internal_format_storage.IsValid(internalFormat)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glTexStorage2DEXT", internalFormat,
                                    "internalFormat");
    return error::kNoError;
  }
  if (width < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2DEXT", "width < 0");
    return error::kNoError;
  }
  if (height < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2DEXT", "height < 0");
    return error::kNoError;
  }
  TRACE_EVENT2("gpu", "RasterDecoder::DoTexStorage2D", "width", width, "height",
               height);
  TexStorageImpl(target, levels, internalFormat, width, height, 1,
                 ContextState::k2D, "glTexStorage2D");
  return error::kNoError;
}

bool RasterDecoder::GetCompressedTexSizeInBytes(const char* function_name,
                                                GLsizei width,
                                                GLsizei height,
                                                GLsizei depth,
                                                GLenum format,
                                                GLsizei* size_in_bytes) {
  base::CheckedNumeric<GLsizei> bytes_required(0);

  switch (format) {
    case GL_ATC_RGB_AMD:
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
    case GL_ETC1_RGB8_OES:
      bytes_required = (width + kS3TCBlockWidth - 1) / kS3TCBlockWidth;
      bytes_required *= (height + kS3TCBlockHeight - 1) / kS3TCBlockHeight;
      bytes_required *= kS3TCDXT1BlockSize;
      break;
    case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
    case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
    case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
    case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
    case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
    case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
    case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
    case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
    case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
    case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
    case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
    case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
    case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
    case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR: {
      const int index =
          (format < GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR)
              ? static_cast<int>(format - GL_COMPRESSED_RGBA_ASTC_4x4_KHR)
              : static_cast<int>(format -
                                 GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR);

      const int kBlockWidth = kASTCBlockArray[index].blockWidth;
      const int kBlockHeight = kASTCBlockArray[index].blockHeight;

      bytes_required = (width + kBlockWidth - 1) / kBlockWidth;
      bytes_required *= (height + kBlockHeight - 1) / kBlockHeight;

      bytes_required *= kASTCBlockSize;
      break;
    }
    case GL_ATC_RGBA_EXPLICIT_ALPHA_AMD:
    case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
      bytes_required = (width + kS3TCBlockWidth - 1) / kS3TCBlockWidth;
      bytes_required *= (height + kS3TCBlockHeight - 1) / kS3TCBlockHeight;
      bytes_required *= kS3TCDXT3AndDXT5BlockSize;
      break;
    case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
    case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
      bytes_required = std::max(width, 8);
      bytes_required *= std::max(height, 8);
      bytes_required *= 4;
      bytes_required += 7;
      bytes_required /= 8;
      break;
    case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
    case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
      bytes_required = std::max(width, 16);
      bytes_required *= std::max(height, 8);
      bytes_required *= 2;
      bytes_required += 7;
      bytes_required /= 8;
      break;

    // ES3 formats.
    case GL_COMPRESSED_R11_EAC:
    case GL_COMPRESSED_SIGNED_R11_EAC:
    case GL_COMPRESSED_RGB8_ETC2:
    case GL_COMPRESSED_SRGB8_ETC2:
    case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
      bytes_required =
          (width + kEACAndETC2BlockSize - 1) / kEACAndETC2BlockSize;
      bytes_required *=
          (height + kEACAndETC2BlockSize - 1) / kEACAndETC2BlockSize;
      bytes_required *= 8;
      bytes_required *= depth;
      break;
    case GL_COMPRESSED_RG11_EAC:
    case GL_COMPRESSED_SIGNED_RG11_EAC:
    case GL_COMPRESSED_RGBA8_ETC2_EAC:
    case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
      bytes_required =
          (width + kEACAndETC2BlockSize - 1) / kEACAndETC2BlockSize;
      bytes_required *=
          (height + kEACAndETC2BlockSize - 1) / kEACAndETC2BlockSize;
      bytes_required *= 16;
      bytes_required *= depth;
      break;
    default:
      LOCAL_SET_GL_ERROR_INVALID_ENUM(function_name, format, "format");
      return false;
  }

  if (!bytes_required.IsValid()) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, function_name, "invalid size");
    return false;
  }

  *size_in_bytes = bytes_required.ValueOrDefault(0);
  return true;
}

error::Error RasterDecoder::HandleGenTexturesImmediate(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::GenTexturesImmediate& c =
      *static_cast<const volatile gles2::cmds::GenTexturesImmediate*>(cmd_data);
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32_t data_size;
  if (!SafeMultiplyUint32(n, sizeof(GLuint), &data_size)) {
    return error::kOutOfBounds;
  }
  volatile GLuint* textures =
      GetImmediateDataAs<volatile GLuint*>(c, data_size, immediate_data_size);
  if (textures == NULL) {
    return error::kOutOfBounds;
  }
  auto textures_copy = std::make_unique<GLuint[]>(n);
  GLuint* textures_safe = textures_copy.get();
  std::copy(textures, textures + n, textures_safe);
  if (!CheckUniqueAndNonNullIds(n, textures_safe) ||
      !GenTexturesHelper(n, textures_safe)) {
    return error::kInvalidArguments;
  }
  return error::kNoError;
}

error::Error RasterDecoder::HandleTexParameteri(uint32_t immediate_data_size,
                                                const volatile void* cmd_data) {
  const volatile gles2::cmds::TexParameteri& c =
      *static_cast<const volatile gles2::cmds::TexParameteri*>(cmd_data);
  GLenum target = static_cast<GLenum>(c.target);
  GLenum pname = static_cast<GLenum>(c.pname);
  GLint param = static_cast<GLint>(c.param);
  if (!validators_->texture_bind_target.IsValid(target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glTexParameteri", target, "target");
    return error::kNoError;
  }
  if (!validators_->texture_parameter.IsValid(pname)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glTexParameteri", pname, "pname");
    return error::kNoError;
  }
  TextureRef* texture =
      texture_manager()->GetTextureInfoForTarget(&state_, target);
  if (!texture) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexParameteri", "unknown texture");
    return error::kNoError;
  }

  texture_manager()->SetParameteri("glTexParameteri", GetErrorState(), texture,
                                   pname, param);
  return error::kNoError;
}

error::Error RasterDecoder::HandleBindTexImage2DCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::BindTexImage2DCHROMIUM& c =
      *static_cast<const volatile gles2::cmds::BindTexImage2DCHROMIUM*>(
          cmd_data);
  GLenum target = static_cast<GLenum>(c.target);
  GLint imageId = static_cast<GLint>(c.imageId);
  if (!validators_->texture_bind_target.IsValid(target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glBindTexImage2DCHROMIUM", target,
                                    "target");
    return error::kNoError;
  }

  TRACE_EVENT0("gpu", "RasterDecoder::DoBindTexImage2DCHROMIUM");
  BindTexImage2DCHROMIUMImpl("glBindTexImage2DCHROMIUM", target, 0, imageId);
  return error::kNoError;
}

void RasterDecoder::BindTexImage2DCHROMIUMImpl(const char* function_name,
                                               GLenum target,
                                               GLenum internalformat,
                                               GLint image_id) {
  if (target == GL_TEXTURE_CUBE_MAP) {
    LOCAL_SET_GL_ERROR(GL_INVALID_ENUM, function_name, "invalid target");
    return;
  }

  // Default target might be conceptually valid, but disallow it to avoid
  // accidents.
  TextureRef* texture_ref =
      texture_manager()->GetTextureInfoForTargetUnlessDefault(&state_, target);
  if (!texture_ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, function_name, "no texture bound");
    return;
  }

  gl::GLImage* image = image_manager()->LookupImage(image_id);
  if (!image) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, function_name,
                       "no image found with the given ID");
    return;
  }

  Texture::ImageState image_state = Texture::UNBOUND;

  {
    ScopedGLErrorSuppressor suppressor(
        "RasterDecoder::DoBindTexImage2DCHROMIUM", GetErrorState());

    // Note: We fallback to using CopyTexImage() before the texture is used
    // when BindTexImage() fails.
    if (internalformat) {
      if (image->BindTexImageWithInternalformat(target, internalformat))
        image_state = Texture::BOUND;
    } else {
      if (image->BindTexImage(target))
        image_state = Texture::BOUND;
    }
  }

  gfx::Size size = image->GetSize();
  GLenum texture_internalformat =
      internalformat ? internalformat : image->GetInternalFormat();
  texture_manager()->SetLevelInfo(texture_ref, target, 0,
                                  texture_internalformat, size.width(),
                                  size.height(), 1, 0, texture_internalformat,
                                  GL_UNSIGNED_BYTE, gfx::Rect(size));
  texture_manager()->SetLevelImage(texture_ref, target, 0, image, image_state);
}

error::Error RasterDecoder::HandleGenQueriesEXTImmediate(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::GenQueriesEXTImmediate& c =
      *static_cast<const volatile gles2::cmds::GenQueriesEXTImmediate*>(
          cmd_data);
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32_t data_size;
  if (!SafeMultiplyUint32(n, sizeof(GLuint), &data_size)) {
    return error::kOutOfBounds;
  }
  volatile GLuint* queries =
      GetImmediateDataAs<volatile GLuint*>(c, data_size, immediate_data_size);
  if (queries == NULL) {
    return error::kOutOfBounds;
  }
  auto queries_copy = std::make_unique<GLuint[]>(n);
  GLuint* queries_safe = queries_copy.get();
  std::copy(queries, queries + n, queries_safe);
  if (!CheckUniqueAndNonNullIds(n, queries_safe) ||
      !GenQueriesEXTHelper(n, queries_safe)) {
    return error::kInvalidArguments;
  }
  return error::kNoError;
}

error::Error RasterDecoder::HandleDeleteQueriesEXTImmediate(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::DeleteQueriesEXTImmediate& c =
      *static_cast<const volatile gles2::cmds::DeleteQueriesEXTImmediate*>(
          cmd_data);
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32_t data_size;
  if (!SafeMultiplyUint32(n, sizeof(GLuint), &data_size)) {
    return error::kOutOfBounds;
  }
  volatile const GLuint* queries = GetImmediateDataAs<volatile const GLuint*>(
      c, data_size, immediate_data_size);
  if (queries == NULL) {
    return error::kOutOfBounds;
  }
  DeleteQueriesEXTHelper(n, queries);
  return error::kNoError;
}

error::Error RasterDecoder::HandleCopySubTextureCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::CopySubTextureCHROMIUM& c =
      *static_cast<const volatile gles2::cmds::CopySubTextureCHROMIUM*>(
          cmd_data);
  GLuint source_id = static_cast<GLuint>(c.source_id);
  GLint source_level = static_cast<GLint>(c.source_level);
  GLenum dest_target = static_cast<GLenum>(c.dest_target);
  GLuint dest_id = static_cast<GLuint>(c.dest_id);
  GLint dest_level = static_cast<GLint>(c.dest_level);
  GLint xoffset = static_cast<GLint>(c.xoffset);
  GLint yoffset = static_cast<GLint>(c.yoffset);
  GLint x = static_cast<GLint>(c.x);
  GLint y = static_cast<GLint>(c.y);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  GLboolean unpack_flip_y = static_cast<GLboolean>(c.unpack_flip_y);
  GLboolean unpack_premultiply_alpha =
      static_cast<GLboolean>(c.unpack_premultiply_alpha);
  GLboolean unpack_unmultiply_alpha =
      static_cast<GLboolean>(c.unpack_unmultiply_alpha);
  if (!validators_->texture_target.IsValid(dest_target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glCopySubTextureCHROMIUM", dest_target,
                                    "dest_target");
    return error::kNoError;
  }
  if (width < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTextureCHROMIUM",
                       "width < 0");
    return error::kNoError;
  }
  if (height < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTextureCHROMIUM",
                       "height < 0");
    return error::kNoError;
  }
  DoCopySubTextureCHROMIUM(source_id, source_level, dest_target, dest_id,
                           dest_level, xoffset, yoffset, x, y, width, height,
                           unpack_flip_y, unpack_premultiply_alpha,
                           unpack_unmultiply_alpha);
  return error::kNoError;
}

void RasterDecoder::DoCopySubTextureCHROMIUM(
    GLuint source_id,
    GLint source_level,
    GLenum dest_target,
    GLuint dest_id,
    GLint dest_level,
    GLint xoffset,
    GLint yoffset,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLboolean unpack_flip_y,
    GLboolean unpack_premultiply_alpha,
    GLboolean unpack_unmultiply_alpha) {
  TRACE_EVENT0("gpu", "RasterDecoder::DoCopySubTextureCHROMIUM");

  static const char kFunctionName[] = "glCopySubTextureCHROMIUM";
  TextureRef* source_texture_ref = GetTexture(source_id);
  TextureRef* dest_texture_ref = GetTexture(dest_id);

  if (!ValidateCopyTextureCHROMIUMTextures(
          kFunctionName, dest_target, source_texture_ref, dest_texture_ref)) {
    return;
  }

  if (source_level < 0 || dest_level < 0 ||
      (feature_info_->IsWebGL1OrES2Context() && source_level > 0)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, kFunctionName,
                       "source_level or dest_level out of range");
    return;
  }

  Texture* source_texture = source_texture_ref->texture();
  Texture* dest_texture = dest_texture_ref->texture();
  GLenum source_target = source_texture->target();
  GLenum dest_binding_target = dest_texture->target();
  int source_width = 0;
  int source_height = 0;
  gl::GLImage* image =
      source_texture->GetLevelImage(source_target, source_level);
  if (image) {
    gfx::Size size = image->GetSize();
    source_width = size.width();
    source_height = size.height();
    if (source_width <= 0 || source_height <= 0) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, kFunctionName, "invalid image size");
      return;
    }

    // Ideally we should not need to check that the sub-texture copy rectangle
    // is valid in two different ways, here and below. However currently there
    // is no guarantee that a texture backed by a GLImage will have sensible
    // level info. If this synchronization were to be enforced then this and
    // other functions in this file could be cleaned up.
    // See: https://crbug.com/586476
    int32_t max_x;
    int32_t max_y;
    if (!SafeAddInt32(x, width, &max_x) || !SafeAddInt32(y, height, &max_y) ||
        x < 0 || y < 0 || max_x > source_width || max_y > source_height) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, kFunctionName,
                         "source texture bad dimensions");
      return;
    }
  } else {
    if (!source_texture->GetLevelSize(source_target, source_level,
                                      &source_width, &source_height, nullptr)) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, kFunctionName,
                         "source texture has no data for level");
      return;
    }

    // Check that this type of texture is allowed.
    if (!texture_manager()->ValidForTarget(source_target, source_level,
                                           source_width, source_height, 1)) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, kFunctionName,
                         "source texture bad dimensions");
      return;
    }

    if (!source_texture->ValidForTexture(source_target, source_level, x, y, 0,
                                         width, height, 1)) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, kFunctionName,
                         "source texture bad dimensions.");
      return;
    }
  }

  GLenum source_type = 0;
  GLenum source_internal_format = 0;
  source_texture->GetLevelType(source_target, source_level, &source_type,
                               &source_internal_format);

  GLenum dest_type = 0;
  GLenum dest_internal_format = 0;
  bool dest_level_defined = dest_texture->GetLevelType(
      dest_target, dest_level, &dest_type, &dest_internal_format);
  if (!dest_level_defined) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, kFunctionName,
                       "destination texture is not defined");
    return;
  }
  if (!dest_texture->ValidForTexture(dest_target, dest_level, xoffset, yoffset,
                                     0, width, height, 1)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, kFunctionName,
                       "destination texture bad dimensions.");
    return;
  }

  if (!ValidateCopyTextureCHROMIUMInternalFormats(
          kFunctionName, source_internal_format, dest_internal_format)) {
    return;
  }

  if (feature_info_->feature_flags().desktop_srgb_support) {
    bool enable_framebuffer_srgb =
        GLES2Util::GetColorEncodingFromInternalFormat(source_internal_format) ==
            GL_SRGB ||
        GLES2Util::GetColorEncodingFromInternalFormat(dest_internal_format) ==
            GL_SRGB;
    state_.EnableDisableFramebufferSRGB(enable_framebuffer_srgb);
  }

  // Clear the source texture if necessary.
  if (!texture_manager()->ClearTextureLevel(this, source_texture_ref,
                                            source_target, source_level)) {
    LOCAL_SET_GL_ERROR(GL_OUT_OF_MEMORY, kFunctionName,
                       "source texture dimensions too big");
    return;
  }

  if (!InitializeCopyTextureCHROMIUM(kFunctionName))
    return;

  int dest_width = 0;
  int dest_height = 0;
  bool ok = dest_texture->GetLevelSize(dest_target, dest_level, &dest_width,
                                       &dest_height, nullptr);
  DCHECK(ok);
  if (xoffset != 0 || yoffset != 0 || width != dest_width ||
      height != dest_height) {
    gfx::Rect cleared_rect;
    if (TextureManager::CombineAdjacentRects(
            dest_texture->GetLevelClearedRect(dest_target, dest_level),
            gfx::Rect(xoffset, yoffset, width, height), &cleared_rect)) {
      DCHECK_GE(cleared_rect.size().GetArea(),
                dest_texture->GetLevelClearedRect(dest_target, dest_level)
                    .size()
                    .GetArea());
      texture_manager()->SetLevelClearedRect(dest_texture_ref, dest_target,
                                             dest_level, cleared_rect);
    } else {
      // Otherwise clear part of texture level that is not already cleared.
      if (!texture_manager()->ClearTextureLevel(this, dest_texture_ref,
                                                dest_target, dest_level)) {
        LOCAL_SET_GL_ERROR(GL_OUT_OF_MEMORY, kFunctionName,
                           "destination texture dimensions too big");
        return;
      }
    }
  } else {
    texture_manager()->SetLevelCleared(dest_texture_ref, dest_target,
                                       dest_level, true);
  }

  // Try using GLImage::CopyTexSubImage when possible.
  bool unpack_premultiply_alpha_change =
      (unpack_premultiply_alpha ^ unpack_unmultiply_alpha) != 0;
  // TODO(qiankun.miao@intel.com): Support level > 0 for CopyTexSubImage.
  if (image && dest_internal_format == source_internal_format &&
      dest_level == 0 && !unpack_flip_y && !unpack_premultiply_alpha_change) {
    ScopedTextureBinder binder(&state_, dest_texture->service_id(),
                               dest_binding_target);
    if (image->CopyTexSubImage(dest_target, gfx::Point(xoffset, yoffset),
                               gfx::Rect(x, y, width, height))) {
      return;
    }
  }

  DoBindOrCopyTexImageIfNeeded(source_texture, source_target, 0);

  // GL_TEXTURE_EXTERNAL_OES texture requires apply a transform matrix
  // before presenting.
  if (source_target == GL_TEXTURE_EXTERNAL_OES) {
    if (GLStreamTextureImage* image =
            source_texture->GetLevelStreamTextureImage(GL_TEXTURE_EXTERNAL_OES,
                                                       source_level)) {
      GLfloat transform_matrix[16];
      image->GetTextureMatrix(transform_matrix);
      copy_texture_CHROMIUM_->DoCopySubTextureWithTransform(
          this, source_target, source_texture->service_id(), source_level,
          source_internal_format, dest_target, dest_texture->service_id(),
          dest_level, dest_internal_format, xoffset, yoffset, x, y, width,
          height, dest_width, dest_height, source_width, source_height,
          unpack_flip_y == GL_TRUE, unpack_premultiply_alpha == GL_TRUE,
          unpack_unmultiply_alpha == GL_TRUE, transform_matrix,
          copy_tex_image_blit_.get());
      return;
    }
  }

  CopyTextureMethod method = getCopyTextureCHROMIUMMethod(
      source_target, source_level, source_internal_format, source_type,
      dest_binding_target, dest_level, dest_internal_format,
      unpack_flip_y == GL_TRUE, unpack_premultiply_alpha == GL_TRUE,
      unpack_unmultiply_alpha == GL_TRUE);
#if defined(OS_CHROMEOS) && defined(ARCH_CPU_X86_FAMILY)
  // glDrawArrays is faster than glCopyTexSubImage2D on IA Mesa driver,
  // although opposite in Android.
  // TODO(dshwang): After Mesa fixes this issue, remove this hack.
  // https://bugs.freedesktop.org/show_bug.cgi?id=98478, crbug.com/535198.
  if (Texture::ColorRenderable(GetFeatureInfo(), dest_internal_format,
                               dest_texture->IsImmutable()) &&
      method == DIRECT_COPY) {
    method = DIRECT_DRAW;
  }
#endif

  copy_texture_CHROMIUM_->DoCopySubTexture(
      this, source_target, source_texture->service_id(), source_level,
      source_internal_format, dest_target, dest_texture->service_id(),
      dest_level, dest_internal_format, xoffset, yoffset, x, y, width, height,
      dest_width, dest_height, source_width, source_height,
      unpack_flip_y == GL_TRUE, unpack_premultiply_alpha == GL_TRUE,
      unpack_unmultiply_alpha == GL_TRUE, method, copy_tex_image_blit_.get());
}

bool RasterDecoder::ValidateCopyTextureCHROMIUMTextures(
    const char* function_name,
    GLenum dest_target,
    TextureRef* source_texture_ref,
    TextureRef* dest_texture_ref) {
  if (!source_texture_ref || !dest_texture_ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, function_name, "unknown texture id");
    return false;
  }

  Texture* source_texture = source_texture_ref->texture();
  Texture* dest_texture = dest_texture_ref->texture();
  if (source_texture == dest_texture) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, function_name,
                       "source and destination textures are the same");
    return false;
  }

  if (dest_texture->target() !=
      GLES2Util::GLFaceTargetToTextureTarget(dest_target)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, function_name,
                       "target should be aligned with dest target");
    return false;
  }
  switch (dest_texture->target()) {
    case GL_TEXTURE_2D:
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_RECTANGLE_ARB:
      break;
    default:
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, function_name,
                         "invalid dest texture target binding");
      return false;
  }

  switch (source_texture->target()) {
    case GL_TEXTURE_2D:
    case GL_TEXTURE_RECTANGLE_ARB:
    case GL_TEXTURE_EXTERNAL_OES:
      break;
    default:
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, function_name,
                         "invalid source texture target binding");
      return false;
  }
  return true;
}

bool RasterDecoder::CanUseCopyTextureCHROMIUMInternalFormat(
    GLenum dest_internal_format) {
  switch (dest_internal_format) {
    case GL_RGB:
    case GL_RGBA:
    case GL_RGB8:
    case GL_RGBA8:
    case GL_BGRA_EXT:
    case GL_BGRA8_EXT:
    case GL_SRGB_EXT:
    case GL_SRGB_ALPHA_EXT:
    case GL_R8:
    case GL_R8UI:
    case GL_RG8:
    case GL_RG8UI:
    case GL_SRGB8:
    case GL_RGB565:
    case GL_RGB8UI:
    case GL_SRGB8_ALPHA8:
    case GL_RGB5_A1:
    case GL_RGBA4:
    case GL_RGBA8UI:
    case GL_RGB9_E5:
    case GL_R16F:
    case GL_R32F:
    case GL_RG16F:
    case GL_RG32F:
    case GL_RGB16F:
    case GL_RGB32F:
    case GL_RGBA16F:
    case GL_RGBA32F:
    case GL_R11F_G11F_B10F:
      return true;
    default:
      return false;
  }
}

bool RasterDecoder::ValidateCopyTextureCHROMIUMInternalFormats(
    const char* function_name,
    GLenum source_internal_format,
    GLenum dest_internal_format) {
  bool valid_dest_format = false;
  // TODO(qiankun.miao@intel.com): ALPHA, LUMINANCE and LUMINANCE_ALPHA formats
  // are not supported on GL core profile. See crbug.com/577144. Enable the
  // workaround for glCopyTexImage and glCopyTexSubImage in
  // gles2_cmd_copy_tex_image.cc for glCopyTextureCHROMIUM implementation.
  switch (dest_internal_format) {
    case GL_RGB:
    case GL_RGBA:
    case GL_RGB8:
    case GL_RGBA8:
      valid_dest_format = true;
      break;
    case GL_BGRA_EXT:
    case GL_BGRA8_EXT:
      valid_dest_format =
          feature_info_->feature_flags().ext_texture_format_bgra8888;
      break;
    case GL_SRGB_EXT:
    case GL_SRGB_ALPHA_EXT:
      valid_dest_format = feature_info_->feature_flags().ext_srgb;
      break;
    case GL_R8:
    case GL_R8UI:
    case GL_RG8:
    case GL_RG8UI:
    case GL_SRGB8:
    case GL_RGB565:
    case GL_RGB8UI:
    case GL_SRGB8_ALPHA8:
    case GL_RGB5_A1:
    case GL_RGBA4:
    case GL_RGBA8UI:
    case GL_RGB10_A2:
      valid_dest_format = feature_info_->IsWebGL2OrES3Context();
      break;
    case GL_RGB9_E5:
    case GL_R16F:
    case GL_R32F:
    case GL_RG16F:
    case GL_RG32F:
    case GL_RGB16F:
    case GL_RGBA16F:
    case GL_R11F_G11F_B10F:
      valid_dest_format = feature_info_->ext_color_buffer_float_available();
      break;
    case GL_RGB32F:
      valid_dest_format =
          feature_info_->ext_color_buffer_float_available() ||
          feature_info_->feature_flags().chromium_color_buffer_float_rgb;
      break;
    case GL_RGBA32F:
      valid_dest_format =
          feature_info_->ext_color_buffer_float_available() ||
          feature_info_->feature_flags().chromium_color_buffer_float_rgba;
      break;
    case GL_ALPHA:
    case GL_LUMINANCE:
    case GL_LUMINANCE_ALPHA:
      valid_dest_format = true;
      break;
    default:
      valid_dest_format = false;
      break;
  }

  // TODO(aleksandar.stojiljkovic): Use sized internal formats: crbug.com/628064
  bool valid_source_format =
      source_internal_format == GL_RED || source_internal_format == GL_ALPHA ||
      source_internal_format == GL_RGB || source_internal_format == GL_RGBA ||
      source_internal_format == GL_RGB8 || source_internal_format == GL_RGBA8 ||
      source_internal_format == GL_LUMINANCE ||
      source_internal_format == GL_LUMINANCE_ALPHA ||
      source_internal_format == GL_BGRA_EXT ||
      source_internal_format == GL_BGRA8_EXT ||
      source_internal_format == GL_RGB_YCBCR_420V_CHROMIUM ||
      source_internal_format == GL_RGB_YCBCR_422_CHROMIUM ||
      source_internal_format == GL_R16_EXT;
  if (!valid_source_format) {
    std::string msg = "invalid source internal format " +
                      GLES2Util::GetStringEnum(source_internal_format);
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, function_name, msg.c_str());
    return false;
  }
  if (!valid_dest_format) {
    std::string msg = "invalid dest internal format " +
                      GLES2Util::GetStringEnum(dest_internal_format);
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, function_name, msg.c_str());
    return false;
  }

  return true;
}

bool RasterDecoder::DoBindOrCopyTexImageIfNeeded(Texture* texture,
                                                 GLenum textarget,
                                                 GLuint texture_unit) {
  // Image is already in use if texture is attached to a framebuffer.
  if (texture && !texture->IsAttachedToFramebuffer()) {
    Texture::ImageState image_state;
    gl::GLImage* image = texture->GetLevelImage(textarget, 0, &image_state);
    if (image && image_state == Texture::UNBOUND) {
      ScopedGLErrorSuppressor suppressor(
          "RasterDecoder::DoBindOrCopyTexImageIfNeeded", GetErrorState());
      if (texture_unit)
        api()->glActiveTextureFn(texture_unit);
      api()->glBindTextureFn(textarget, texture->service_id());
      if (image->BindTexImage(textarget)) {
        image_state = Texture::BOUND;
      } else {
        DoCopyTexImage(texture, textarget, image);
      }
      if (!texture_unit) {
        RestoreCurrentTextureBindings(&state_, textarget,
                                      state_.active_texture_unit);
        return false;
      }
      return true;
    }
  }
  return false;
}

bool RasterDecoder::InitializeCopyTextureCHROMIUM(const char* function_name) {
  // Defer initializing the CopyTextureCHROMIUMResourceManager until it is
  // needed because it takes 10s of milliseconds to initialize.
  if (!copy_texture_CHROMIUM_.get()) {
    LOCAL_COPY_REAL_GL_ERRORS_TO_WRAPPER(function_name);
    copy_texture_CHROMIUM_.reset(new CopyTextureCHROMIUMResourceManager());
    copy_texture_CHROMIUM_->Initialize(this, features());
    if (LOCAL_PEEK_GL_ERROR(function_name) != GL_NO_ERROR)
      return false;

    // On the desktop core profile this also needs emulation of
    // CopyTex{Sub}Image2D for luminance, alpha, and luminance_alpha
    // textures.
    if (CopyTexImageResourceManager::CopyTexImageRequiresBlit(
            feature_info_.get(), GL_LUMINANCE)) {
      if (!InitializeCopyTexImageBlitter(function_name))
        return false;
    }
  }
  return true;
}

CopyTextureMethod RasterDecoder::getCopyTextureCHROMIUMMethod(
    GLenum source_target,
    GLint source_level,
    GLenum source_internal_format,
    GLenum source_type,
    GLenum dest_target,
    GLint dest_level,
    GLenum dest_internal_format,
    bool flip_y,
    bool premultiply_alpha,
    bool unpremultiply_alpha) {
  bool premultiply_alpha_change = premultiply_alpha ^ unpremultiply_alpha;
  bool source_format_color_renderable =
      Texture::ColorRenderable(GetFeatureInfo(), source_internal_format, false);
  bool dest_format_color_renderable =
      Texture::ColorRenderable(GetFeatureInfo(), dest_internal_format, false);
  std::string output_error_msg;

  switch (dest_internal_format) {
#if defined(OS_MACOSX)
    // RGB5_A1 is not color-renderable on NVIDIA Mac, see crbug.com/676209.
    case GL_RGB5_A1:
      return DRAW_AND_READBACK;
#endif
    // RGB9_E5 isn't accepted by glCopyTexImage2D if underlying context is ES.
    case GL_RGB9_E5:
      if (gl_version_info().is_es)
        return DRAW_AND_READBACK;
      break;
    // SRGB format has color-space conversion issue. WebGL spec doesn't define
    // clearly if linear-to-srgb color space conversion is required or not when
    // uploading DOM elements to SRGB textures. WebGL conformance test expects
    // no linear-to-srgb conversion, while current GPU path for
    // CopyTextureCHROMIUM does the conversion. Do a fallback path before the
    // issue is resolved. see https://github.com/KhronosGroup/WebGL/issues/2165.
    // TODO(qiankun.miao@intel.com): revisit this once the above issue is
    // resolved.
    case GL_SRGB_EXT:
    case GL_SRGB_ALPHA_EXT:
    case GL_SRGB8:
    case GL_SRGB8_ALPHA8:
      if (feature_info_->IsWebGLContext())
        return DRAW_AND_READBACK;
      break;
    default:
      break;
  }

  // CopyTexImage* should not allow internalformat of GL_BGRA_EXT and
  // GL_BGRA8_EXT. crbug.com/663086.
  bool copy_tex_image_format_valid =
      source_internal_format != GL_BGRA_EXT &&
      dest_internal_format != GL_BGRA_EXT &&
      source_internal_format != GL_BGRA8_EXT &&
      dest_internal_format != GL_BGRA8_EXT &&
      ValidateCopyTexFormatHelper(dest_internal_format, source_internal_format,
                                  source_type, &output_error_msg);

  // TODO(qiankun.miao@intel.com): for WebGL 2.0 or OpenGL ES 3.0, both
  // DIRECT_DRAW path for dest_level > 0 and DIRECT_COPY path for source_level >
  // 0 are not available due to a framebuffer completeness bug:
  // crbug.com/678526. Once the bug is fixed, the limitation for WebGL 2.0 and
  // OpenGL ES 3.0 can be lifted.
  // For WebGL 1.0 or OpenGL ES 2.0, DIRECT_DRAW path isn't available for
  // dest_level > 0 due to level > 0 isn't supported by glFramebufferTexture2D
  // in ES2 context. DIRECT_DRAW path isn't available for cube map dest texture
  // either due to it may be cube map incomplete. Go to DRAW_AND_COPY path in
  // these cases.
  if (source_target == GL_TEXTURE_2D &&
      (dest_target == GL_TEXTURE_2D || dest_target == GL_TEXTURE_CUBE_MAP) &&
      source_format_color_renderable && copy_tex_image_format_valid &&
      source_level == 0 && !flip_y && !premultiply_alpha_change)
    return DIRECT_COPY;
  if (dest_format_color_renderable && dest_level == 0 &&
      dest_target != GL_TEXTURE_CUBE_MAP)
    return DIRECT_DRAW;

  // Draw to a fbo attaching level 0 of an intermediate texture,
  // then copy from the fbo to dest texture level with glCopyTexImage2D.
  return DRAW_AND_COPY;
}

bool RasterDecoder::ValidateCompressedCopyTextureCHROMIUM(
    const char* function_name,
    TextureRef* source_texture_ref,
    TextureRef* dest_texture_ref) {
  if (!source_texture_ref || !dest_texture_ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, function_name, "unknown texture id");
    return false;
  }

  Texture* source_texture = source_texture_ref->texture();
  Texture* dest_texture = dest_texture_ref->texture();
  if (source_texture == dest_texture) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, function_name,
                       "source and destination textures are the same");
    return false;
  }

  if (dest_texture->target() != GL_TEXTURE_2D ||
      (source_texture->target() != GL_TEXTURE_2D &&
       source_texture->target() != GL_TEXTURE_RECTANGLE_ARB &&
       source_texture->target() != GL_TEXTURE_EXTERNAL_OES)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, function_name,
                       "invalid texture target binding");
    return false;
  }

  GLenum source_type = 0;
  GLenum source_internal_format = 0;
  source_texture->GetLevelType(source_texture->target(), 0, &source_type,
                               &source_internal_format);

  bool valid_format =
      source_internal_format == GL_ATC_RGB_AMD ||
      source_internal_format == GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD ||
      source_internal_format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
      source_internal_format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ||
      source_internal_format == GL_ETC1_RGB8_OES;

  if (!valid_format) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, function_name,
                       "invalid internal format");
    return false;
  }

  return true;
}

void RasterDecoder::DoCopyTexImage(Texture* texture,
                                   GLenum textarget,
                                   gl::GLImage* image) {
  // Note: We update the state to COPIED prior to calling CopyTexImage()
  // as that allows the GLImage implemenatation to set it back to UNBOUND
  // and ensure that CopyTexImage() is called each time the texture is
  // used.
  texture->SetLevelImageState(textarget, 0, Texture::COPIED);
  bool rv = image->CopyTexImage(textarget);
  DCHECK(rv) << "CopyTexImage() failed";
}

bool RasterDecoder::InitializeCopyTexImageBlitter(const char* function_name) {
  if (!copy_tex_image_blit_.get()) {
    LOCAL_COPY_REAL_GL_ERRORS_TO_WRAPPER(function_name);
    copy_tex_image_blit_.reset(
        new CopyTexImageResourceManager(feature_info_.get()));
    copy_tex_image_blit_->Initialize(this);
    if (LOCAL_PEEK_GL_ERROR(function_name) != GL_NO_ERROR)
      return false;
  }
  return true;
}

bool RasterDecoder::ValidateCopyTexFormatHelper(GLenum internal_format,
                                                GLenum read_format,
                                                GLenum read_type,
                                                std::string* output_error_msg) {
  DCHECK(output_error_msg);
  if (read_format == 0) {
    *output_error_msg = std::string("no valid color image");
    return false;
  }
  // Check we have compatible formats.
  uint32_t channels_exist = GLES2Util::GetChannelsForFormat(read_format);
  uint32_t channels_needed = GLES2Util::GetChannelsForFormat(internal_format);
  if (!channels_needed ||
      (channels_needed & channels_exist) != channels_needed) {
    *output_error_msg = std::string("incompatible format");
    return false;
  }
  if (feature_info_->IsWebGL2OrES3Context()) {
    GLint color_encoding =
        GLES2Util::GetColorEncodingFromInternalFormat(read_format);
    bool float_mismatch = feature_info_->ext_color_buffer_float_available()
                              ? (GLES2Util::IsIntegerFormat(internal_format) !=
                                 GLES2Util::IsIntegerFormat(read_format))
                              : GLES2Util::IsFloatFormat(internal_format);
    if (color_encoding !=
            GLES2Util::GetColorEncodingFromInternalFormat(internal_format) ||
        float_mismatch ||
        (GLES2Util::IsSignedIntegerFormat(internal_format) !=
         GLES2Util::IsSignedIntegerFormat(read_format)) ||
        (GLES2Util::IsUnsignedIntegerFormat(internal_format) !=
         GLES2Util::IsUnsignedIntegerFormat(read_format))) {
      *output_error_msg = std::string("incompatible format");
      return false;
    }
  }
  if ((channels_needed & (GLES2Util::kDepth | GLES2Util::kStencil)) != 0) {
    *output_error_msg =
        std::string("can not be used with depth or stencil textures");
    return false;
  }
  if (feature_info_->IsWebGL2OrES3Context() ||
      (feature_info_->feature_flags().chromium_color_buffer_float_rgb &&
       internal_format == GL_RGB32F) ||
      (feature_info_->feature_flags().chromium_color_buffer_float_rgba &&
       internal_format == GL_RGBA32F)) {
    if (GLES2Util::IsSizedColorFormat(internal_format)) {
      int sr, sg, sb, sa;
      GLES2Util::GetColorFormatComponentSizes(read_format, read_type, &sr, &sg,
                                              &sb, &sa);
      DCHECK(sr > 0 || sg > 0 || sb > 0 || sa > 0);
      int dr, dg, db, da;
      GLES2Util::GetColorFormatComponentSizes(internal_format, 0, &dr, &dg, &db,
                                              &da);
      DCHECK(dr > 0 || dg > 0 || db > 0 || da > 0);
      if ((dr > 0 && sr != dr) || (dg > 0 && sg != dg) ||
          (db > 0 && sb != db) || (da > 0 && sa != da)) {
        *output_error_msg = std::string("incompatible color component sizes");
        return false;
      }
    }
  }
  return true;
}

void RasterDecoder::DeleteTexturesHelper(GLsizei n,
                                         const volatile GLuint* client_ids) {
  // FIXME(backer)
  // bool supports_separate_framebuffer_binds =
  // SupportsSeparateFramebufferBinds();
  bool supports_separate_framebuffer_binds = false;
  for (GLsizei ii = 0; ii < n; ++ii) {
    GLuint client_id = client_ids[ii];
    TextureRef* texture_ref = GetTexture(client_id);
    if (texture_ref) {
      UnbindTexture(texture_ref, supports_separate_framebuffer_binds);
      RemoveTexture(client_id);
    }
  }
}

error::Error RasterDecoder::HandleDeleteTexturesImmediate(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::DeleteTexturesImmediate& c =
      *static_cast<const volatile gles2::cmds::DeleteTexturesImmediate*>(
          cmd_data);
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32_t data_size;
  if (!SafeMultiplyUint32(n, sizeof(GLuint), &data_size)) {
    return error::kOutOfBounds;
  }
  volatile const GLuint* textures = GetImmediateDataAs<volatile const GLuint*>(
      c, data_size, immediate_data_size);
  if (textures == NULL) {
    return error::kOutOfBounds;
  }
  DeleteTexturesHelper(n, textures);
  return error::kNoError;
}

void RasterDecoder::UnbindTexture(TextureRef* texture_ref,
                                  bool supports_separate_framebuffer_binds) {
  // Unbind texture_ref from texture_ref units.
  state_.UnbindTexture(texture_ref);
}

error::Error RasterDecoder::HandleReleaseTexImage2DCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::ReleaseTexImage2DCHROMIUM& c =
      *static_cast<const volatile gles2::cmds::ReleaseTexImage2DCHROMIUM*>(
          cmd_data);
  GLenum target = static_cast<GLenum>(c.target);
  GLint imageId = static_cast<GLint>(c.imageId);
  if (!validators_->texture_bind_target.IsValid(target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glReleaseTexImage2DCHROMIUM", target,
                                    "target");
    return error::kNoError;
  }
  DoReleaseTexImage2DCHROMIUM(target, imageId);
  return error::kNoError;
}
void RasterDecoder::DoReleaseTexImage2DCHROMIUM(GLenum target, GLint image_id) {
  TRACE_EVENT0("gpu", "RasterDecoder::DoReleaseTexImage2DCHROMIUM");

  // Default target might be conceptually valid, but disallow it to avoid
  // accidents.
  TextureRef* texture_ref =
      texture_manager()->GetTextureInfoForTargetUnlessDefault(&state_, target);
  if (!texture_ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glReleaseTexImage2DCHROMIUM",
                       "no texture bound");
    return;
  }

  gl::GLImage* image = image_manager()->LookupImage(image_id);
  if (!image) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glReleaseTexImage2DCHROMIUM",
                       "no image found with the given ID");
    return;
  }

  Texture::ImageState image_state;

  // Do nothing when image is not currently bound.
  if (texture_ref->texture()->GetLevelImage(target, 0, &image_state) != image)
    return;

  if (image_state == Texture::BOUND) {
    ScopedGLErrorSuppressor suppressor(
        "RasterDecoder::DoReleaseTexImage2DCHROMIUM", GetErrorState());

    image->ReleaseTexImage(target);
    texture_manager()->SetLevelInfo(texture_ref, target, 0, GL_RGBA, 0, 0, 1, 0,
                                    GL_RGBA, GL_UNSIGNED_BYTE, gfx::Rect());
  }

  texture_manager()->SetLevelImage(texture_ref, target, 0, nullptr,
                                   Texture::UNBOUND);
}

error::Error RasterDecoder::HandleGetString(uint32_t immediate_data_size,
                                            const volatile void* cmd_data) {
  const volatile gles2::cmds::GetString& c =
      *static_cast<const volatile gles2::cmds::GetString*>(cmd_data);
  GLenum name = static_cast<GLenum>(c.name);
  if (!validators_->string_type.IsValid(name)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glGetString", name, "name");
    return error::kNoError;
  }

  const char* str = nullptr;
  std::string extensions;
  switch (name) {
    case GL_VERSION:
      str = GetServiceVersionString(feature_info_.get());
      break;
    case GL_SHADING_LANGUAGE_VERSION:
      str = GetServiceShadingLanguageVersionString(feature_info_.get());
      break;
    case GL_EXTENSIONS: {
      str = "";
      NOTIMPLEMENTED();
      break;
    }
    default:
      str = reinterpret_cast<const char*>(api()->glGetStringFn(name));
      break;
  }
  Bucket* bucket = CreateBucket(c.bucket_id);
  bucket->SetFromString(str);
  return error::kNoError;
}

error::Error RasterDecoder::HandleBeginRasterCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::BeginRasterCHROMIUM& c =
      *static_cast<const volatile gles2::cmds::BeginRasterCHROMIUM*>(cmd_data);
  if (!features().chromium_raster_transport) {
    return error::kUnknownCommand;
  }

  GLuint texture_id = static_cast<GLuint>(c.texture_id);
  GLuint sk_color = static_cast<GLuint>(c.sk_color);
  GLuint msaa_sample_count = static_cast<GLuint>(c.msaa_sample_count);
  GLboolean can_use_lcd_text = static_cast<GLboolean>(c.can_use_lcd_text);
  GLboolean use_distance_field_text =
      static_cast<GLboolean>(c.use_distance_field_text);
  GLint pixel_config = static_cast<GLint>(c.pixel_config);
  DoBeginRasterCHROMIUM(texture_id, sk_color, msaa_sample_count,
                        can_use_lcd_text, use_distance_field_text,
                        pixel_config);
  return error::kNoError;
}

error::Error RasterDecoder::HandleEndRasterCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  if (!features().chromium_raster_transport) {
    return error::kUnknownCommand;
  }

  DoEndRasterCHROMIUM();
  return error::kNoError;
}

void RasterDecoder::DoBeginRasterCHROMIUM(GLuint texture_id,
                                          GLuint sk_color,
                                          GLuint msaa_sample_count,
                                          GLboolean can_use_lcd_text,
                                          GLboolean use_distance_field_text,
                                          GLint pixel_config) {
  if (!gr_context_) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "chromium_raster_transport not enabled via attribs");
    return;
  }
  if (sk_surface_) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "BeginRasterCHROMIUM without EndRasterCHROMIUM");
    return;
  }

  gr_context_->resetContext();

  // This function should look identical to
  // ResourceProvider::ScopedSkSurfaceProvider.
  GrGLTextureInfo texture_info;
  auto* texture_ref = GetTexture(texture_id);
  if (!texture_ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "unknown texture id");
    return;
  }
  auto* texture = texture_ref->texture();
  int width;
  int height;
  int depth;
  if (!texture->GetLevelSize(texture->target(), 0, &width, &height, &depth)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "missing texture size info");
    return;
  }
  GLenum type;
  GLenum internal_format;
  if (!texture->GetLevelType(texture->target(), 0, &type, &internal_format)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "missing texture type info");
    return;
  }
  texture_info.fID = texture_ref->service_id();
  texture_info.fTarget = texture->target();

  if (texture->target() != GL_TEXTURE_2D &&
      texture->target() != GL_TEXTURE_RECTANGLE_ARB) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "invalid texture target");
    return;
  }

  switch (pixel_config) {
    case kRGBA_4444_GrPixelConfig:
    case kRGBA_8888_GrPixelConfig:
    case kSRGBA_8888_GrPixelConfig:
      if (internal_format != GL_RGBA8_OES && internal_format != GL_RGBA) {
        LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                           "pixel config mismatch");
        return;
      }
      break;
    case kBGRA_8888_GrPixelConfig:
    case kSBGRA_8888_GrPixelConfig:
      if (internal_format != GL_BGRA_EXT && internal_format != GL_BGRA8_EXT) {
        LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                           "pixel config mismatch");
        return;
      }
      break;
    default:
      LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                         "unsupported pixel config");
      return;
  }

  GrBackendTexture gr_texture(
      width, height, static_cast<GrPixelConfig>(pixel_config), texture_info);

  uint32_t flags =
      use_distance_field_text ? SkSurfaceProps::kUseDistanceFieldFonts_Flag : 0;
  // Use unknown pixel geometry to disable LCD text.
  SkSurfaceProps surface_props(flags, kUnknown_SkPixelGeometry);
  if (can_use_lcd_text) {
    // LegacyFontHost will get LCD text and skia figures out what type to use.
    surface_props =
        SkSurfaceProps(flags, SkSurfaceProps::kLegacyFontHost_InitType);
  }

  // Resolve requested msaa samples with GrGpu capabilities.
  int final_msaa_count = gr_context_->caps()->getSampleCount(
      msaa_sample_count, static_cast<GrPixelConfig>(pixel_config));
  sk_surface_ = SkSurface::MakeFromBackendTextureAsRenderTarget(
      gr_context_.get(), gr_texture, kTopLeft_GrSurfaceOrigin, final_msaa_count,
      nullptr, &surface_props);

  if (!sk_surface_) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "failed to create surface");
    return;
  }

  // All or nothing clearing, as no way to validate the client's input on what
  // is the "used" part of the texture.
  if (texture->IsLevelCleared(texture->target(), 0))
    return;

  // TODO(enne): this doesn't handle the case where the background color
  // changes and so any extra pixels outside the raster area that get
  // sampled may be incorrect.
  sk_surface_->getCanvas()->drawColor(sk_color);
  texture_manager()->SetLevelCleared(texture_ref, texture->target(), 0, true);
}

error::Error RasterDecoder::HandleRasterCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  if (!sk_surface_) {
    LOG(ERROR) << "RasterCHROMIUM without BeginRasterCHROMIUM";
    return error::kInvalidArguments;
  }

  alignas(
      cc::PaintOpBuffer::PaintOpAlign) char data[sizeof(cc::LargestPaintOp)];
  auto& c = *static_cast<const volatile gles2::cmds::RasterCHROMIUM*>(cmd_data);
  size_t size = c.data_size;
  char* buffer =
      GetSharedMemoryAs<char*>(c.list_shm_id, c.list_shm_offset, size);
  if (!buffer)
    return error::kOutOfBounds;

  SkCanvas* canvas = sk_surface_->getCanvas();
  SkMatrix original_ctm;
  cc::PlaybackParams playback_params(nullptr, original_ctm);
  cc::PaintOp::DeserializeOptions options;

  int op_idx = 0;
  while (size > 4) {
    size_t skip = 0;
    cc::PaintOp* deserialized_op = cc::PaintOp::Deserialize(
        buffer, size, &data[0], sizeof(cc::LargestPaintOp), &skip, options);
    if (!deserialized_op) {
      LOG(ERROR) << "RasterCHROMIUM: bad op: " << op_idx;
      return error::kInvalidArguments;
    }

    deserialized_op->Raster(canvas, playback_params);
    deserialized_op->DestroyThis();

    size -= skip;
    buffer += skip;
    op_idx++;
  }

  return error::kNoError;
}

void RasterDecoder::DoEndRasterCHROMIUM() {
  if (!sk_surface_) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "EndRasterCHROMIUM without BeginRasterCHROMIUM");
    return;
  }

  sk_surface_->prepareForExternalIO();
  sk_surface_.reset();

  RestoreState(nullptr);
}

error::Error RasterDecoder::HandleFlush(uint32_t immediate_data_size,
                                        const volatile void* cmd_data) {
  api()->glFlushFn();
  ProcessPendingQueries(false);
  return error::kNoError;
}

error::Error RasterDecoder::HandleFlushDriverCachesCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  // On Adreno Android devices we need to use a workaround to force caches to
  // clear.
  if (workarounds().unbind_egl_context_to_flush_driver_caches) {
    context_->ReleaseCurrent(nullptr);
    context_->MakeCurrent(surface_.get());
  }
  return error::kNoError;
}

}  // namespace gles2
}  // namespace gpu
