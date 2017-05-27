// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_CODEC_WRAPPER_H_
#define MEDIA_GPU_ANDROID_CODEC_WRAPPER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "media/base/android/media_codec_bridge.h"
#include "media/gpu/media_gpu_export.h"
#include "media/gpu/surface_texture_gl_owner.h"

namespace media {
class CodecWrapper;

// A MediaCodec output buffer that can be released on any thread, and
// lets you query whether it has been invalidated.
class MEDIA_GPU_EXPORT CodecBuffer {
 public:
  // Releases the buffer without rendering it.
  ~CodecBuffer();

  // Whether this buffer is still valid, i.e., it has not been released yet and
  // its backing codec has no been released or flushed.
  bool IsValid();

  // Releases this buffer to the codec and renders it to the surface.
  void ReleaseToSurface();

 private:
  // Only CodecWrapper can construct these.
  friend class CodecWrapper;
  CodecBuffer(scoped_refptr<CodecWrapper> codec, int64_t id);

  scoped_refptr<CodecWrapper> codec_;
  int64_t id_;
  bool released_;
};

// This wraps a MediaCodecBridge for MediaCodecVideoDecoder. It's similar to the
// MediaCodecBridge interface, but it's pared down to only what we need plus:
// * It returns CodecBuffers from DequeueOutputBuffer() which can be safely
//   rendered on any thread, and that will release their buffers on
//   destruction. This lets us decode on one thread while rendering on another.
// * It maintains codec specific state like whether an error has occurred.
//
// CodecWrapper is not generally threadsafe; it should only be used on a
// single thread but its returned CodecBuffers can released on any thread.
class MEDIA_GPU_EXPORT CodecWrapper
    : public base::RefCountedThreadSafe<CodecWrapper> {
 public:
  CodecWrapper(std::unique_ptr<MediaCodecBridge> codec);
  std::unique_ptr<MediaCodecBridge> TakeCodec();

  // Whether the codec supports Flush().
  bool SupportsFlush();

  // Whether there are any valid CodecBuffers that have no been released.
  bool HasValidCodecBuffers();

  // Like MediaCodecBridge::DequeueOutputBuffer() but it outputs a CodecBuffer
  // instead of an index. |*codec_buffer| must be null.
  MediaCodecStatus DequeueOutputBuffer(
      base::TimeDelta timeout,
      base::TimeDelta* presentation_time,
      bool* end_of_stream,
      std::unique_ptr<CodecBuffer>* codec_buffer);

  // See MediaCodecBridge for documentation on the following.
  bool Flush();
  bool GetOutputSize(gfx::Size* size);
  MediaCodecStatus QueueInputBuffer(int index,
                                    const uint8_t* data,
                                    size_t data_size,
                                    base::TimeDelta presentation_time);
  MediaCodecStatus QueueSecureInputBuffer(
      int index,
      const uint8_t* data,
      size_t data_size,
      const std::string& key_id,
      const std::string& iv,
      const std::vector<SubsampleEntry>& subsamples,
      const EncryptionScheme& encryption_scheme,
      base::TimeDelta presentation_time);
  void QueueEOS(int input_buffer_index);
  MediaCodecStatus DequeueInputBuffer(base::TimeDelta timeout, int* index);
  bool SetSurface(jobject surface);

 private:
  friend class CodecBuffer;
  friend class base::RefCountedThreadSafe<CodecWrapper>;
  ~CodecWrapper();

  // Releases dequeued codec buffers back to the codec and invalidates them.
  void ReleaseAndInvalidateCodecBuffers();

  // Whether the codec buffer id is still valid, i.e., the underlying codec is
  // still valid and |id| refers to one of the currently dequeued buffers.
  // Can be called on any thread.
  bool IsCodecBufferValid(int64_t id);

  // Releases the codec buffer and optionally renders it. This is a noop if
  // the codec buffer is not valid (i.e., there's no race between checking its
  // validity and releasing it). Can be called on any thread.
  void ReleaseCodecBuffer(int64_t id, bool render);

  // |lock_| protects |codec_| and |in_error_state_|.
  base::Lock lock_;
  std::unique_ptr<MediaCodecBridge> codec_;
  bool in_error_state_;

  // Buffer ids are unique for a given CodecWrapper and map to MediaCodec buffer
  // indices.
  int64_t next_buffer_id_;
  base::flat_map<int64_t, int> buffer_ids_;

  DISALLOW_COPY_AND_ASSIGN(CodecWrapper);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_CODEC_WRAPPER_H_
