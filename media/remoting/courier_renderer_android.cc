// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/remoting/courier_renderer.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/numerics/safe_math.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/buffering_state.h"
#include "media/base/media_resource.h"
#include "media/base/renderer_client.h"
#include "media/base/video_renderer_sink.h"
#include "media/remoting/demuxer_stream_adapter.h"
#include "media/remoting/proto_enum_utils.h"
#include "media/remoting/proto_utils.h"
#include "media/remoting/renderer_controller.h"

namespace media {
namespace remoting {

namespace {

// The moving time window to track the media time and statistics updates.
constexpr base::TimeDelta kTrackingWindow = base::TimeDelta::FromSeconds(5);

// The allowed delay for the remoting playback. When continuously exceeds this
// limit for |kPlaybackDelayCountThreshold| times, the user experience is likely
// poor and the controller is notified.
constexpr base::TimeDelta kMediaPlaybackDelayThreshold =
    base::TimeDelta::FromMilliseconds(750);
constexpr int kPlaybackDelayCountThreshold = 3;

// The allowed percentage of the number of video frames dropped vs. the number
// of the video frames decoded. When exceeds this limit, the user experience is
// likely poor and the controller is notified.
constexpr int kMaxNumVideoFramesDroppedPercentage = 3;

// The time period to allow receiver get stable after playback rate change or
// Flush().
constexpr base::TimeDelta kStabilizationPeriod =
    base::TimeDelta::FromSeconds(2);

// The amount of time between polling the DemuxerStreamAdapters to measure their
// data flow rates for metrics.
constexpr base::TimeDelta kDataFlowPollPeriod =
    base::TimeDelta::FromSeconds(10);

}  // namespace

CourierRenderer::CourierRenderer(
    scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
    const base::WeakPtr<RendererController>& controller,
    VideoRendererSink* video_renderer_sink)
    : state_(STATE_UNINITIALIZED),
      main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      media_task_runner_(std::move(media_task_runner)),
      media_resource_(nullptr),
      client_(nullptr),
      controller_(controller),
      rpc_broker_(controller_->GetRpcBroker()),
      rpc_handle_(rpc_broker_->GetUniqueHandle()),
      remote_renderer_handle_(RpcBroker::kInvalidHandle),
      video_renderer_sink_(video_renderer_sink),
      clock_(new base::DefaultTickClock()),
      weak_factory_(this) {
  VLOG(2) << __func__;
  // Note: The constructor is running on the main thread, but will be destroyed
  // on the media thread. Therefore, all weak pointers must be dereferenced on
  // the media thread.
  const RpcBroker::ReceiveMessageCallback receive_callback =
      base::Bind(&CourierRenderer::OnMessageReceivedOnMainThread,
                 media_task_runner_, weak_factory_.GetWeakPtr());
  rpc_broker_->RegisterMessageReceiverCallback(rpc_handle_, receive_callback);
}

CourierRenderer::~CourierRenderer() {
  VLOG(2) << __func__;
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  // Post task on main thread to unregister message receiver.
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&RpcBroker::UnregisterMessageReceiverCallback,
                            rpc_broker_, rpc_handle_));

  if (video_renderer_sink_) {
    video_renderer_sink_->PaintSingleFrame(
        VideoFrame::CreateBlackFrame(gfx::Size(1280, 720)));
  }
}

void CourierRenderer::Initialize(MediaResource* media_resource,
                                 RendererClient* client,
                                 const PipelineStatusCB& init_cb) {
  VLOG(2) << __func__;
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(media_resource);
  DCHECK(client);

  if (state_ != STATE_UNINITIALIZED) {
    media_task_runner_->PostTask(
        FROM_HERE, base::Bind(init_cb, PIPELINE_ERROR_INVALID_STATE));
    return;
  }

  media_resource_ = media_resource;
  client_ = client;
  init_workflow_done_callback_ = init_cb;

  state_ = STATE_CREATE_PIPE;

  // TODO(servolk): Add support for multiple streams. For now use the first
  // enabled audio and video streams to preserve the existing behavior.
  ::media::DemuxerStream* audio_demuxer_stream =
      media_resource_->GetFirstStream(DemuxerStream::AUDIO);
  ::media::DemuxerStream* video_demuxer_stream =
      media_resource_->GetFirstStream(DemuxerStream::VIDEO);

  // Create audio mojo data pipe handles if audio is available.
  std::unique_ptr<mojo::DataPipe> audio_data_pipe;
  if (audio_demuxer_stream) {
    audio_data_pipe = base::WrapUnique(DemuxerStreamAdapter::CreateDataPipe());
  }

  // Create video mojo data pipe handles if video is available.
  std::unique_ptr<mojo::DataPipe> video_data_pipe;
  if (video_demuxer_stream) {
    video_data_pipe = base::WrapUnique(DemuxerStreamAdapter::CreateDataPipe());
  }

  // Establish remoting data pipe connection using main thread.
  const SharedSession::DataPipeStartCallback data_pipe_callback =
      base::Bind(&CourierRenderer::OnDataPipeCreatedOnMainThread,
                 media_task_runner_, weak_factory_.GetWeakPtr(), rpc_broker_);
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&RendererController::StartDataPipe, controller_,
                 base::Passed(&audio_data_pipe), base::Passed(&video_data_pipe),
                 data_pipe_callback));
}

void CourierRenderer::SetCdm(CdmContext* cdm_context,
                             const CdmAttachedCB& cdm_attached_cb) {
  VLOG(2) << __func__ << " cdm_id:" << cdm_context->GetCdmId();
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  // TODO(erickung): add implementation once Remote CDM implementation is done.
  // Right now it returns callback immediately.
  if (!cdm_attached_cb.is_null()) {
    cdm_attached_cb.Run(false);
  }
}

void CourierRenderer::Flush(const base::Closure& flush_cb) {
  VLOG(2) << __func__;
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(flush_cb_.is_null());

  if (state_ != STATE_PLAYING) {
    DCHECK_EQ(state_, STATE_ERROR);
    // In the error state, this renderer will be shut down shortly. To prevent
    // breaking the pipeline impl, just run the done callback (interface
    // requirement).
    media_task_runner_->PostTask(FROM_HERE, flush_cb);
    return;
  }

  state_ = STATE_FLUSHING;
  base::Optional<uint32_t> flush_audio_count;
  if (audio_demuxer_stream_adapter_)
    flush_audio_count = audio_demuxer_stream_adapter_->SignalFlush(true);
  base::Optional<uint32_t> flush_video_count;
  if (video_demuxer_stream_adapter_)
    flush_video_count = video_demuxer_stream_adapter_->SignalFlush(true);
  // Makes sure flush count is valid if stream is available or both audio and
  // video agrees on the same flushing state.
  if ((audio_demuxer_stream_adapter_ && !flush_audio_count.has_value()) ||
      (video_demuxer_stream_adapter_ && !flush_video_count.has_value()) ||
      (audio_demuxer_stream_adapter_ && video_demuxer_stream_adapter_ &&
       flush_audio_count.has_value() != flush_video_count.has_value())) {
    VLOG(1) << "Ignoring flush request while under flushing operation";
    return;
  }

  flush_cb_ = flush_cb;
}

void CourierRenderer::StartPlayingFrom(base::TimeDelta time) {
  VLOG(2) << __func__ << ": " << time.InMicroseconds();
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  if (state_ != STATE_PLAYING) {
    DCHECK_EQ(state_, STATE_ERROR);
    return;
  }

  {
    base::AutoLock auto_lock(time_lock_);
    current_media_time_ = time;
  }
  ResetMeasurements();
}

void CourierRenderer::SetPlaybackRate(double playback_rate) {
  VLOG(2) << __func__ << ": " << playback_rate;
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  if (state_ != STATE_FLUSHING && state_ != STATE_PLAYING) {
    DCHECK_EQ(state_, STATE_ERROR);
    return;
  }

  playback_rate_ = playback_rate;
  ResetMeasurements();
}

void CourierRenderer::SetVolume(float volume) {
  VLOG(2) << __func__ << ": " << volume;
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  if (state_ != STATE_FLUSHING && state_ != STATE_PLAYING) {
    DCHECK_EQ(state_, STATE_ERROR);
    return;
  }
}

base::TimeDelta CourierRenderer::GetMediaTime() {
  // No BelongsToCurrentThread() checking because this can be called from other
  // threads.
  // TODO(erickung): Interpolate current media time using local system time.
  // Current receiver is to update |current_media_time_| every 250ms. But it
  // needs to lower the update frequency in order to reduce network usage. Hence
  // the interpolation is needed after receiver implementation is changed.
  base::AutoLock auto_lock(time_lock_);
  return current_media_time_;
}

// static
void CourierRenderer::OnDataPipeCreatedOnMainThread(
    scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
    base::WeakPtr<CourierRenderer> self,
    base::WeakPtr<RpcBroker> rpc_broker,
    mojom::RemotingDataStreamSenderPtrInfo audio,
    mojom::RemotingDataStreamSenderPtrInfo video,
    mojo::ScopedDataPipeProducerHandle audio_handle,
    mojo::ScopedDataPipeProducerHandle video_handle) {
  media_task_runner->PostTask(
      FROM_HERE,
      base::Bind(&CourierRenderer::OnDataPipeCreated, self,
                 base::Passed(&audio), base::Passed(&video),
                 base::Passed(&audio_handle), base::Passed(&video_handle),
                 rpc_broker ? rpc_broker->GetUniqueHandle()
                            : RpcBroker::kInvalidHandle,
                 rpc_broker ? rpc_broker->GetUniqueHandle()
                            : RpcBroker::kInvalidHandle));
}

void CourierRenderer::OnDataPipeCreated(
    mojom::RemotingDataStreamSenderPtrInfo audio,
    mojom::RemotingDataStreamSenderPtrInfo video,
    mojo::ScopedDataPipeProducerHandle audio_handle,
    mojo::ScopedDataPipeProducerHandle video_handle,
    int audio_rpc_handle,
    int video_rpc_handle) {
  VLOG(2) << __func__;
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  if (state_ == STATE_ERROR)
    return;  // Abort because something went wrong in the meantime.
  DCHECK_EQ(state_, STATE_CREATE_PIPE);
  DCHECK(!init_workflow_done_callback_.is_null());

  // TODO(servolk): Add support for multiple streams. For now use the first
  // enabled audio and video streams to preserve the existing behavior.
  ::media::DemuxerStream* audio_demuxer_stream =
      media_resource_->GetFirstStream(DemuxerStream::AUDIO);
  ::media::DemuxerStream* video_demuxer_stream =
      media_resource_->GetFirstStream(DemuxerStream::VIDEO);

  // Create audio demuxer stream adapter if audio is available.
  if (audio_demuxer_stream && audio.is_valid() && audio_handle.is_valid() &&
      audio_rpc_handle != RpcBroker::kInvalidHandle) {
    VLOG(2) << "Initialize audio";
    audio_demuxer_stream_adapter_.reset(new DemuxerStreamAdapter(
        main_task_runner_, media_task_runner_, "audio", audio_demuxer_stream,
        rpc_broker_, audio_rpc_handle, std::move(audio),
        std::move(audio_handle),
        base::Bind(&CourierRenderer::OnFatalError, base::Unretained(this))));
  }

  // Create video demuxer stream adapter if video is available.
  if (video_demuxer_stream && video.is_valid() && video_handle.is_valid() &&
      video_rpc_handle != RpcBroker::kInvalidHandle) {
    VLOG(2) << "Initialize video";
    video_demuxer_stream_adapter_.reset(new DemuxerStreamAdapter(
        main_task_runner_, media_task_runner_, "video", video_demuxer_stream,
        rpc_broker_, video_rpc_handle, std::move(video),
        std::move(video_handle),
        base::Bind(&CourierRenderer::OnFatalError, base::Unretained(this))));
  }

  // Checks if data pipe is created successfully.
  if (!audio_demuxer_stream_adapter_ && !video_demuxer_stream_adapter_) {
    OnFatalError(DATA_PIPE_CREATE_ERROR);
    return;
  }

  state_ = STATE_ACQUIRING;
}

// static
void CourierRenderer::OnMessageReceivedOnMainThread(
    scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
    base::WeakPtr<CourierRenderer> self,
    std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::OnReceivedRpc(std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::SendRpcToRemote(std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::AcquireRendererDone(
    std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::InitializeCallback(
    std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::FlushUntilCallback() {
  NOTIMPLEMENTED();
}

void CourierRenderer::SetCdmCallback(std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::OnTimeUpdate(std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::OnBufferingStateChange(
    std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::OnAudioConfigChange(
    std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::OnVideoConfigChange(
    std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::OnVideoNaturalSizeChange(
    std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::OnVideoOpacityChange(
    std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::OnStatisticsUpdate(
    std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::OnDurationChange(
    std::unique_ptr<pb::RpcMessage> message) {
  NOTIMPLEMENTED();
}

void CourierRenderer::OnFatalError(StopTrigger stop_trigger) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK_NE(UNKNOWN_STOP_TRIGGER, stop_trigger);

  VLOG(2) << __func__ << " with StopTrigger " << stop_trigger;

  // If this is the first error, notify the controller. It is expected the
  // controller will cause this renderer to shut down shortly.
  if (state_ != STATE_ERROR) {
    state_ = STATE_ERROR;
    main_task_runner_->PostTask(
        FROM_HERE, base::Bind(&RendererController::OnRendererFatalError,
                              controller_, stop_trigger));
  }

  data_flow_poll_timer_.Stop();

  // This renderer will be shut down shortly. To prevent breaking the pipeline,
  // just run the callback without reporting error.
  if (!init_workflow_done_callback_.is_null()) {
    base::ResetAndReturn(&init_workflow_done_callback_).Run(PIPELINE_OK);
    return;
  }

  if (!flush_cb_.is_null())
    base::ResetAndReturn(&flush_cb_).Run();
}

void CourierRenderer::OnMediaTimeUpdated() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (!flush_cb_.is_null())
    return;  // Don't manage and check the queue when Flush() is on-going.

  base::TimeTicks current_time = clock_->NowTicks();
  if (current_time < ignore_updates_until_time_)
    return;  // Not stable yet.

  media_time_queue_.push_back(
      std::make_pair(current_time, current_media_time_));
  base::TimeDelta window_duration =
      current_time - media_time_queue_.front().first;
  if (window_duration < kTrackingWindow)
    return;  // Not enough data to make a reliable decision.

  base::TimeDelta media_duration =
      media_time_queue_.back().second - media_time_queue_.front().second;
  base::TimeDelta update_duration =
      (media_time_queue_.back().first - media_time_queue_.front().first) *
      playback_rate_;
  if ((media_duration - update_duration).magnitude() >=
      kMediaPlaybackDelayThreshold) {
    VLOG(1) << "Irregular playback detected: Media playback delayed."
            << " media_duration = " << media_duration
            << " update_duration = " << update_duration;
    ++times_playback_delayed_;
    if (times_playback_delayed_ == kPlaybackDelayCountThreshold)
      OnFatalError(PACING_TOO_SLOWLY);
  } else {
    times_playback_delayed_ = 0;
  }

  // Prune |media_time_queue_|.
  while (media_time_queue_.back().first - media_time_queue_.front().first >=
         kTrackingWindow)
    media_time_queue_.pop_front();
}

void CourierRenderer::UpdateVideoStatsQueue(int video_frames_decoded,
                                            int video_frames_dropped) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (!flush_cb_.is_null())
    return;  // Don't manage and check the queue when Flush() is on-going.

  if (!stats_updated_) {
    if (video_frames_decoded)
      stats_updated_ = true;
    // Ignore the first stats since it may include the information during
    // unstable period.
    return;
  }

  base::TimeTicks current_time = clock_->NowTicks();
  if (current_time < ignore_updates_until_time_)
    return;  // Not stable yet.

  video_stats_queue_.push_back(std::make_tuple(
      current_time, video_frames_decoded, video_frames_dropped));
  sum_video_frames_decoded_ += video_frames_decoded;
  sum_video_frames_dropped_ += video_frames_dropped;
  base::TimeDelta window_duration =
      current_time - std::get<0>(video_stats_queue_.front());
  if (window_duration < kTrackingWindow)
    return;  // Not enough data to make a reliable decision.

  if (sum_video_frames_decoded_ &&
      sum_video_frames_dropped_ * 100 >
          sum_video_frames_decoded_ * kMaxNumVideoFramesDroppedPercentage) {
    VLOG(1) << "Irregular playback detected: Too many video frames dropped."
            << " video_frames_decoded= " << sum_video_frames_decoded_
            << " video_frames_dropped= " << sum_video_frames_dropped_;
    OnFatalError(FRAME_DROP_RATE_HIGH);
  }
  // Prune |video_stats_queue_|.
  while (std::get<0>(video_stats_queue_.back()) -
             std::get<0>(video_stats_queue_.front()) >=
         kTrackingWindow) {
    sum_video_frames_decoded_ -= std::get<1>(video_stats_queue_.front());
    sum_video_frames_dropped_ -= std::get<2>(video_stats_queue_.front());
    video_stats_queue_.pop_front();
  }
}

void CourierRenderer::ResetMeasurements() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  media_time_queue_.clear();
  video_stats_queue_.clear();
  sum_video_frames_dropped_ = 0;
  sum_video_frames_decoded_ = 0;
  stats_updated_ = false;
  ignore_updates_until_time_ = clock_->NowTicks() + kStabilizationPeriod;

  if (state_ != STATE_ERROR &&
      (audio_demuxer_stream_adapter_ || video_demuxer_stream_adapter_)) {
    data_flow_poll_timer_.Start(FROM_HERE, kDataFlowPollPeriod, this,
                                &CourierRenderer::MeasureAndRecordDataRates);
  }
}

void CourierRenderer::MeasureAndRecordDataRates() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  // Whenever media is first started or flushed/seeked, there is a "burst
  // bufferring" period as the remote device rapidly fills its buffer before
  // resuming playback. Since the goal here is to measure the sustained content
  // bitrates, ignore the byte counts the first time since the last
  // ResetMeasurements() call.
  const base::TimeTicks current_time = clock_->NowTicks();
  if (current_time < ignore_updates_until_time_ + kDataFlowPollPeriod) {
    if (audio_demuxer_stream_adapter_)
      audio_demuxer_stream_adapter_->GetBytesWrittenAndReset();
    if (video_demuxer_stream_adapter_)
      video_demuxer_stream_adapter_->GetBytesWrittenAndReset();
    return;
  }

  const int kBytesPerKilobit = 1024 / 8;
  if (audio_demuxer_stream_adapter_) {
    const double kilobits_per_second =
        (audio_demuxer_stream_adapter_->GetBytesWrittenAndReset() /
         kDataFlowPollPeriod.InSecondsF()) /
        kBytesPerKilobit;
    DCHECK_GE(kilobits_per_second, 0);
    const base::CheckedNumeric<int> checked_kbps = kilobits_per_second;
    metrics_recorder_.OnAudioRateEstimate(
        checked_kbps.ValueOrDefault(std::numeric_limits<int>::max()));
  }
  if (video_demuxer_stream_adapter_) {
    const double kilobits_per_second =
        (video_demuxer_stream_adapter_->GetBytesWrittenAndReset() /
         kDataFlowPollPeriod.InSecondsF()) /
        kBytesPerKilobit;
    DCHECK_GE(kilobits_per_second, 0);
    const base::CheckedNumeric<int> checked_kbps = kilobits_per_second;
    metrics_recorder_.OnVideoRateEstimate(
        checked_kbps.ValueOrDefault(std::numeric_limits<int>::max()));
  }
}

}  // namespace remoting
}  // namespace media
