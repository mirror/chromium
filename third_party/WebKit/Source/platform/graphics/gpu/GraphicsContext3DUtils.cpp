// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "GraphicsContext3DUtils.h"

#include "gpu/command_buffer/client/gles2_interface.h"
#include "platform/graphics/WebGraphicsContext3DProviderWrapper.h"
#include "skia/ext/texture_handle.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/gpu/GrContext.h"

namespace {

struct GrTextureMailboxReleaseProcData {
  GrTexture* gr_texture_;
  WeakPtr<blink::WebGraphicsContext3DProviderWrapper> context_provider_wrapper_;
};

void GrTextureMailboxReleaseProc(void* data) {
  GrTextureMailboxReleaseProcData* release_proc_data =
      static_cast<GrTextureMailboxReleaseProcData*>(data);

  if (release_proc_data->context_provider_wrapper_) {
    release_proc_data->context_provider_wrapper_->Utils()->RemoveCachedMailbox(
        release_proc_data->gr_texture_);
  }

  delete release_proc_data;
}

}  // unnamed namespace

namespace blink {

void GraphicsContext3DUtils::GetMailboxForSkImage(gpu::Mailbox& out_mailbox,
                                                  gpu::SyncToken& sync_token,
                                                  const sk_sp<SkImage>& image) {
  // This object is owned by context_provider_wrapper_, so that weak ref
  // should never be null.
  DCHECK(context_provider_wrapper_);
  DCHECK(image->isTextureBacked());
  GrContext* gr = context_provider_wrapper_->ContextProvider()->GetGrContext();
  gpu::gles2::GLES2Interface* gl =
      context_provider_wrapper_->ContextProvider()->ContextGL();

  DCHECK(gr);
  DCHECK(gl);
  GrTexture* gr_texture = image->getTexture();
  DCHECK(gr == gr_texture->getContext());
  auto it = cached_mailboxes_.find(gr_texture);
  if (it != cached_mailboxes_.end()) {
    out_mailbox = it->value;
  } else {
    GLuint texture_id =
        skia::GrBackendObjectToGrGLTextureInfo(image->getTextureHandle(true))
            ->fID;
    gl->BindTexture(GL_TEXTURE_2D, texture_id);

    gl->GenMailboxCHROMIUM(out_mailbox.name);
    gl->ProduceTextureCHROMIUM(GL_TEXTURE_2D, out_mailbox.name);

    GrTextureMailboxReleaseProcData* release_proc_data =
        new GrTextureMailboxReleaseProcData();
    release_proc_data->gr_texture_ = gr_texture;
    release_proc_data->context_provider_wrapper_ = context_provider_wrapper_;
    gr_texture->setRelease(GrTextureMailboxReleaseProc, release_proc_data);
    cached_mailboxes_.insert(gr_texture, out_mailbox);
  }

  const GLuint64 fence_sync = gl->InsertFenceSyncCHROMIUM();
  gl->ShallowFlushCHROMIUM();
  gl->GenSyncTokenCHROMIUM(fence_sync, sync_token.GetData());

  gl->BindTexture(GL_TEXTURE_2D, 0);
  // We changed bound textures in this function, so reset the GrContext.
  gr->resetContext(kTextureBinding_GrGLBackendState);
}

void GraphicsContext3DUtils::RemoveCachedMailbox(GrTexture* gr_texture) {
  cached_mailboxes_.erase(gr_texture);
}

}  // namespace blink
