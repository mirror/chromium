// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "media/gpu/android/media_codec_wrapper.h"

namespace media {

MediaCodecWrapper::MediaCodecWrapper(std::unique_ptr<MediaCodecBridge> codec)
    : codec_(std::move(codec)) {}

MediaCodecWrapper::~MediaCodecWrapper() = default;

bool MediaCodecWrapper::NeedsDrainingBeforeRelease() {
  return !IsEmpty() && config_.codec() == kCodecVP8;
}

bool MediaCodecWrapper::IsEmpty() {
  return num_buffers_in_codec_ == 0;
}

bool MediaCodecWrapper::EosSubmitted() {
  // XXX
  return false;
}

bool MediaCodecWrapper::IsFlushed() {
  // XXX
  return false;
}

bool MediaCodecWrapper::SupportsFlush() {
  // XXX
  return false;
}

void MediaCodecWrapper::Stop() {
  codec_->Stop();
}

MediaCodecStatus MediaCodecWrapper::Flush() {
  return codec_->Flush();
}

MediaCodecStatus MediaCodecWrapper::GetOutputSize(gfx::Size* size) {
  return codec_->GetOutputSize(size);
}

MediaCodecStatus MediaCodecWrapper::GetOutputSamplingRate(int* sampling_rate) {
  return codec_->GetOutputSamplingRate(sampling_rate);
}

MediaCodecStatus MediaCodecWrapper::GetOutputChannelCount(int* channel_count) {
  return codec_->GetOutputChannelCount(channel_count);
}

MediaCodecStatus MediaCodecWrapper::QueueInputBuffer(
    int index,
    const uint8_t* data,
    size_t data_size,
    base::TimeDelta presentation_time) {
  return codec_->QueueInputBuffer(index, data, data_size, presentation_time);
}

MediaCodecStatus MediaCodecWrapper::QueueSecureInputBuffer(
    int index,
    const uint8_t* data,
    size_t data_size,
    const std::string& key_id,
    const std::string& iv,
    const std::vector<SubsampleEntry>& subsamples,
    const EncryptionScheme& encryption_scheme,
    base::TimeDelta presentation_time) {
  return codec_->QueueSecureInputBuffer(index, data, data_size, key_id, iv,
                                        subsamples, encryption_scheme,
                                        presentation_time);
}

void MediaCodecWrapper::QueueEOS(int input_buffer_index) {
  codec_->QueueEOS(input_buffer_index);
}

MediaCodecStatus MediaCodecWrapper::DequeueInputBuffer(base::TimeDelta timeout,
                                                       int* index) {
  return codec_->DequeueInputBuffer(timeout, index);
}

MediaCodecStatus MediaCodecWrapper::DequeueOutputBuffer(
    base::TimeDelta timeout,
    int* index,
    size_t* offset,
    size_t* size,
    base::TimeDelta* presentation_time,
    bool* end_of_stream,
    bool* key_frame) {
  return codec_->DequeueOutputBuffer(timeout, index, offset, size,
                                     presentation_time, end_of_stream,
                                     key_frame);
}

void MediaCodecWrapper::ReleaseOutputBuffer(int index, bool render) {
  codec_->ReleaseOutputBuffer(index, render);
}

MediaCodecStatus MediaCodecWrapper::GetInputBuffer(int input_buffer_index,
                                                   uint8_t** data,
                                                   size_t* capacity) {
  return codec_->GetInputBuffer(input_buffer_index, data, capacity);
}

MediaCodecStatus MediaCodecWrapper::CopyFromOutputBuffer(int index,
                                                         size_t offset,
                                                         void* dst,
                                                         size_t num) {
  return codec_->CopyFromOutputBuffer(index, offset, dst, num);
}

std::string MediaCodecWrapper::GetName() {
  return codec_->GetName();
}

bool MediaCodecWrapper::SetSurface(jobject surface) {
  return codec_->SetSurface(surface);
}

void MediaCodecWrapper::SetVideoBitrate(int bps, int frame_rate) {
  codec_->SetVideoBitrate(bps, frame_rate);
}

void MediaCodecWrapper::RequestKeyFrameSoon() {
  codec_->RequestKeyFrameSoon();
}

bool MediaCodecWrapper::IsAdaptivePlaybackSupported() {
  return codec_->IsAdaptivePlaybackSupported();
}

}  // namespace media
