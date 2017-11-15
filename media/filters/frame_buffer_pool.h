// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_FRAME_BUFFER_POOL_H_
#define MEDIA_FILTERS_FRAME_BUFFER_POOL_H_

#include <stdint.h>

#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "base/trace_event/memory_dump_provider.h"

struct vpx_codec_frame_buffer;

namespace base {
class SequencedTaskRunner;
}

namespace media {

// FrameBufferPool is a pool of simple CPU memory, allocated by hand and used by
// both VP9 and any data consumers. This class needs to be ref-counted to hold
// on to allocated memory via the memory-release callback of
// CreateFrameCallback().
// TODO(dalecurtis): Make this generic so it can be used with AV1.
class FrameBufferPool : public base::RefCountedThreadSafe<FrameBufferPool>,
                        public base::trace_event::MemoryDumpProvider {
 public:
  FrameBufferPool();

  // Callback that will be called by libvpx when it needs a frame buffer.
  // Parameters:
  // |user_priv|  Private data passed to libvpx (pointer to memory pool).
  // |min_size|   Minimum size needed by libvpx to decompress the next frame.
  // |fb|         Pointer to the frame buffer to update.
  // Returns 0 on success. Returns < 0 on failure.
  static int32_t GetVP9FrameBuffer(void* user_priv,
                                   size_t min_size,
                                   vpx_codec_frame_buffer* fb);

  // Callback that will be called by libvpx when the frame buffer is no longer
  // being used by libvpx. Can be called with NULL user data when decode stops
  // because of an invalid bitstream.
  // Parameters:
  // |user_priv|  Private data passed to libvpx (pointer to memory pool).
  // |fb|         Pointer to the frame buffer that's being released.
  // Returns 0 on success. Returns < 0 on failure.
  static int32_t ReleaseVP9FrameBuffer(void* user_priv,
                                       vpx_codec_frame_buffer* fb);

  // Generates a "no_longer_needed" closure that holds a reference to this pool.
  base::Closure CreateFrameCallback(void* fb_priv_data);

  // Reference counted frame buffers used for VP9 decoding.
  struct VP9FrameBuffer {
    VP9FrameBuffer();
    ~VP9FrameBuffer();
    std::vector<uint8_t> data;
    std::vector<uint8_t> alpha_data;
    bool held_by_libvpx = false;
    // Needs to be a counter since libvpx may vend a framebuffer multiple times.
    int held_by_frame = 0;
    base::TimeTicks last_use_time;
  };

  size_t get_pool_size_for_testing() const { return frame_buffers_.size(); }

  void set_tick_clock_for_testing(base::TickClock* tick_clock) {
    tick_clock_ = tick_clock;
  }

  void Shutdown();

 private:
  friend class base::RefCountedThreadSafe<FrameBufferPool>;
  ~FrameBufferPool() override;

  // base::MemoryDumpProvider.
  bool OnMemoryDump(const base::trace_event::MemoryDumpArgs& args,
                    base::trace_event::ProcessMemoryDump* pmd) override;

  static bool IsUsed(const VP9FrameBuffer* buf);

  // Drop all entries in |frame_buffers_| that report !IsUsed().
  void EraseUnusedResources();

  // Gets the next available frame buffer for use by libvpx.
  VP9FrameBuffer* GetFreeFrameBuffer(size_t min_size);

  // Method that gets called when a VideoFrame that references this pool gets
  // destroyed.
  void OnVideoFrameDestroyed(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      VP9FrameBuffer* frame_buffer);

  // Frame buffers to be used by libvpx for VP9 Decoding.
  std::vector<std::unique_ptr<VP9FrameBuffer>> frame_buffers_;

  bool in_shutdown_ = false;

  bool registered_dump_provider_ = false;

  // |tick_clock_| is always &|default_tick_clock_| outside of testing.
  base::DefaultTickClock default_tick_clock_;
  base::TickClock* tick_clock_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(FrameBufferPool);
};

}  // namespace media

#endif  // MEDIA_FILTERS_FRAME_BUFFER_POOL_H_
