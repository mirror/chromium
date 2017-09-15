// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/pulse/pulse_util.h"

#include <stdint.h>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "media/audio/audio_device_description.h"
#include "media/base/audio_parameters.h"
#include "media/base/audio_timestamp_helper.h"

#if defined(DLOPEN_PULSEAUDIO)
#include "media/audio/pulse/pulse_stubs.h"

using media_audio_pulse::kModulePulse;
using media_audio_pulse::InitializeStubs;
using media_audio_pulse::StubPathMap;
#endif  // defined(DLOPEN_PULSEAUDIO)

// Helper macro to avoid code spam and string bloat.
#define RETURN_ON_FAILURE(expression, message) \
  do {                                         \
    if (!(expression)) {                       \
      DLOG(ERROR) << message;                  \
      return false;                            \
    }                                          \
  } while (0)

namespace media {

namespace pulse {

namespace {

#if defined(GOOGLE_CHROME_BUILD)
static const char kBrowserDisplayName[] = "google-chrome";
#else
static const char kBrowserDisplayName[] = "chromium-browser";
#endif

#if defined(DLOPEN_PULSEAUDIO)
static const base::FilePath::CharType kPulseLib[] =
    FILE_PATH_LITERAL("libpulse.so.0");
#endif

pa_channel_position ChromiumToPAChannelPosition(Channels channel) {
  switch (channel) {
    // PulseAudio does not differentiate between left/right and
    // stereo-left/stereo-right, both translate to front-left/front-right.
    case LEFT:
      return PA_CHANNEL_POSITION_FRONT_LEFT;
    case RIGHT:
      return PA_CHANNEL_POSITION_FRONT_RIGHT;
    case CENTER:
      return PA_CHANNEL_POSITION_FRONT_CENTER;
    case LFE:
      return PA_CHANNEL_POSITION_LFE;
    case BACK_LEFT:
      return PA_CHANNEL_POSITION_REAR_LEFT;
    case BACK_RIGHT:
      return PA_CHANNEL_POSITION_REAR_RIGHT;
    case LEFT_OF_CENTER:
      return PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER;
    case RIGHT_OF_CENTER:
      return PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER;
    case BACK_CENTER:
      return PA_CHANNEL_POSITION_REAR_CENTER;
    case SIDE_LEFT:
      return PA_CHANNEL_POSITION_SIDE_LEFT;
    case SIDE_RIGHT:
      return PA_CHANNEL_POSITION_SIDE_RIGHT;
    default:
      NOTREACHED() << "Invalid channel: " << channel;
      return PA_CHANNEL_POSITION_INVALID;
  }
}

class ScopedPropertyList {
 public:
  ScopedPropertyList() : property_list_(pa_proplist_new()) {}
  ~ScopedPropertyList() { pa_proplist_free(property_list_); }

  pa_proplist* get() const { return property_list_; }

 private:
  pa_proplist* property_list_;
  DISALLOW_COPY_AND_ASSIGN(ScopedPropertyList);
};

}  // namespace

Glue::Glue() {
#if defined(DLOPEN_PULSEAUDIO)
  static const bool kLibraryLoaded = []() {
    StubPathMap paths;
    paths[kModulePulse].push_back(kPulseLib);

    const bool success = InitializeStubs(paths);
    if (!success)
      VLOG(1) << "Failed on loading the Pulse library and symbols";
    return success;
  };

  if (!kLibraryLoaded)
    return;
#endif  // defined(DLOPEN_PULSEAUDIO)

  const std::string& app_name = AudioManager::GetGlobalAppName();
  if (!Initialize(app_name.empty() ? "Chromium" : app_name.c_str()))
    Destroy();
}

void Glue::WaitForOperationCompletion(pa_operation* operation) {
  DCHECK(pa_mainloop_);
  RETURN_ON_FAILURE(operation, "Operation is NULL");

  while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
    pa_threaded_mainloop_wait(pa_mainloop_);

  pa_operation_unref(operation);
}

bool Glue::Initialize(const char* context_name) {
  // The setup order below follows the pattern used by pa_simple_new():
  // https://github.com/pulseaudio/pulseaudio/blob/master/src/pulse/simple.c

  // Create a mainloop API and connect to the default server.
  // The mainloop is the internal asynchronous API event loop.
  pa_mainloop_ = pa_threaded_mainloop_new();
  RETURN_ON_FAILURE(pa_mainloop_, "Failed to create PulseAudio main loop.");

  pa_context_ =
      pa_context_new(pa_threaded_mainloop_get_api(pa_mainloop_), context_name);
  RETURN_ON_FAILURE(pa_context_, "Failed to create PulseAudio context.");

  pa_context_set_state_callback(pa_context_, &OnContextUpdate, pa_mainloop_);
  RETURN_ON_FAILURE(pa_context_connect(pa_context_, nullptr,
                                       PA_CONTEXT_NOAUTOSPAWN, nullptr) == 0,
                    "Failed to connect PulseAudio context.");

  // Lock the event loop object, effectively blocking the event loop thread
  // from processing events. This is necessary.
  AutoPulseLock auto_lock(pa_mainloop_);

  // Start the threaded mainloop after everything has been configured.
  RETURN_ON_FAILURE(pa_threaded_mainloop_start(pa_mainloop_) == 0,
                    "Failed to start PulseAudio main loop.");

  // Wait until |pa_context| is ready.  pa_threaded_mainloop_wait() must be
  // called after pa_context_get_state() in case the context is already ready,
  // otherwise pa_threaded_mainloop_wait() will hang indefinitely.
  while (true) {
    pa_context_state_t context_state = pa_context_get_state(pa_context);
    RETURN_ON_FAILURE(PA_CONTEXT_IS_GOOD(context_state),
                      "Invalid PulseAudio context state.");
    if (context_state == PA_CONTEXT_READY)
      break;
    pa_threaded_mainloop_wait(pa_mainloop);
  }

  return true;
}

void Glue::Destroy() {
  if (pa_context_) {
    AutoPulseLock auto_lock(pa_mainloop_);
    pa_context_set_state_callback(pa_context_, nullptr, nullptr);
    pa_context_disconnect(pa_context_);
    pa_context_unref(pa_context_);
    pa_context_ = nullptr;
  }

  if (pa_mainloop_) {
    pa_threaded_mainloop_stop(pa_mainloop_);
    pa_threaded_mainloop_free(pa_mainloop_);
    pa_mainloop_ = nullptr;
  }
}

void Glue::DestroyStream(pa_stream* pa_stream) {}

void Glue::OnContextUpdate(pa_context* context, void* mainloop) {
  pa_threaded_mainloop* pa_mainloop =
      static_cast<pa_threaded_mainloop*>(mainloop);
  pa_threaded_mainloop_signal(pa_mainloop, 0);
}

void Glue::StreamSuccessCallback(pa_stream* s, int success, void* mainloop) {
  stream_success_ = !!success;
  pa_threaded_mainloop_signal(static_cast<pa_threaded_mainloop*>(mainloop), 0);
}

Stream::Stream(StreamType type,
               Glue* glue,
               const AudioParameters& params,
               const std::string& device_id,
               pa_stream_notify_cb_t stream_callback,
               pa_stream_request_cb_t request_callback,
               pa_stream_flags_t flags,
               const pa_sample_spec* sample_spec,
               const pa_buffer_attr* buffer_attr,
               void* user_data)
    : glue_(glue) {
  if (!Initialize(type, params, device_id, stream_callback, request_callback,
                  flags, sample_spec, buffer_attr, user_data)) {
    Destroy();
  }
}

Stream::~Stream() {
  Destroy();
}

bool Stream::Initialize(StreamType type,
                        const AudioParameters& params,
                        const std::string& device_id,
                        pa_stream_notify_cb_t stream_callback,
                        pa_stream_request_cb_t request_callback,
                        pa_stream_flags_t flags,
                        const pa_sample_spec* sample_spec,
                        const pa_buffer_attr* buffer_attr,
                        void* user_data) {
  DCHECK(glue_->pa_mainloop());
  DCHECK(glue_->pa_context());

  // Lock the main loop while setting up the stream.  Failure to do so may lead
  // to crashes as the PulseAudio thread tries to run before things are ready.
  AutoPulseLock auto_lock(glue_->pa_mainloop());

  // Get channel mapping; attempt to use preferred layout if supported.
  pa_channel_map source_channel_map =
      ChannelLayoutToPAChannelMap(params.channel_layout());
  pa_channel_map* map =
      source_channel_map.channels ? &source_channel_map : nullptr;

  // Open stream and tell PulseAudio what the stream icon should be.
  ScopedPropertyList property_list;
  pa_proplist_sets(property_list.get(), PA_PROP_APPLICATION_ICON_NAME,
                   kBrowserDisplayName);
  pa_stream_ = pa_stream_new_with_proplist(
      glue_->pa_context(),
      type == StreamType::OUTPUT ? "Playback" : "RecordStream", sample_spec,
      map, property_list.get());
  RETURN_ON_FAILURE(*pa_stream, "pa_stream_new_with_proplist failed.");

  pa_stream_set_state_callback(pa_stream_, stream_callback, user_data);

  const char* pa_device_id =
      device_id == AudioDeviceDescription::kDefaultDeviceId ? nullptr
                                                            : device_id.c_str();

  if (type == StreamType::OUTPUT) {
    // Even though we start the stream corked below, PulseAudio will issue one
    // stream request after setup.  write_callback() must fulfill the write.
    pa_stream_set_write_callback(pa_stream_, request_callback, user_data);

    // Connect playback stream.  Like pa_buffer_attr, the pa_stream_flags have a
    // huge impact on the performance of the stream and were chosen through
    // trial and error.
    RETURN_ON_FAILURE(
        pa_stream_connect_playback(pa_stream_ pa_device_id, buffer_attr, flags,
                                   nullptr, nullptr) == 0,
        "pa_stream_connect_playback failed.");
  } else {
    pa_stream_set_read_callback(pa_stream_, request_callback, user_data);
    pa_stream_readable_size(pa_stream_);

    RETURN_ON_FAILURE(pa_stream_connect_record(pa_stream_, pa_device_id,
                                               buffer_attr, flags) == 0,
                      "pa_stream_connect_record failed.");
  }

  // Wait for the stream to be ready.
  while (true) {
    pa_stream_state_t stream_state = pa_stream_get_state(pa_stream_);
    RETURN_ON_FAILURE(PA_STREAM_IS_GOOD(stream_state),
                      "Invalid PulseAudio stream state");
    if (stream_state == PA_STREAM_READY)
      break;
    pa_threaded_mainloop_wait(glue_->pa_mainloop());
  }

  return true;
}

void Stream::Destroy() {
  if (!pa_stream)
    return;

  AutoPulseLock auto_lock(glue_->pa_mainloop());

  // Disable all the callbacks before disconnecting.
  pa_stream_set_state_callback(pa_stream_, nullptr, nullptr);

  glue_->WaitForOperationCompletion(pa_stream_flush(
      pa_stream_, &StreamSuccessCallback, glue_->pa_mainloop()));

  if (pa_stream_get_state(pa_stream_) != PA_STREAM_UNCONNECTED)
    pa_stream_disconnect(pa_stream_);

  pa_stream_set_write_callback(pa_stream_, nullptr, nullptr);
  pa_stream_set_state_callback(pa_stream_, nullptr, nullptr);
  pa_stream_unref(pa_stream);
  pa_stream_ = nullptr;
}

bool Stream::Start() {
  AutoPulseLock auto_lock(glue_->pa_mainloop());

  // Ensure the context and stream are ready.
  if (pa_context_get_state(glue_->pa_context()) != PA_CONTEXT_READY &&
      pa_stream_get_state(pa_stream_) != PA_STREAM_READY) {
    has_error_ = true;
    return false;
  }

  WaitForOperationCompletion(pa_stream_cork(
      pa_stream_, 0, &StreamSuccessCallback, glue_->pa_mainloop()));
  return !has_error_;
}

bool Stream::Stop() {
  AutoPulseLock auto_lock(glue_->pa_mainloop());

  if (!has_error_) {
    // Flush the stream prior to cork, doing so after will cause hangs.  Request
    // callbacks are suspended while inside pa_threaded_mainloop_lock() so this
    // is all thread safe.
    glue_->WaitForOperationCompletion(pa_stream_flush(
        pa_stream_, &StreamSuccessCallback, glue_->pa_mainloop()));
  }

  glue_->WaitForOperationCompletion(pa_stream_cork(
      pa_stream_, 1, &StreamSuccessCallback, glue_->pa_mainloop()));
  return !has_error_;
}

pa_sample_format_t BitsToPASampleFormat(int bits_per_sample) {
  switch (bits_per_sample) {
    case 8:
      return PA_SAMPLE_U8;
    case 16:
      return PA_SAMPLE_S16LE;
    case 24:
      return PA_SAMPLE_S24LE;
    case 32:
      return PA_SAMPLE_S32LE;
    default:
      NOTREACHED() << "Invalid bits per sample: " << bits_per_sample;
      return PA_SAMPLE_INVALID;
  }
}

pa_channel_map ChannelLayoutToPAChannelMap(ChannelLayout channel_layout) {
  pa_channel_map channel_map;
  if (channel_layout == CHANNEL_LAYOUT_MONO) {
    // CHANNEL_LAYOUT_MONO only specifies audio on the C channel, but we
    // want PulseAudio to play single-channel audio on more than just that.
    pa_channel_map_init_mono(&channel_map);
  } else {
    pa_channel_map_init(&channel_map);

    channel_map.channels = ChannelLayoutToChannelCount(channel_layout);
    for (Channels ch = LEFT; ch <= CHANNELS_MAX;
         ch = static_cast<Channels>(ch + 1)) {
      int channel_index = ChannelOrder(channel_layout, ch);
      if (channel_index < 0)
        continue;

      channel_map.map[channel_index] = ChromiumToPAChannelPosition(ch);
    }
  }

  return channel_map;
}

base::TimeDelta GetHardwareLatency(pa_stream* stream) {
  DCHECK(stream);
  int negative = 0;
  pa_usec_t latency_micros = 0;
  if (pa_stream_get_latency(stream, &latency_micros, &negative) != 0)
    return base::TimeDelta();

  if (negative)
    return base::TimeDelta();

  return base::TimeDelta::FromMicroseconds(latency_micros);
}

}  // namespace pulse

}  // namespace media
