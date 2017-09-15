// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_PULSE_PULSE_UTIL_H_
#define MEDIA_AUDIO_PULSE_PULSE_UTIL_H_

#include <pulse/pulseaudio.h>

#include <string>

#include "base/macros.h"
#include "base/time/time.h"
#include "media/audio/audio_device_name.h"
#include "media/base/channel_layout.h"

namespace media {

class AudioParameters;

namespace pulse {

// A helper class that acquires pa_threaded_mainloop_lock() while in scope.
class AutoPulseLock {
 public:
  explicit AutoPulseLock(pa_threaded_mainloop* pa_mainloop)
      : pa_mainloop_(pa_mainloop) {
    pa_threaded_mainloop_lock(pa_mainloop_);
  }

  ~AutoPulseLock() { pa_threaded_mainloop_unlock(pa_mainloop_); }

 private:
  pa_threaded_mainloop* pa_mainloop_;
  DISALLOW_COPY_AND_ASSIGN(AutoPulseLock);
};

// A helper class for setting up a pa_threaded_mainloop and pa_context.
class MEDIA_EXPORT Glue {
  Glue();
  ~Glue();

  // Wait for an operation running on |pa_mainloop_| to complete.
  void WaitForOperationCompletion(pa_operation* operation);

  // Both will be null if they could not be initialized.
  pa_threaded_mainloop* pa_mainloop() const { return pa_mainloop_; }
  pa_context* pa_context() const { return pa_context_; }

 private:
  // Initializes |pa_mainloop_| and |pa_context_| using the given context name.
  // Returns false if setup fails. Caller is responsible for calling destroy in
  // the event up setup failure.
  bool Initialize(const char* context_name);

  // Clears context callbacks and frees |pa_context_| and |pa_mainloop_|.
  void Destroy();

  // PulseAudio callback handlers.

  static void OnContextUpdate(pa_context* context, void* mainloop);

  pa_threaded_mainloop* pa_mainloop_ = nullptr;
  pa_context_* pa_context_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(Glue);
};

class Stream {
 public:
  enum class StreamType { INPUT, OUTPUT };
  Stream(StreamType type,
         Glue* glue,
         const AudioParameters& params,
         const std::string& device_id,
         pa_stream_notify_cb_t stream_callback,
         pa_stream_request_cb_t request_callback,
         pa_stream_flags_t flags,
         const pa_sample_spec* sample_spec,
         const pa_buffer_attr* buffer_attr,
         void* user_data);
  ~Stream();
  bool Start();
  bool Stop();

  pa_stream* pa_stream() const { return pa_stream_; }
  bool has_error() const { return has_error_; }

 private:
  // Initializes |pa_stream_|. Returns false if setup fails. Caller is
  // responsible for calling Destroy() in the event up setup failure.
  bool Initialize(StreamType type,
                  const AudioParameters& params,
                  const std::string& device_id,
                  pa_stream_notify_cb_t stream_callback,
                  pa_stream_request_cb_t request_callback,
                  pa_stream_flags_t flags,
                  const pa_sample_spec* sample_spec,
                  const pa_buffer_attr* buffer_attr,
                  void* user_data);

  // Clears callbacks and frees |pa_stream_|.
  void Destroy();

  void StreamSuccessCallback(pa_stream* s, int success, void* mainloop);
  pa_stream* pa_stream_ = nullptr;
  bool has_error_ = false;

  DISALLOW_COPY_AND_ASSIGN(Stream);
};

pa_sample_format_t BitsToPASampleFormat(int bits_per_sample);

pa_channel_map ChannelLayoutToPAChannelMap(ChannelLayout channel_layout);

base::TimeDelta GetHardwareLatency(pa_stream* stream);

}  // namespace pulse

}  // namespace media

#endif  // MEDIA_AUDIO_PULSE_PULSE_UTIL_H_
