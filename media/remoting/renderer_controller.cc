// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/remoting/renderer_controller.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "media/base/bind_to_current_loop.h"
#include "media/remoting/remoting_cdm.h"
#include "media/remoting/remoting_cdm_context.h"

namespace media {
namespace remoting {

namespace {

// The time window in seconds to etimate the bitrate.
constexpr base::TimeDelta kBitrateEstimationDuration =
    base::TimeDelta::FromSeconds(5);
// The max bitrate that Media Remoting supports.
constexpr int kMaxRemotingBitrate = 5000000;

}  // namespace

RendererController::RendererController(scoped_refptr<SharedSession> session)
    : session_(std::move(session)), weak_factory_(this) {
  session_->AddClient(this);
}

RendererController::~RendererController() {
  DCHECK(thread_checker_.CalledOnValidThread());
  metrics_recorder_.WillStopSession(MEDIA_ELEMENT_DESTROYED);
  session_->RemoveClient(this);
}

void RendererController::OnStarted(bool success) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (success) {
    VLOG(1) << "Remoting started successively.";
    if (remote_rendering_started_) {
      metrics_recorder_.DidStartSession();
      DCHECK(client_);
      client_->SwitchRenderer(true);
    } else {
      session_->StopRemoting(this);
    }
  } else {
    VLOG(1) << "Failed to start remoting.";
    remote_rendering_started_ = false;
    metrics_recorder_.WillStopSession(START_RACE);
  }
}

void RendererController::OnSessionStateChanged() {
  DCHECK(thread_checker_.CalledOnValidThread());
  UpdateFromSessionState(SINK_AVAILABLE, ROUTE_TERMINATED);
}

void RendererController::UpdateFromSessionState(StartTrigger start_trigger,
                                                StopTrigger stop_trigger) {
  VLOG(1) << "UpdateFromSessionState: " << session_->state();
  if (client_)
    client_->ActivateViewportIntersectionMonitoring(IsRemoteSinkAvailable());

  UpdateAndMaybeSwitch(start_trigger, stop_trigger);
}

bool RendererController::IsRemoteSinkAvailable() {
  DCHECK(thread_checker_.CalledOnValidThread());

  switch (session_->state()) {
    case SharedSession::SESSION_CAN_START:
    case SharedSession::SESSION_STARTING:
    case SharedSession::SESSION_STARTED:
      return true;
    case SharedSession::SESSION_UNAVAILABLE:
    case SharedSession::SESSION_STOPPING:
    case SharedSession::SESSION_PERMANENTLY_STOPPED:
      return false;
  }

  NOTREACHED();
  return false;  // To suppress compiler warning on Windows.
}

void RendererController::OnEnteredFullscreen() {
  DCHECK(thread_checker_.CalledOnValidThread());

  is_fullscreen_ = true;
  // See notes in OnBecameDominantVisibleContent() for why this is forced:
  is_dominant_content_ = true;
  UpdateAndMaybeSwitch(ENTERED_FULLSCREEN, UNKNOWN_STOP_TRIGGER);
}

void RendererController::OnExitedFullscreen() {
  DCHECK(thread_checker_.CalledOnValidThread());

  is_fullscreen_ = false;
  // See notes in OnBecameDominantVisibleContent() for why this is forced:
  is_dominant_content_ = false;
  UpdateAndMaybeSwitch(UNKNOWN_START_TRIGGER, EXITED_FULLSCREEN);
}

void RendererController::OnBecameDominantVisibleContent(bool is_dominant) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Two scenarios where "dominance" status mixes with fullscreen transitions:
  //
  //   1. Just before/after entering fullscreen, the element will, of course,
  //      become the dominant on-screen content via automatic page layout.
  //   2. Just before/after exiting fullscreen, the element may or may not
  //      shrink in size enough to become non-dominant. However, exiting
  //      fullscreen was caused by a user action that explicitly indicates a
  //      desire to exit remoting, so even if the element is still dominant,
  //      remoting should be shut down.
  //
  // Thus, to achieve the desired behaviors, |is_dominant_content_| is force-set
  // in OnEnteredFullscreen() and OnExitedFullscreen(), and changes to it here
  // are ignored while in fullscreen.
  if (is_fullscreen_)
    return;

  is_dominant_content_ = is_dominant;
  UpdateAndMaybeSwitch(BECAME_DOMINANT_CONTENT, BECAME_AUXILIARY_CONTENT);
}

void RendererController::OnSetCdm(CdmContext* cdm_context) {
  DCHECK(thread_checker_.CalledOnValidThread());

  auto* remoting_cdm_context = RemotingCdmContext::From(cdm_context);
  if (!remoting_cdm_context)
    return;

  session_->RemoveClient(this);
  session_ = remoting_cdm_context->GetSharedSession();
  session_->AddClient(this);
  UpdateFromSessionState(CDM_READY, DECRYPTION_ERROR);
}

void RendererController::OnRemotePlaybackDisabled(bool disabled) {
  DCHECK(thread_checker_.CalledOnValidThread());

  is_remote_playback_disabled_ = disabled;
  metrics_recorder_.OnRemotePlaybackDisabled(disabled);
  UpdateAndMaybeSwitch(ENABLED_BY_PAGE, DISABLED_BY_PAGE);
}

base::WeakPtr<RpcBroker> RendererController::GetRpcBroker() const {
  DCHECK(thread_checker_.CalledOnValidThread());

  return session_->rpc_broker()->GetWeakPtr();
}

void RendererController::StartDataPipe(
    std::unique_ptr<mojo::DataPipe> audio_data_pipe,
    std::unique_ptr<mojo::DataPipe> video_data_pipe,
    const SharedSession::DataPipeStartCallback& done_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  session_->StartDataPipe(std::move(audio_data_pipe),
                          std::move(video_data_pipe), done_callback);
}

void RendererController::OnMetadataChanged(const PipelineMetadata& metadata) {
  DCHECK(thread_checker_.CalledOnValidThread());

  const bool was_audio_codec_supported = has_audio() && IsAudioCodecSupported();
  const bool was_video_codec_supported = has_video() && IsVideoCodecSupported();
  pipeline_metadata_ = metadata;
  const bool is_audio_codec_supported = has_audio() && IsAudioCodecSupported();
  const bool is_video_codec_supported = has_video() && IsVideoCodecSupported();
  metrics_recorder_.OnPipelineMetadataChanged(metadata);

  is_encrypted_ = false;
  if (has_video())
    is_encrypted_ |= metadata.video_decoder_config.is_encrypted();
  if (has_audio())
    is_encrypted_ |= metadata.audio_decoder_config.is_encrypted();

  StartTrigger start_trigger = UNKNOWN_START_TRIGGER;
  if (!was_audio_codec_supported && is_audio_codec_supported)
    start_trigger = SUPPORTED_AUDIO_CODEC;
  if (!was_video_codec_supported && is_video_codec_supported) {
    start_trigger = start_trigger == SUPPORTED_AUDIO_CODEC
                        ? SUPPORTED_AUDIO_AND_VIDEO_CODECS
                        : SUPPORTED_VIDEO_CODEC;
  }
  StopTrigger stop_trigger = UNKNOWN_STOP_TRIGGER;
  if (was_audio_codec_supported && !is_audio_codec_supported)
    stop_trigger = UNSUPPORTED_AUDIO_CODEC;
  if (was_video_codec_supported && !is_video_codec_supported) {
    stop_trigger = stop_trigger == UNSUPPORTED_AUDIO_CODEC
                       ? UNSUPPORTED_AUDIO_AND_VIDEO_CODECS
                       : UNSUPPORTED_VIDEO_CODEC;
  }
  UpdateAndMaybeSwitch(start_trigger, stop_trigger);
}

bool RendererController::IsVideoCodecSupported() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(has_video());

  switch (pipeline_metadata_.video_decoder_config.codec()) {
    case VideoCodec::kCodecH264:
    case VideoCodec::kCodecVP8:
      return true;
    default:
      VLOG(2) << "Remoting does not support video codec: "
              << pipeline_metadata_.video_decoder_config.codec();
      return false;
  }
}

bool RendererController::IsAudioCodecSupported() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(has_audio());

  switch (pipeline_metadata_.audio_decoder_config.codec()) {
    case AudioCodec::kCodecAAC:
    case AudioCodec::kCodecMP3:
    case AudioCodec::kCodecPCM:
    case AudioCodec::kCodecVorbis:
    case AudioCodec::kCodecFLAC:
    case AudioCodec::kCodecAMR_NB:
    case AudioCodec::kCodecAMR_WB:
    case AudioCodec::kCodecPCM_MULAW:
    case AudioCodec::kCodecGSM_MS:
    case AudioCodec::kCodecPCM_S16BE:
    case AudioCodec::kCodecPCM_S24BE:
    case AudioCodec::kCodecOpus:
    case AudioCodec::kCodecEAC3:
    case AudioCodec::kCodecPCM_ALAW:
    case AudioCodec::kCodecALAC:
    case AudioCodec::kCodecAC3:
      return true;
    default:
      VLOG(2) << "Remoting does not support audio codec: "
              << pipeline_metadata_.audio_decoder_config.codec();
      return false;
  }
}

void RendererController::OnPlaying() {
  DCHECK(thread_checker_.CalledOnValidThread());

  is_paused_ = false;
  UpdateAndMaybeSwitch(PLAY_COMMAND, UNKNOWN_STOP_TRIGGER);
}

void RendererController::OnPaused() {
  DCHECK(thread_checker_.CalledOnValidThread());

  is_paused_ = true;

  // Stop the bitrate estimation to avoid inaccurate estimation or starting
  // remoting while paused.
  if (bitrate_estimating_) {
    client_->StopVideoStreamBitrateEstimation();
    bitrate_estimating_ = false;
  }
}

bool RendererController::ShouldBeRemoting() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!client_) {
    DCHECK(!remote_rendering_started_);
    return false;  // No way to switch to the remoting renderer.
  }

  const SharedSession::SessionState state = session_->state();
  if (is_encrypted_) {
    // Due to technical limitations when playing encrypted content, once a
    // remoting session has been started, playback cannot be resumed locally
    // without reloading the page, so leave the CourierRenderer in-place to
    // avoid having the default renderer attempt and fail to play the content.
    //
    // TODO(miu): Revisit this once more of the encrypted-remoting impl is
    // in-place. For example, this will prevent metrics from recording session
    // stop reasons.
    return state == SharedSession::SESSION_STARTED ||
           state == SharedSession::SESSION_STOPPING ||
           state == SharedSession::SESSION_PERMANENTLY_STOPPED;
  }

  if (encountered_renderer_fatal_error_)
    return false;

  switch (state) {
    case SharedSession::SESSION_UNAVAILABLE:
      return false;  // Cannot remote media without a remote sink.
    case SharedSession::SESSION_CAN_START:
    case SharedSession::SESSION_STARTING:
    case SharedSession::SESSION_STARTED:
      break;  // Media remoting is possible, assuming other requirments are met.
    case SharedSession::SESSION_STOPPING:
    case SharedSession::SESSION_PERMANENTLY_STOPPED:
      return false;  // Use local rendering after stopping remoting.
  }

  switch (session_->sink_capabilities()) {
    case mojom::RemotingSinkCapabilities::NONE:
      return false;
    case mojom::RemotingSinkCapabilities::RENDERING_ONLY:
    case mojom::RemotingSinkCapabilities::CONTENT_DECRYPTION_AND_RENDERING:
      break;  // The sink is capable of remote rendering.
  }

  if ((!has_audio() && !has_video()) ||
      (has_video() && !IsVideoCodecSupported()) ||
      (has_audio() && !IsAudioCodecSupported())) {
    return false;
  }

  if (is_remote_playback_disabled_)
    return false;

  // Normally, entering fullscreen or being the dominant visible content is the
  // signal that starts remote rendering. However, current technical limitations
  // require encrypted content be remoted without waiting for a user signal.
  return is_fullscreen_ || is_dominant_content_;
}

void RendererController::UpdateAndMaybeSwitch(StartTrigger start_trigger,
                                              StopTrigger stop_trigger) {
  DCHECK(thread_checker_.CalledOnValidThread());

  bool should_be_remoting = ShouldBeRemoting();

  if (!should_be_remoting && bitrate_estimating_) {
    client_->StopVideoStreamBitrateEstimation();
    bitrate_estimating_ = false;
  }

  if (remote_rendering_started_ == should_be_remoting)
    return;

  if (should_be_remoting)
    MaybeSwitchToRemoting(start_trigger);
  else
    SwitchToLocalRenderer(stop_trigger);
}

void RendererController::MaybeSwitchToRemoting(StartTrigger start_trigger) {
  DCHECK(!remote_rendering_started_);
  DCHECK(client_);
  DCHECK_NE(start_trigger, UNKNOWN_START_TRIGGER);

  // Only switch to remoting when media is playing. Since the renderer is
  // created when video starts loading/playing, receiver will display a black
  // screen before video starts playing if switching to remoting when paused.
  // Thus, the user experience is improved by not starting remoting until
  // playback resumes.
  if (is_paused_)
    return;

  if (session_->state() == SharedSession::SESSION_PERMANENTLY_STOPPED) {
    remote_rendering_started_ = true;
    client_->SwitchRenderer(true);
    return;
  }

  // Start remoting for encrypted content or audio only content without bitrate
  // check.
  if (start_trigger == CDM_READY || !has_video()) {
    remote_rendering_started_ = true;
    metrics_recorder_.WillStartSession(start_trigger);
    // |MediaObserverClient::SwitchRenderer()| will be called after remoting is
    // started successfully.
    session_->StartRemoting(this);
    return;
  }

  // Bitrate estimation is already in progress. There is no need to start it
  // again.
  if (bitrate_estimating_)
    return;

  // Starts bitrate estimation and will switch to remoting renderer if the
  // estimated bitrate is supported by remoting.
  bitrate_estimating_ = true;
  VLOG(2) << "Start video stream bitrate estimation.";
  client_->StartVideoStreamBitrateEstimation(
      kBitrateEstimationDuration,
      BindToCurrentLoop(base::BindOnce(&RendererController::OnBitrateEstimated,
                                       weak_factory_.GetWeakPtr(),
                                       start_trigger)));
}

void RendererController::SwitchToLocalRenderer(StopTrigger stop_trigger) {
  DCHECK(remote_rendering_started_);
  DCHECK(client_);
  // For encrypted content, it's only valid to switch to remoting renderer,
  // and never back to the local renderer. The RemotingCdmController will
  // force-stop the session when remoting has ended; so no need to call
  // StopRemoting() from here.
  DCHECK(!is_encrypted_);
  DCHECK_NE(stop_trigger, UNKNOWN_STOP_TRIGGER);

  remote_rendering_started_ = false;
  metrics_recorder_.WillStopSession(stop_trigger);
  client_->SwitchRenderer(false);
  session_->StopRemoting(this);
}

void RendererController::OnBitrateEstimated(StartTrigger start_trigger,
                                            BitrateEstimator::Status status,
                                            int bitrate) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Bitrate estimation was stopped, which indicates the remoting start request
  // was cancelled.
  if (!bitrate_estimating_)
    return;
  bitrate_estimating_ = false;
  if (status != BitrateEstimator::Status::kOk) {
    VLOG(1) << "Warning: Bitrate estimation fails with bitrate = " << bitrate;
    metrics_recorder_.WillStopSession(StopTrigger::BITRATE_ESTIMATION_ABORTED);
    // Don't start remoting if the bitrate estimation failed.
    return;
  }

  DCHECK_NE(start_trigger, UNKNOWN_START_TRIGGER);
  DCHECK(!remote_rendering_started_);

  // TODO(xjz): In an incoming change, use receiver's info (model, wifi snr,
  // etc.) to decide the max bitrate that remoting can support. After mirror
  // service refactoring is done, use the estimated network bandwidth in this
  // decision.
  if (bitrate > kMaxRemotingBitrate) {
    VLOG(1) << "Disable media remoting: content bitrate = " << bitrate;
    encountered_renderer_fatal_error_ = true;
    metrics_recorder_.WillStopSession(StopTrigger::CONTENT_BITRATE_TOO_HIGH);
    return;
  }

  remote_rendering_started_ = true;
  metrics_recorder_.WillStartSession(start_trigger);
  // |MediaObserverClient::SwitchRenderer()| will be called after remoting is
  // started successfully.
  session_->StartRemoting(this);
}

void RendererController::OnRendererFatalError(StopTrigger stop_trigger) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Do not act on errors caused by things like Mojo pipes being closed during
  // shutdown.
  if (!remote_rendering_started_)
    return;

  encountered_renderer_fatal_error_ = true;
  UpdateAndMaybeSwitch(UNKNOWN_START_TRIGGER, stop_trigger);
}

void RendererController::SetClient(MediaObserverClient* client) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(client);
  DCHECK(!client_);

  client_ = client;
  client_->ActivateViewportIntersectionMonitoring(IsRemoteSinkAvailable());
}

}  // namespace remoting
}  // namespace media
