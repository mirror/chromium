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
class MEDIA_GPU_EXPORT CodecOutputBuffer {
 public:
  // Releases the buffer without rendering it.
  ~CodecOutputBuffer();

  // Whether this buffer has been invalidated, i.e., it has been released
  // or its backing codec was released or flushed.
  bool HasBeenInvalidated();

  // Releases this buffer to the codec and renders it to the surface.
  void ReleaseToSurface();

 private:
  // Let CodecWrapper call the constructor.
  friend class CodecWrapper;
  CodecOutputBuffer(scoped_refptr<CodecWrapper> codec, int64_t id);

  scoped_refptr<CodecWrapper> codec_;
  int64_t id_;
  DISALLOW_COPY_AND_ASSIGN(CodecOutputBuffer);
};

// This wraps a MediaCodecBridge and provides a pared down version of its
// interface. It also adds the following features:
// * It outputs CodecOutputBuffers from DequeueOutputBuffer() which can be
//   safely rendered on any thread, and that will release their buffers on
//   destruction. This lets us decode on one thread while rendering on another.
// * It maintains codec specific state like whether an error has occurred.
//
// CodecWrapper is threadsafe, but it's invalid to call most of its member
// functions after TakeCodec() is called.
class MEDIA_GPU_EXPORT CodecWrapper
    : public base::RefCountedThreadSafe<CodecWrapper> {
 public:
  // |codec| should be in the state referred to by the MediaCodec docs as
  // "Flushed", i.e., freshly configured or after a Flush() call.
  CodecWrapper(std::unique_ptr<MediaCodecBridge> codec);

  // Takes the backing codec and discards all outstanding codec buffers. This
  // lets you tear down the codec while there are still CodecOutputBuffers
  // referencing |this|.
  std::unique_ptr<MediaCodecBridge> TakeCodec();

  // Whether there are any valid CodecOutputBuffers that have not been released.
  bool HasValidCodecOutputBuffers() const;

  // Releases currently dequeued codec buffers back to the codec without
  // rendering.
  void DiscardCodecOutputBuffers();

  // Whether the codec supports Flush().
  bool SupportsFlush() const;

  // See MediaCodecBridge documentation for the following.
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

  // Like MediaCodecBridge::DequeueOutputBuffer() but it outputs a
  // CodecOutputBuffer instead of an index. |*codec_buffer| must be null.
  MediaCodecStatus DequeueOutputBuffer(
      base::TimeDelta timeout,
      base::TimeDelta* presentation_time,
      bool* end_of_stream,
      std::unique_ptr<CodecOutputBuffer>* codec_buffer);
  bool SetSurface(jobject surface);

 private:
  friend class CodecOutputBuffer;
  friend class base::RefCountedThreadSafe<CodecWrapper>;
  ~CodecWrapper();

  void DiscardCodecOutputBuffers_Locked();

  // Whether the codec buffer id has been invalidated, i.e., the underlying
  // codec was taken or |id| does not refer to one of the currently dequeued
  // buffers. Can be called on any thread.
  bool IsCodecOutputBufferInvalid(int64_t id);

  // Releases the codec buffer and optionally renders it. This is a noop if
  // the codec buffer is not valid (i.e., there's no race between checking its
  // validity and releasing it). Can be called on any thread.
  void ReleaseCodecOutputBuffer(int64_t id, bool render);

  // |lock_| protects access to all member variables.
  mutable base::Lock lock_;
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
