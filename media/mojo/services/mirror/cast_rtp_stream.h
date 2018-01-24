// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MIRROR_CAST_RTP_STREAM_H_
#define MEDIA_MOJO_SERVICES_MIRROR_CAST_RTP_STREAM_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "media/cast/cast_config.h"
#include "media/cast/constants.h"

namespace media {

class VideoFrame;
class MirrorService;
class AudioBus;
class AudioParameters;

namespace cast {
class CastSender;
class VideoFrameInput;
}  // namespace cast

class CastAudioSink;

// This class receives MediaStreamTrack events and video frames from a
// MediaStreamVideoTrack.  It also includes a timer to request refresh frames
// when the capturer halts (e.g., a screen capturer stops delivering frames
// because the screen is not being updated).  When a halt is detected, refresh
// frames will be requested at regular intervals for a short period of time.
// This provides the video encoder, downstream, several copies of the last frame
// so that it may clear up lossy encoding artifacts.
//
// Threading: Video frames are received on the IO thread and then
// forwarded to media::cast::VideoFrameInput.  The inner class, Deliverer,
// handles this.  Otherwise, all methods and member variables of the outer class
// must only be accessed on the render thread.
class CastVideoSink {
 public:
  using ErrorCallback = base::Callback<void(const std::string&)>;
  // |track| provides data for this sink.
  // |error_callback| is called if video formats don't match.
  explicit CastVideoSink(const ErrorCallback& error_callback,
                         MirrorService* service);
  ~CastVideoSink();

  // Attach this sink to a video track represented by |track_|.
  // Data received from the track will be submitted to |frame_input|.
  void AddToTrack(
      const scoped_refptr<media::cast::VideoFrameInput>& frame_input);

  class Deliverer : public base::RefCountedThreadSafe<Deliverer> {
   public:
    explicit Deliverer(const ErrorCallback& error_callback);

    void WillConnectToTrack(
        base::WeakPtr<CastVideoSink> sink,
        scoped_refptr<media::cast::VideoFrameInput> frame_input);

    void OnVideoFrame(const scoped_refptr<media::VideoFrame>& video_frame,
                      base::TimeTicks estimated_capture_time);

   private:
    friend class base::RefCountedThreadSafe<Deliverer>;
    ~Deliverer();

    const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
    const ErrorCallback error_callback_;

    // These are set on the main thread after construction, and before the first
    // call to OnVideoFrame() on the IO thread.  |sink_| may be passed around on
    // any thread, but must only be dereferenced on the main renderer thread.
    base::WeakPtr<CastVideoSink> sink_;
    scoped_refptr<media::cast::VideoFrameInput> frame_input_;

    DISALLOW_COPY_AND_ASSIGN(Deliverer);
  };

  scoped_refptr<Deliverer> deliverer() { return deliverer_; }

 private:
  void OnRefreshTimerFired();

  void DidReceiveFrame();

  const scoped_refptr<Deliverer> deliverer_;

  MirrorService* service_;

  // Requests refresh frames at a constant rate while the source is paused, up
  // to a consecutive maximum.
  base::RepeatingTimer refresh_timer_;

  // Counter for the number of consecutive "refresh frames" requested.
  int consecutive_refresh_count_;

  // Set to true when a request for a refresh frame has been made.  This is
  // cleared once the next frame is received.
  bool expecting_a_refresh_frame_;

  base::WeakPtrFactory<CastVideoSink> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CastVideoSink);
};

// This object represents a RTP stream that encodes and optionally
// encrypt audio or video data from a WebMediaStreamTrack.
// Note that this object does not actually output packets. It allows
// configuration of encoding and RTP parameters and control such a logical
// stream.
class CastRtpStream {
 public:
  static bool IsHardwareVP8EncodingSupported();

  static bool IsHardwareH264EncodingSupported();

  // Return parameters currently supported by this stream.
  static std::vector<media::cast::FrameSenderConfig> GetSupportedConfigs(
      bool is_audio,
      bool is_remoting);

  CastRtpStream(bool is_audio,
                cast::CastSender* cast_sender,
                MirrorService* service);
  ~CastRtpStream();

  void Start(const cast::FrameSenderConfig& config);
  void Stop();

  void StartDeliver();
  scoped_refptr<CastVideoSink::Deliverer> GetVideoDeliverer();
  void OnAudioData(const media::AudioBus& input_bus,
                   base::TimeTicks estimated_capture_time);
  void SetAudioParameters(const media::AudioParameters& params);

 private:
  bool is_audio_;
  cast::CastSender* const cast_sender_;  // Outlives this class.
  MirrorService* service_;
  std::unique_ptr<CastVideoSink> video_sink_;
  std::unique_ptr<CastAudioSink> audio_sink_;

  base::WeakPtrFactory<CastRtpStream> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CastRtpStream);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MIRROR_CAST_RTP_STREAM_H_
