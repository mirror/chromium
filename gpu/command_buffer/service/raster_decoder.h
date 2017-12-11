// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_RASTER_DECODER_H_
#define GPU_COMMAND_BUFFER_SERVICE_RASTER_DECODER_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/command_buffer/common/command_buffer_id.h"
#include "gpu/command_buffer/common/constants.h"
#include "gpu/command_buffer/common/context_result.h"
#include "gpu/command_buffer/common/debug_marker_manager.h"
#include "gpu/command_buffer/common/gles2_cmd_format.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/command_buffer/service/context_state.h"
#include "gpu/command_buffer/service/error_state.h"
#include "gpu/command_buffer/service/gles2_cmd_copy_texture_chromium.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/command_buffer/service/image_manager.h"
#include "gpu/command_buffer/service/logger.h"
#include "gpu/command_buffer/service/memory_tracking.h"
#include "gpu/gpu_export.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"

namespace gpu {
namespace gles2 {

class QueryManager;

// This class implements the AsyncAPIInterface interface, decoding GLES2
// commands and calling GL.
class GPU_EXPORT RasterDecoder : public GLES2Decoder, public ErrorStateClient {
 public:
  RasterDecoder(GLES2DecoderClient* client,
                CommandBufferServiceBase* command_buffer_service,
                Outputter* outputter,
                ContextGroup* group);

  ~RasterDecoder() override;

  base::WeakPtr<GLES2Decoder> AsWeakPtr() override;

  // ErrorStateClient implementation.
  void OnContextLostError() override;
  void OnOutOfMemoryError() override;

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
  gpu::ContextResult Initialize(
      const scoped_refptr<gl::GLSurface>& surface,
      const scoped_refptr<gl::GLContext>& context,
      bool offscreen,
      const DisallowedFeatures& disallowed_features,
      const ContextCreationAttribHelper& attrib_helper) override;

  // Destroys the graphics context.
  void Destroy(bool have_context) override;

  // Set the surface associated with the default FBO.
  void SetSurface(const scoped_refptr<gl::GLSurface>& surface) override;
  // Releases the surface associated with the GL context.
  // The decoder should not be used until a new surface is set.
  void ReleaseSurface() override;

  void TakeFrontBuffer(const Mailbox& mailbox) override;
  void ReturnFrontBuffer(const Mailbox& mailbox, bool is_lost) override;

  // Resize an offscreen frame buffer.
  bool ResizeOffscreenFramebuffer(const gfx::Size& size) override;

  // Make this decoder's GL context current.
  bool MakeCurrent() override;

  // Gets the GLES2 Util which holds info.
  GLES2Util* GetGLES2Util() override;

  // Gets the associated GLContext.
  gl::GLContext* GetGLContext() override;

  // Gets the associated ContextGroup
  ContextGroup* GetContextGroup() override;
  const FeatureInfo* GetFeatureInfo() const override;

  Capabilities GetCapabilities() override;

  void RestoreState(const ContextState* prev_state) override;

  // Restore States.
  void RestoreActiveTexture() const override;
  void RestoreAllTextureUnitAndSamplerBindings(
      const ContextState* prev_state) const override;
  void RestoreActiveTextureUnitBinding(unsigned int target) const override;
  void RestoreBufferBinding(unsigned int target) override;
  void RestoreBufferBindings() const override;
  void RestoreFramebufferBindings() const override;
  void RestoreRenderbufferBindings() override;
  void RestoreGlobalState() const override;
  void RestoreProgramBindings() const override;
  void RestoreTextureState(unsigned service_id) const override;
  void RestoreTextureUnitBindings(unsigned unit) const override;
  void RestoreVertexAttribArray(unsigned index) override;
  void RestoreAllExternalTextureBindingsIfNeeded() override;
  void RestoreDeviceWindowRectangles() const override;

  void ClearAllAttributes() const override;
  void RestoreAllAttributes() const override;

  void SetIgnoreCachedStateForTest(bool ignore) override;
  void SetForceShaderNameHashingForTest(bool force) override;
  uint32_t GetAndClearBackbufferClearBitsForTest() override;
  size_t GetSavedBackTextureCountForTest() override;
  size_t GetCreatedBackTextureCountForTest() override;

  // Gets the QueryManager for this context.
  QueryManager* GetQueryManager() override;

  // Gets the GpuFenceManager for this context.
  GpuFenceManager* GetGpuFenceManager() override;

  // Gets the FramebufferManager for this context.
  FramebufferManager* GetFramebufferManager() override;

  // Gets the TransformFeedbackManager for this context.
  TransformFeedbackManager* GetTransformFeedbackManager() override;

  // Gets the VertexArrayManager for this context.
  VertexArrayManager* GetVertexArrayManager() override;

  // Gets the ImageManager for this context.
  ImageManager* GetImageManagerForTest() override;

  // Returns false if there are no pending queries.
  bool HasPendingQueries() const override;

  // Process any pending queries.
  void ProcessPendingQueries(bool did_finish) override;

  // Returns false if there is no idle work to be made.
  bool HasMoreIdleWork() const override;

  // Perform any idle work that needs to be made.
  void PerformIdleWork() override;

  // Whether there is state that needs to be regularly polled.
  bool HasPollingWork() const override;

  // Perform necessary polling.
  void PerformPollingWork() override;

  // Get the service texture ID corresponding to a client texture ID.
  // If no such record is found then return false.
  // FIXME
  bool GetServiceTextureId(uint32_t client_texture_id,
                           uint32_t* service_texture_id) override;

  // Gets the texture object associated with the client ID.  null is returned on
  // failure or if the texture has not been bound yet.
  TextureBase* GetTextureBase(uint32_t client_id) override;

  // Clears a level sub area of a 2D texture.
  // Returns false if a GL error should be generated.
  bool ClearLevel(Texture* texture,
                  unsigned target,
                  int level,
                  unsigned format,
                  unsigned type,
                  int xoffset,
                  int yoffset,
                  int width,
                  int height) override;

  // Clears a level sub area of a compressed 2D texture.
  // Returns false if a GL error should be generated.
  bool ClearCompressedTextureLevel(Texture* texture,
                                   unsigned target,
                                   int level,
                                   unsigned format,
                                   int width,
                                   int height) override;

  // Indicates whether a given internal format is one for a compressed
  // texture.
  bool IsCompressedTextureFormat(unsigned format) override;

  // Clears a level of a 3D texture.
  // Returns false if a GL error should be generated.
  bool ClearLevel3D(Texture* texture,
                    unsigned target,
                    int level,
                    unsigned format,
                    unsigned type,
                    int width,
                    int height,
                    int depth) override;

  ErrorState* GetErrorState() override;

  void WaitForReadPixels(base::Closure callback) override;

  // Returns true if the context was lost either by GL_ARB_robustness, forced
  // context loss or command buffer parse error.
  bool WasContextLost() const override;

  // Returns true if the context was lost specifically by GL_ARB_robustness.
  bool WasContextLostByRobustnessExtension() const override;

  // Lose this context.
  void MarkContextLost(error::ContextLostReason reason) override;

  // Updates context lost state and returns true if lost. Most callers can use
  // WasContextLost() as the GLES2Decoder will update the state internally. But
  // if making GL calls directly, to the context then this state would not be
  // updated and the caller can use this to determine if their calls failed due
  // to context loss.
  bool CheckResetStatus() override;

  Logger* GetLogger() override;

  void BeginDecoding() override;
  void EndDecoding() override;
  // Executes multiple commands.
  // Parameters:
  //    num_commands: maximum number of commands to execute from buffer.
  //    buffer: pointer to first command entry to process.
  //    num_entries: number of sequential command buffer entries in buffer.
  //    entries_processed: if not 0, is set to the number of entries processed.
  error::Error DoCommands(unsigned int num_commands,
                          const volatile void* buffer,
                          int num_entries,
                          int* entries_processed) override;

  const ContextState* GetContextState() override;
  scoped_refptr<ShaderTranslatorInterface> GetTranslator(
      unsigned int type) override;

  void BindImage(uint32_t client_texture_id,
                 uint32_t texture_target,
                 gl::GLImage* image,
                 bool can_bind_to_sampler) override;

 private:
  const GpuDriverBugWorkarounds& workarounds() const {
    return feature_info_->workarounds();
  }

  gl::GLApi* api() const { return state_.api(); }

  const FeatureInfo::FeatureFlags& features() const {
    return feature_info_->feature_flags();
  }

  ImageManager* image_manager() { return group_->image_manager(); }

  // Creates a Texture for the given texture.
  TextureRef* CreateTexture(GLuint client_id, GLuint service_id) {
    return texture_manager()->CreateTexture(client_id, service_id);
  }

  // Gets the texture info for the given texture. Returns NULL if none exists.
  TextureRef* GetTexture(GLuint client_id) const {
    return texture_manager()->GetTexture(client_id);
  }

  const TextureManager* texture_manager() const {
    return group_->texture_manager();
  }

  TextureManager* texture_manager() { return group_->texture_manager(); }

  MemoryTracker* memory_tracker() { return group_->memory_tracker(); }

  bool EnsureGPUMemoryAvailable(size_t estimated_size) {
    MemoryTracker* tracker = memory_tracker();
    if (tracker) {
      return tracker->EnsureGPUMemoryAvailable(estimated_size);
    }
    return true;
  }

  const gl::GLVersionInfo& gl_version_info() {
    return feature_info_->gl_version_info();
  }

  MailboxManager* mailbox_manager() { return group_->mailbox_manager(); }

  // Set remaining commands to process to 0 to force DoCommands to return
  // and allow context preemption and GPU watchdog checks in CommandExecutor().
  void ExitCommandProcessingEarly() { commands_to_process_ = 0; }

  const char* GetCommandName(unsigned int command_id) const;
  bool GenTexturesHelper(GLsizei n, const GLuint* client_ids);

  bool GenQueriesEXTHelper(GLsizei n, const GLuint* client_ids);
  void DeleteQueriesEXTHelper(GLsizei n, const volatile GLuint* client_ids);
  void DeleteTexturesHelper(GLsizei n, const volatile GLuint* client_ids);

  // Deletes the texture info for the given texture.
  void RemoveTexture(GLuint client_id) {
    texture_manager()->RemoveTexture(client_id);
  }
  void UnbindTexture(TextureRef* texture_ref,
                     bool supports_separate_framebuffer_binds);

  error::Error HandleReleaseTexImage2DCHROMIUM(uint32_t immediate_data_size,
                                               const volatile void* cmd_data);

  error::Error HandleBindTexture(uint32_t immediate_data_size,
                                 const volatile void* cmd_data);
  error::Error HandleCreateAndConsumeTextureINTERNAL(
      uint32_t immediate_data_size,
      const volatile void* cmd_data);
  error::Error HandleTraceBeginCHROMIUM(uint32_t immediate_data_size,
                                        const volatile void* cmd_data);
  error::Error HandleTraceEndCHROMIUM(uint32_t immediate_data_size,
                                      const volatile void* cmd_data);
  error::Error HandleInsertFenceSyncCHROMIUM(uint32_t immediate_data_size,
                                             const volatile void* cmd_data);
  error::Error HandleWaitSyncTokenCHROMIUM(uint32_t immediate_data_size,
                                           const volatile void* cmd_data);
  error::Error HandleTexStorage2DEXT(uint32_t immediate_data_size,
                                     const volatile void* cmd_data);
  error::Error HandleGenTexturesImmediate(uint32_t immediate_data_size,
                                          const volatile void* cmd_data);
  error::Error HandleTexParameteri(uint32_t immediate_data_size,
                                   const volatile void* cmd_data);
  error::Error HandleBindTexImage2DCHROMIUM(uint32_t immediate_data_size,
                                            const volatile void* cmd_data);
  error::Error HandleGenQueriesEXTImmediate(uint32_t immediate_data_size,
                                            const volatile void* cmd_data);
  error::Error HandleDeleteQueriesEXTImmediate(uint32_t immediate_data_size,
                                               const volatile void* cmd_data);
  error::Error HandleCopySubTextureCHROMIUM(uint32_t immediate_data_size,
                                            const volatile void* cmd_data);
  error::Error HandleDeleteTexturesImmediate(uint32_t immediate_data_size,
                                             const volatile void* cmd_data);
  error::Error HandleGetString(uint32_t immediate_data_size,
                               const volatile void* cmd_data);
  error::Error HandleBeginRasterCHROMIUM(uint32_t immediate_data_size,
                                         const volatile void* cmd_data);
  error::Error HandleEndRasterCHROMIUM(uint32_t immediate_data_size,
                                       const volatile void* cmd_data);
  error::Error HandleRasterCHROMIUM(uint32_t immediate_data_size,
                                    const volatile void* cmd_data);
  error::Error HandleFlush(uint32_t immediate_data_size,
                           const volatile void* cmd_data);
  error::Error HandleFlushDriverCachesCHROMIUM(uint32_t immediate_data_size,
                                               const volatile void* cmd_data);

  void DoBeginRasterCHROMIUM(GLuint texture_id,
                             GLuint sk_color,
                             GLuint msaa_sample_count,
                             GLboolean can_use_lcd_text,
                             GLboolean use_distance_field_text,
                             GLint pixel_config);
  void DoEndRasterCHROMIUM();

  void DoCopySubTextureCHROMIUM(GLuint source_id,
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
                                GLboolean unpack_unmultiply_alpha);

  void BindTexImage2DCHROMIUMImpl(const char* function_name,
                                  GLenum target,
                                  GLenum internalformat,
                                  GLint image_id);

  void TexStorageImpl(GLenum target,
                      GLsizei levels,
                      GLenum internal_format,
                      GLsizei width,
                      GLsizei height,
                      GLsizei depth,
                      ContextState::Dimension dimension,
                      const char* function_name);

  bool GetCompressedTexSizeInBytes(const char* function_name,
                                   GLsizei width,
                                   GLsizei height,
                                   GLsizei depth,
                                   GLenum format,
                                   GLsizei* size_in_bytes);

  bool ValidateCopyTextureCHROMIUMTextures(const char* function_name,
                                           GLenum dest_target,
                                           TextureRef* source_texture_ref,
                                           TextureRef* dest_texture_ref);
  bool CanUseCopyTextureCHROMIUMInternalFormat(GLenum dest_internal_format);
  bool ValidateCopyTextureCHROMIUMInternalFormats(const char* function_name,
                                                  GLenum source_internal_format,
                                                  GLenum dest_internal_format);
  bool InitializeCopyTextureCHROMIUM(const char* function_name);

  bool DoBindOrCopyTexImageIfNeeded(Texture* texture,
                                    GLenum textarget,
                                    GLuint texture_unit);

  CopyTextureMethod getCopyTextureCHROMIUMMethod(GLenum source_target,
                                                 GLint source_level,
                                                 GLenum source_internal_format,
                                                 GLenum source_type,
                                                 GLenum dest_target,
                                                 GLint dest_level,
                                                 GLenum dest_internal_format,
                                                 bool flip_y,
                                                 bool premultiply_alpha,
                                                 bool unpremultiply_alpha);
  bool ValidateCompressedCopyTextureCHROMIUM(const char* function_name,
                                             TextureRef* source_texture_ref,
                                             TextureRef* dest_texture_ref);

  void DoCopyTexImage(Texture* texture, GLenum textarget, gl::GLImage* image);

  bool InitializeCopyTexImageBlitter(const char* function_name);
  bool ValidateCopyTexFormatHelper(GLenum internal_format,
                                   GLenum read_format,
                                   GLenum read_type,
                                   std::string* output_error_msg);

  void DoReleaseTexImage2DCHROMIUM(GLenum target, GLint image_id);

  scoped_refptr<gl::GLSurface> surface_;
  scoped_refptr<gl::GLContext> context_;

  GLES2DecoderClient* client_;

  // The ContextGroup for this decoder uses to track resources.
  scoped_refptr<ContextGroup> group_;
  const Validators* validators_;
  scoped_refptr<FeatureInfo> feature_info_;

  std::unique_ptr<QueryManager> query_manager_;

  typedef gpu::gles2::GLES2Decoder::Error (RasterDecoder::*CmdHandler)(
      uint32_t immediate_data_size,
      const volatile void* data);

  // A struct to hold info about each command.
  struct CommandInfo {
    CmdHandler cmd_handler;
    uint8_t arg_flags;   // How to handle the arguments for this command
    uint8_t cmd_flags;   // How to handle this command
    uint16_t arg_count;  // How many arguments are expected for this command.
  };

  // A table of CommandInfo for all the commands.
  static CommandInfo command_info[kNumCommands - kFirstGLES2Command];

  // Number of commands remaining to be processed in DoCommands().
  int commands_to_process_;

  // The current decoder error communicates the decoder error through command
  // processing functions that do not return the error value. Should be set
  // only if not returning an error.
  error::Error current_decoder_error_;

  DebugMarkerManager debug_marker_manager_;
  Logger logger_;

  // FIXME(backer): Probably still not necessary.
  // All the state for this context.
  ContextState state_;

  std::unique_ptr<CopyTexImageResourceManager> copy_tex_image_blit_;
  std::unique_ptr<CopyTextureCHROMIUMResourceManager> copy_texture_CHROMIUM_;

  // Raster helpers.
  sk_sp<GrContext> gr_context_;
  sk_sp<SkSurface> sk_surface_;

  bool supports_oop_raster_;

  base::WeakPtrFactory<GLES2Decoder> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RasterDecoder);
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_RASTER_DECODER_H_
