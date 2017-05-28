// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/codec_wrapper.h"

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "media/base/android/media_codec_util.h"

namespace media {

// CodecWrapperRef contains a lock and a pointer to a CodecWrapper. If
// is_valid() the pointer may be used while the lock is held.. Otherwise, the
// CodecWrapper it was pointing to was destructed.
struct CodecWrapperRef : public base::RefCountedThreadSafe<CodecWrapperRef> {
  CodecWrapperRef(CodecWrapper* codec) : codec(codec) {}

  bool is_valid() { return codec != nullptr; }

  base::Lock lock;
  CodecWrapper* codec;

 private:
  friend class RefCountedThreadSafe<CodecWrapperRef>;
  ~CodecWrapperRef() = default;
};

CodecOutputBuffer::CodecOutputBuffer(scoped_refptr<CodecWrapperRef> codec_ref,
                                     int64_t id,
                                     gfx::Size size)
    : codec_ref_(std::move(codec_ref)), id_(id), size_(size) {}

CodecOutputBuffer::~CodecOutputBuffer() {
  base::AutoLock l(codec_ref_->lock);
  if (codec_ref_->is_valid())
    codec_ref_->codec->ReleaseCodecOutputBuffer(id_, false);
}

bool CodecOutputBuffer::ReleaseToSurface() {
  base::AutoLock l(codec_ref_->lock);
  if (!codec_ref_->is_valid())
    return false;
  return codec_ref_->codec->ReleaseCodecOutputBuffer(id_, true);
}

CodecWrapper::CodecWrapper(std::unique_ptr<MediaCodecBridge> codec)
    : codec_(std::move(codec)),
      in_error_state_(false),
      next_buffer_id_(0),
      this_ref_(new CodecWrapperRef(this)) {}

CodecWrapper::~CodecWrapper() {
  DCHECK(!codec_);
  // Invalidate the reference held by output buffers.
  base::AutoLock l(this_ref_->lock);
  this_ref_->codec = nullptr;
}

std::unique_ptr<MediaCodecBridge> CodecWrapper::TakeCodec() {
  base::AutoLock l(lock_);
  DiscardCodecOutputBuffers_Locked();
  return std::move(codec_);
}

bool CodecWrapper::HasValidCodecOutputBuffers() const {
  base::AutoLock l(lock_);
  return !buffer_ids_.empty();
}

void CodecWrapper::DiscardCodecOutputBuffers() {
  base::AutoLock l(lock_);
  DiscardCodecOutputBuffers_Locked();
}

void CodecWrapper::DiscardCodecOutputBuffers_Locked() {
  lock_.AssertAcquired();
  for (auto& kv : buffer_ids_)
    codec_->ReleaseOutputBuffer(kv.second, false);
  buffer_ids_.clear();
}

bool CodecWrapper::SupportsFlush() const {
  base::AutoLock l(lock_);
  return !MediaCodecUtil::CodecNeedsFlushWorkaround(codec_.get());
}

bool CodecWrapper::Flush() {
  base::AutoLock l(lock_);
  DCHECK(codec_ && !in_error_state_);

  // Dequeued output buffers are invalidated by flushing.
  buffer_ids_.clear();
  auto status = codec_->Flush();
  if (status == MEDIA_CODEC_ERROR) {
    in_error_state_ = true;
    return false;
  }
  return true;
}

MediaCodecStatus CodecWrapper::QueueInputBuffer(
    int index,
    const uint8_t* data,
    size_t data_size,
    base::TimeDelta presentation_time) {
  base::AutoLock l(lock_);
  DCHECK(codec_ && !in_error_state_);

  auto status =
      codec_->QueueInputBuffer(index, data, data_size, presentation_time);
  if (status == MEDIA_CODEC_ERROR)
    in_error_state_ = true;
  return status;
}

MediaCodecStatus CodecWrapper::QueueSecureInputBuffer(
    int index,
    const uint8_t* data,
    size_t data_size,
    const std::string& key_id,
    const std::string& iv,
    const std::vector<SubsampleEntry>& subsamples,
    const EncryptionScheme& encryption_scheme,
    base::TimeDelta presentation_time) {
  base::AutoLock l(lock_);
  DCHECK(codec_ && !in_error_state_);

  auto status = codec_->QueueSecureInputBuffer(
      index, data, data_size, key_id, iv, subsamples, encryption_scheme,
      presentation_time);
  if (status == MEDIA_CODEC_ERROR)
    in_error_state_ = true;
  return status;
}

void CodecWrapper::QueueEOS(int input_buffer_index) {
  base::AutoLock l(lock_);
  DCHECK(codec_ && !in_error_state_);
  codec_->QueueEOS(input_buffer_index);
}

MediaCodecStatus CodecWrapper::DequeueInputBuffer(base::TimeDelta timeout,
                                                  int* index) {
  base::AutoLock l(lock_);
  DCHECK(codec_ && !in_error_state_);
  auto status = codec_->DequeueInputBuffer(timeout, index);
  if (status == MEDIA_CODEC_ERROR)
    in_error_state_ = true;
  return status;
}

MediaCodecStatus CodecWrapper::DequeueOutputBuffer(
    base::TimeDelta timeout,
    base::TimeDelta* presentation_time,
    bool* end_of_stream,
    std::unique_ptr<CodecOutputBuffer>* codec_buffer) {
  base::AutoLock l(lock_);
  DCHECK(codec_ && !in_error_state_);
  // If |*codec_buffer| were not null, deleting it may deadlock when it
  // tries to release itself.
  DCHECK(!*codec_buffer);

  // Dequeue in a loop so we can avoid propagating the uninteresting
  // OUTPUT_FORMAT_CHANGED and OUTPUT_BUFFERS_CHANGED statuses to our caller.
  for (int attempt = 0; attempt < 3; ++attempt) {
    int index = -1;
    size_t unused_offset = 0;
    size_t unused_size = 0;
    bool* unused_key_frame = nullptr;
    auto status = codec_->DequeueOutputBuffer(timeout, &index, &unused_offset,
                                              &unused_size, presentation_time,
                                              end_of_stream, unused_key_frame);
    switch (status) {
      case MEDIA_CODEC_OK: {
        int64_t buffer_id = next_buffer_id_++;
        buffer_ids_[buffer_id] = index;
        *codec_buffer = base::WrapUnique(
            new CodecOutputBuffer(this_ref_, buffer_id, size_));
        return status;
      }
      case MEDIA_CODEC_ERROR: {
        in_error_state_ = true;
        return status;
      }
      case MEDIA_CODEC_OUTPUT_FORMAT_CHANGED: {
        // An OUTPUT_FORMAT_CHANGED is not reported after Flush() if the frame
        // size does not change.
        if (codec_->GetOutputSize(&size_) == MEDIA_CODEC_ERROR) {
          in_error_state_ = true;
          return MEDIA_CODEC_ERROR;
        }
        continue;
      }
      case MEDIA_CODEC_OUTPUT_BUFFERS_CHANGED:
        continue;
      default:
        return status;
    }
  }

  in_error_state_ = true;
  return MEDIA_CODEC_ERROR;
}

bool CodecWrapper::SetSurface(jobject surface) {
  base::AutoLock l(lock_);
  DCHECK(codec_ && !in_error_state_);

  bool status = codec_->SetSurface(surface);
  if (!status)
    in_error_state_ = true;
  return status;
}

bool CodecWrapper::ReleaseCodecOutputBuffer(int64_t id, bool render) {
  base::AutoLock l(lock_);
  if (!codec_ || in_error_state_)
    return false;

  auto it = buffer_ids_.find(id);
  if (it == buffer_ids_.end())
    return false;
  int index = it->second;
  buffer_ids_.erase(it);
  codec_->ReleaseOutputBuffer(index, render);
  return true;
}

}  // namespace media
