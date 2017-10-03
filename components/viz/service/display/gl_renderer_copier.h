// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_GL_RENDERER_COPIER_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_GL_RENDERER_COPIER_H_

#include <stdint.h>

#include <array>
#include <forward_list>
#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/unguessable_token.h"
#include "components/viz/common/gl_helper.h"
#include "components/viz/service/viz_service_export.h"

namespace gfx {
class ColorSpace;
class Rect;
class Size;
}  // namespace gfx

namespace viz {

class ContextProvider;
class CopyOutputRequest;
class TextureMailboxDeleter;

// Helper class for GLRenderer that executes CopyOutputRequests using GL, and
// manages the caching of resources needed to ensure efficient video
// performance.
//
// GLRenderer calls CopyFromFramebuffer() to execute a CopyOutputRequest.
// GLRendererCopier will examine the request and determine the minimal amount of
// work needed to satisfy all the requirements of the request.
//
// In many cases, interim GL objects (textures, framebuffers, etc.) must be
// created as part of a multi-step process. When considering video performance
// (i.e., a series of CopyOutputRequests from the same "source"), these interim
// objects must be cached to prevent a significant performance penalty on some
// GPU/drivers. GLRendererCopier manages such a cache and automatically frees
// the objects when it detects that a stream CopyOutputRequests from a given
// "source" has ended.
class VIZ_SERVICE_EXPORT GLRendererCopier {
 public:
  // |texture_mailbox_deleter| must outlive this instance.
  GLRendererCopier(scoped_refptr<ContextProvider> context_provider,
                   TextureMailboxDeleter* texture_mailbox_deleter);

  ~GLRendererCopier();

  // Executes the |request|, copying from the currently-bound framebuffer of the
  // given |internal_format|. The caller MUST set |request->area()|, which is
  // the source rect to copy in the draw space of the RenderPass the request was
  // attached to. |copy_rect| represents the identical source rect, but in
  // window space (see DirectRenderer::MoveFromDrawToWindowSpace()).
  // |framebuffer_texture| and |framebuffer_texture_size| are optional: When
  // non-zero, the texture might be used as the source, to avoid making an extra
  // copy of the framebuffer. |color_space| specifies the color space of the
  // pixels in the framebuffer.
  //
  // This implementation may change texture and framebuffer bindings, and so the
  // caller must not make any assumptions about the original objects still being
  // bound to the same units.
  void CopyFromFramebuffer(std::unique_ptr<CopyOutputRequest> request,
                           const gfx::Rect& copy_rect,
                           GLenum internal_format,
                           GLuint framebuffer_texture,
                           const gfx::Size& framebuffer_texture_size,
                           const gfx::ColorSpace& color_space);

  // Checks whether cached resources should be freed because recent copy
  // activity is no longer using them. This should be called after a frame has
  // finished drawing (after all copy requests have been executed).
  void FreeUnusedCachedResources();

 private:
  friend class GLRendererCopierTest;

  // The collection of resources that might be cached over multiple copy
  // requests from the same source.
  struct CacheEntry {
    base::UnguessableToken request_source;
    uint32_t purge_count_at_last_use;
    std::array<GLuint, 3> object_names;
    std::unique_ptr<GLHelper::ScalerInterface> scaler;

    // Index in |object_names| of the various objects that might be cached.
    static constexpr int kFramebufferCopyTexture = 0;
    static constexpr int kResultTexture = 1;
    static constexpr int kReadbackFramebuffer = 2;

    explicit CacheEntry(const base::UnguessableToken& source);
    ~CacheEntry();
  };

  // Produces a texture containing a copy of the currently-bound framebuffer,
  // and scaled (if requested). The caller owns the returned texture.
  GLuint ProduceResultTexture(const CopyOutputRequest& request,
                              const gfx::Rect& framebuffer_copy_rect,
                              GLenum internal_format,
                              GLuint framebuffer_texture,
                              const gfx::Size& framebuffer_texture_size,
                              const gfx::Rect& result_rect);

  // Processes the next phase of the copy request by starting readback of a
  // |copy_rect| from the given |texture|. This method does NOT take ownership
  // of the |texture|.
  void StartReadbackFromTexture(std::unique_ptr<CopyOutputRequest> request,
                                GLuint texture,
                                const gfx::Rect& copy_rect,
                                const gfx::Rect& result_rect,
                                const gfx::ColorSpace& color_space);

  // Processes the next phase of the copy request by starting readback of a
  // |copy_rect| from the currently-bound framebuffer. This method kicks-off an
  // asynchronous glReadPixels() operation.
  void StartReadbackFromFramebuffer(std::unique_ptr<CopyOutputRequest> request,
                                    const gfx::Rect& copy_rect,
                                    const gfx::Rect& result_rect,
                                    const gfx::ColorSpace& color_space);

  // Callback for the asynchronous glReadPixels(). The pixels are read from the
  // |transfer_buffer|, and a CopyOutputResult is sent to the requestor.
  void OnReadbackFinished(std::unique_ptr<CopyOutputRequest> copy_request,
                          const gfx::Rect& result_rect,
                          const gfx::ColorSpace& color_space,
                          GLuint transfer_buffer,
                          GLuint query);

  // Completes a copy request by packaging-up and sending the given
  // |result_texture| in a TextureMailbox. This method takes ownership of
  // |result_texture|.
  void SendTextureResult(std::unique_ptr<CopyOutputRequest> request,
                         GLuint result_texture,
                         const gfx::Rect& result_rect,
                         const gfx::ColorSpace& color_space);

  // Finds and returns the relevant CacheEntry in |cache_|, or creates a new one
  // on-demand.
  CacheEntry* GetCacheEntry(const base::UnguessableToken& for_source);

  // Returns a GL object name found in the cache, or creates a new one
  // on-demand. The caller takes ownership of the object.
  GLuint TakeCachedObjectOrCreate(const CopyOutputRequest& for_request,
                                  int which);

  // Stashes a GL object name into the cache, or deletes the object if it should
  // not be cached.
  void CacheObjectOrDelete(const base::UnguessableToken& for_source,
                           int which,
                           GLuint object_name);

  // Returns a cached scaler for the given request, or creates one on-demand.
  std::unique_ptr<GLHelper::ScalerInterface> TakeCachedScalerOrCreate(
      const CopyOutputRequest& for_request);

  // Stashes a scaler into the cache, or deletes it if it should not be cached.
  void CacheScalerOrDelete(const CopyOutputRequest& for_request,
                           std::unique_ptr<GLHelper::ScalerInterface> scaler);

  // Frees any objects currently stashed in the given CacheEntry.
  void FreeCachedResources(CacheEntry* entry);

  // Injected dependencies.
  const scoped_refptr<ContextProvider> context_provider_;
  TextureMailboxDeleter* const texture_mailbox_deleter_;

  // Provides comprehensive, quality and efficient scaling and other utilities.
  GLHelper helper_;

  // This increments by one for every call to FreeUnusedCachedResources(). It
  // is meant to determine when cached resources should be freed because they
  // are unlikely to see further use.
  uint32_t purge_counter_;

  // A cache of resources recently used in the execution of a stream of copy
  // requests from the same source. Since this reflects the number of active
  // video captures, it is expected to almost always be zero or one element in
  // size. Thus, std::forward_list is a more-efficient choice than std::vector.
  std::forward_list<CacheEntry> cache_;

  // Cancels callbacks to OnReadbackFinished(). In the case where this
  // GLRendererCopier is deleted, it is assumed that the whole GL context is
  // about to be torn-down, and so any lingering resources allocated for the
  // readback will be freed externally somehow.
  base::WeakPtrFactory<GLRendererCopier> weak_ptr_factory_;

  // Purge cache entries that have not been used after this many calls to
  // FreeUnusedCachedResources(). The choice of 60 is arbitrary, but on most
  // platforms means that a somewhat-to-fully active compositor will cause
  // things to be auto-purged after approx. 1-2 seconds of not being used.
  static constexpr int kKeepalivePeriod = 60;

  DISALLOW_COPY_AND_ASSIGN(GLRendererCopier);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_GL_RENDERER_COPIER_H_
