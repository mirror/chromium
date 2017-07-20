// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/output/texture_mailbox_deleter.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "cc/resources/release_callback.h"
#include "components/viz/common/gpu/context_provider.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/sync_token.h"

namespace cc {

static void DeleteTextureOnImplThread(
    const scoped_refptr<viz::ContextProvider>& context_provider,
    unsigned texture_id,
    const gpu::SyncToken& sync_token,
    bool is_lost) {
  if (sync_token.HasData()) {
    context_provider->ContextGL()->WaitSyncTokenCHROMIUM(
        sync_token.GetConstData());
  }
  context_provider->ContextGL()->DeleteTextures(1, &texture_id);
}

static void PostTaskFromMainToImplThread(
    scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner,
    SingleReleaseCallback run_impl_callback,
    const gpu::SyncToken& sync_token,
    bool is_lost) {
  // This posts the task to RunDeleteTextureOnImplThread().
  impl_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(run_impl_callback), sync_token, is_lost));
}

TextureMailboxDeleter::TextureMailboxDeleter(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : impl_task_runner_(std::move(task_runner)), weak_ptr_factory_(this) {}

TextureMailboxDeleter::~TextureMailboxDeleter() {
  auto impl_callbacks = std::move(impl_callbacks_);
  for (auto& item : impl_callbacks)
    std::move(item.second).Run(gpu::SyncToken(), true);
}

SingleReleaseCallback TextureMailboxDeleter::GetReleaseCallback(
    scoped_refptr<viz::ContextProvider> context_provider,
    unsigned texture_id) {
  // This callback owns the |context_provider|. It must be destroyed on the impl
  // thread. Upon destruction of this class, the callback must immediately be
  // destroyed.
  SingleReleaseCallback impl_callback = base::BindOnce(
      &DeleteTextureOnImplThread, std::move(context_provider), texture_id);

  int32_t id = next_id_++;
  impl_callbacks_.insert(std::make_pair(id, std::move(impl_callback)));

  SingleReleaseCallback run_impl_callback =
      base::BindOnce(&TextureMailboxDeleter::RunDeleteTextureOnImplThread,
                     weak_ptr_factory_.GetWeakPtr(), id);

  // Provide a callback for the main thread that posts back to the impl
  // thread.
  if (!impl_task_runner_)
    return run_impl_callback;
  return base::BindOnce(&PostTaskFromMainToImplThread, impl_task_runner_,
                        std::move(run_impl_callback));
}

void TextureMailboxDeleter::RunDeleteTextureOnImplThread(
    int32_t id,
    const gpu::SyncToken& sync_token,
    bool is_lost) {
  auto found = impl_callbacks_.find(id);
  auto impl_callback = std::move(found->second);
  impl_callbacks_.erase(found);
  std::move(impl_callback).Run(sync_token, is_lost);
}

}  // namespace cc
