// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/frame_buffer_pool.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/stl_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/trace_event/memory_allocator_dump.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/process_memory_dump.h"

// Include libvpx header files.
// VPX_CODEC_DISABLE_COMPAT excludes parts of the libvpx API that provide
// backwards compatibility for legacy applications using the library.
#define VPX_CODEC_DISABLE_COMPAT 1
extern "C" {
#include "third_party/libvpx/source/libvpx/vpx/vpx_frame_buffer.h"
}

namespace media {

FrameBufferPool::VP9FrameBuffer::VP9FrameBuffer() = default;
FrameBufferPool::VP9FrameBuffer::~VP9FrameBuffer() = default;

FrameBufferPool::FrameBufferPool() : tick_clock_(&default_tick_clock_) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

FrameBufferPool::~FrameBufferPool() {
  DCHECK(in_shutdown_);

  // May be destructed on any thread.
}

FrameBufferPool::VP9FrameBuffer* FrameBufferPool::GetFreeFrameBuffer(
    size_t min_size) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!in_shutdown_);

  if (!registered_dump_provider_) {
    base::trace_event::MemoryDumpManager::GetInstance()
        ->RegisterDumpProviderWithSequencedTaskRunner(
            this, "VpxVideoDecoder", base::SequencedTaskRunnerHandle::Get(),
            MemoryDumpProvider::Options());
    registered_dump_provider_ = true;
  }

  // Check if a free frame buffer exists.
  size_t i = 0;
  for (; i < frame_buffers_.size(); ++i) {
    if (!IsUsed(frame_buffers_[i].get()))
      break;
  }

  if (i == frame_buffers_.size()) {
    // Create a new frame buffer.
    frame_buffers_.push_back(base::MakeUnique<VP9FrameBuffer>());
  }

  // Resize the frame buffer if necessary.
  if (frame_buffers_[i]->data.size() < min_size)
    frame_buffers_[i]->data.resize(min_size);
  return frame_buffers_[i].get();
}

int32_t FrameBufferPool::GetVP9FrameBuffer(void* user_priv,
                                           size_t min_size,
                                           vpx_codec_frame_buffer* fb) {
  DCHECK(user_priv);
  DCHECK(fb);

  FrameBufferPool* memory_pool = static_cast<FrameBufferPool*>(user_priv);

  VP9FrameBuffer* fb_to_use = memory_pool->GetFreeFrameBuffer(min_size);
  if (!fb_to_use)
    return -1;

  fb->data = &fb_to_use->data[0];
  fb->size = fb_to_use->data.size();

  DCHECK(!IsUsed(fb_to_use));
  fb_to_use->held_by_libvpx = true;

  // Set the frame buffer's private data to point at the external frame buffer.
  fb->priv = static_cast<void*>(fb_to_use);
  return 0;
}

int32_t FrameBufferPool::ReleaseVP9FrameBuffer(void* user_priv,
                                               vpx_codec_frame_buffer* fb) {
  DCHECK(user_priv);
  DCHECK(fb);

  if (!fb->priv)
    return -1;

  // Note: libvpx may invoke this method multiple times for the same frame, so
  // we can't DCHECK that |held_by_libvpx| is true.
  VP9FrameBuffer* frame_buffer = static_cast<VP9FrameBuffer*>(fb->priv);
  frame_buffer->held_by_libvpx = false;

  if (!IsUsed(frame_buffer)) {
    // TODO(dalecurtis): This should be |tick_clock_| but we don't have access
    // to the main class from this static function and its only needed for tests
    // which all hit the OnVideoFrameDestroyed() path below instead.
    frame_buffer->last_use_time = base::TimeTicks::Now();
  }

  return 0;
}

base::Closure FrameBufferPool::CreateFrameCallback(void* fb_priv_data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  VP9FrameBuffer* frame_buffer = static_cast<VP9FrameBuffer*>(fb_priv_data);
  ++frame_buffer->held_by_frame;

  return base::Bind(&FrameBufferPool::OnVideoFrameDestroyed, this,
                    base::SequencedTaskRunnerHandle::Get(), frame_buffer);
}

bool FrameBufferPool::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::trace_event::MemoryAllocatorDump* memory_dump =
      pmd->CreateAllocatorDump("media/vpx/memory_pool");
  base::trace_event::MemoryAllocatorDump* used_memory_dump =
      pmd->CreateAllocatorDump("media/vpx/memory_pool/used");

  pmd->AddSuballocation(memory_dump->guid(),
                        base::trace_event::MemoryDumpManager::GetInstance()
                            ->system_allocator_pool_name());
  size_t bytes_used = 0;
  size_t bytes_reserved = 0;
  for (const auto& frame_buffer : frame_buffers_) {
    if (IsUsed(frame_buffer.get()))
      bytes_used += frame_buffer->data.size();
    bytes_reserved += frame_buffer->data.size();
  }

  memory_dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                         base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                         bytes_reserved);
  used_memory_dump->AddScalar(
      base::trace_event::MemoryAllocatorDump::kNameSize,
      base::trace_event::MemoryAllocatorDump::kUnitsBytes, bytes_used);

  return true;
}

void FrameBufferPool::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  in_shutdown_ = true;

  if (registered_dump_provider_) {
    base::trace_event::MemoryDumpManager::GetInstance()->UnregisterDumpProvider(
        this);
  }

  // Clear any refs held by libvpx which isn't good about cleaning up after
  // itself. This is safe since libvpx has already been shutdown by this point.
  for (const auto& frame_buffer : frame_buffers_)
    frame_buffer->held_by_libvpx = false;

  EraseUnusedResources();
}

// static
bool FrameBufferPool::IsUsed(const VP9FrameBuffer* buf) {
  return buf->held_by_libvpx || buf->held_by_frame > 0;
}

void FrameBufferPool::EraseUnusedResources() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::EraseIf(frame_buffers_, [](const std::unique_ptr<VP9FrameBuffer>& buf) {
    return !IsUsed(buf.get());
  });
}

void FrameBufferPool::OnVideoFrameDestroyed(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    VP9FrameBuffer* frame_buffer) {
  if (!task_runner->RunsTasksInCurrentSequence()) {
    task_runner->PostTask(
        FROM_HERE, base::Bind(&FrameBufferPool::OnVideoFrameDestroyed, this,
                              task_runner, frame_buffer));
    return;
  }

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_GT(frame_buffer->held_by_frame, 0);
  --frame_buffer->held_by_frame;

  if (in_shutdown_) {
    // If we're in shutdown we can be sure that libvpx has been destroyed.
    EraseUnusedResources();
    return;
  }

  const base::TimeTicks now = tick_clock_->NowTicks();
  if (!IsUsed(frame_buffer))
    frame_buffer->last_use_time = now;

  base::EraseIf(frame_buffers_,
                [now](const std::unique_ptr<VP9FrameBuffer>& buf) {
                  constexpr base::TimeDelta kStaleFrameLimit =
                      base::TimeDelta::FromSeconds(10);
                  return !IsUsed(buf.get()) &&
                         now - buf->last_use_time > kStaleFrameLimit;
                });
}

}  // namespace media
