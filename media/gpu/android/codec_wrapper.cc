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

CodecOutputBuffer::~CodecOutputBuffer() {
  codec_->ReleaseCodecOutputBuffer(id_, false);
}

bool CodecOutputBuffer::HasBeenInvalidated() {
  return codec_->IsCodecOutputBufferInvalid(id_);
}

void CodecOutputBuffer::ReleaseToSurface() {
  codec_->ReleaseCodecOutputBuffer(id_, true);
}

CodecOutputBuffer::CodecOutputBuffer(scoped_refptr<CodecWrapper> codec,
                                     int64_t id)
    : codec_(codec), id_(id) {}

CodecWrapper::CodecWrapper(std::unique_ptr<MediaCodecBridge> codec)
    : codec_(std::move(codec)), in_error_state_(false), next_buffer_id_(0) {}

CodecWrapper::~CodecWrapper() {
  DCHECK(!codec_);
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

bool CodecWrapper::GetOutputSize(gfx::Size* size) {
  base::AutoLock l(lock_);
  DCHECK(codec_ && !in_error_state_);

  auto status = codec_->GetOutputSize(size);
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
  // Assigning to |codec_buffer| may deadlock if it's not null.
  DCHECK(!*codec_buffer);

  int index = -1;
  size_t unused_offset = 0;
  size_t unused_size = 0;
  bool* unused_key_frame = nullptr;
  auto status = codec_->DequeueOutputBuffer(timeout, &index, &unused_offset,
                                            &unused_size, presentation_time,
                                            end_of_stream, unused_key_frame);
  if (status == MEDIA_CODEC_OK) {
    int64_t buffer_id = next_buffer_id_++;
    buffer_ids_[buffer_id] = index;
    *codec_buffer = base::WrapUnique(new CodecOutputBuffer(this, buffer_id));
  } else if (status == MEDIA_CODEC_ERROR) {
    in_error_state_ = true;
  }
  return status;
}

bool CodecWrapper::SetSurface(jobject surface) {
  base::AutoLock l(lock_);
  DCHECK(codec_ && !in_error_state_);

  bool status = codec_->SetSurface(surface);
  if (!status)
    in_error_state_ = true;
  return status;
}

bool CodecWrapper::IsCodecOutputBufferInvalid(int64_t id) {
  base::AutoLock l(lock_);
  return !codec_ || in_error_state_ || !base::ContainsKey(buffer_ids_, id);
}

void CodecWrapper::ReleaseCodecOutputBuffer(int64_t id, bool render) {
  base::AutoLock l(lock_);
  if (!codec_ || in_error_state_)
    return;

  auto it = buffer_ids_.find(id);
  if (it == buffer_ids_.end())
    return;
  int index = it->second;
  buffer_ids_.erase(it);
  codec_->ReleaseOutputBuffer(index, render);
}

}  // namespace media
