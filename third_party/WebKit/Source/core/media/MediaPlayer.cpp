// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/media/MediaPlayer.h"

#include <limits>
#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptController.h"
#include "bindings/core/v8/ScriptEventListener.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/HTMLNames.h"
#include "core/css/MediaList.h"
#include "core/dom/Attribute.h"
#include "core/dom/DOMException.h"
#include "core/dom/ElementTraversal.h"
#include "core/dom/ShadowRoot.h"
#include "core/dom/TaskRunnerHelper.h"
#include "core/dom/events/Event.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameClient.h"
#include "core/frame/LocalFrameView.h"
#include "core/frame/Settings.h"
#include "core/frame/UseCounter.h"
#include "core/frame/csp/ContentSecurityPolicy.h"
#include "core/html/HTMLSourceElement.h"
#include "core/html/HTMLTrackElement.h"
#include "core/html/TimeRanges.h"
#include "core/html/media/HTMLMediaSource.h"
#include "core/html/media/MediaControls.h"
#include "core/html/media/MediaError.h"
#include "core/html/media/MediaFragmentURIParser.h"
#include "core/html/track/AudioTrack.h"
#include "core/html/track/AudioTrackList.h"
#include "core/html/track/AutomaticTrackSelection.h"
#include "core/html/track/CueTimeline.h"
#include "core/html/track/InbandTextTrack.h"
#include "core/html/track/TextTrackContainer.h"
#include "core/html/track/TextTrackList.h"
#include "core/html/track/VideoTrack.h"
#include "core/html/track/VideoTrackList.h"
#include "core/inspector/ConsoleMessage.h"
#include "core/layout/IntersectionGeometry.h"
#include "core/layout/LayoutMedia.h"
#include "core/layout/api/LayoutViewItem.h"
#include "core/media/MediaPlayerClient.h"
#include "core/page/ChromeClient.h"
#include "platform/Histogram.h"
#include "platform/LayoutTestSupport.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/audio/AudioBus.h"
#include "platform/audio/AudioSourceProviderClient.h"
#include "platform/bindings/Microtask.h"
#include "platform/graphics/GraphicsLayer.h"
#include "platform/mediastream/MediaStreamDescriptor.h"
#include "platform/network/NetworkStateNotifier.h"
#include "platform/network/ParsedContentType.h"
#include "platform/network/mime/ContentType.h"
#include "platform/network/mime/MIMETypeFromURL.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/AutoReset.h"
#include "platform/wtf/CurrentTime.h"
#include "platform/wtf/MathExtras.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/text/CString.h"
#include "public/platform/Platform.h"
#include "public/platform/WebAudioSourceProvider.h"
#include "public/platform/WebContentDecryptionModule.h"
#include "public/platform/WebInbandTextTrack.h"
#include "public/platform/WebMediaPlayerSource.h"
#include "public/platform/WebMediaStream.h"
#include "public/platform/modules/remoteplayback/WebRemotePlaybackAvailability.h"
#include "public/platform/modules/remoteplayback/WebRemotePlaybackClient.h"
#include "public/platform/modules/remoteplayback/WebRemotePlaybackState.h"

#ifndef BLINK_MEDIA_LOG
#define BLINK_MEDIA_LOG DVLOG(3)
#endif

#ifndef LOG_MEDIA_EVENTS
// Default to not logging events because so many are generated they can
// overwhelm the rest of the logging.
#define LOG_MEDIA_EVENTS 0
#endif

#ifndef LOG_OFFICIAL_TIME_STATUS
// Default to not logging status of official time because it adds a fair amount
// of overhead and logging.
#define LOG_OFFICIAL_TIME_STATUS 0
#endif

namespace blink {

using namespace HTMLNames;

using WeakMediaElementSet = HeapHashSet<WeakMember<MediaPlayer>>;
using DocumentElementSetMap =
    HeapHashMap<WeakMember<Document>, Member<WeakMediaElementSet>>;

namespace {

constexpr double kCheckViewportIntersectionIntervalSeconds = 1;

// This enum is used to record histograms. Do not reorder.
enum MediaControlsShow {
  kMediaControlsShowAttribute = 0,
  kMediaControlsShowFullscreen,
  kMediaControlsShowNoScript,
  kMediaControlsShowNotShown,
  kMediaControlsShowDisabledSettings,
  kMediaControlsShowMax
};

// These values are used for the Media.MediaElement.ContentTypeResult histogram.
// Do not reorder.
enum ContentTypeParseableResult {
  kIsSupportedParseable = 0,
  kMayBeSupportedParseable,
  kIsNotSupportedParseable,
  kIsSupportedNotParseable,
  kMayBeSupportedNotParseable,
  kIsNotSupportedNotParseable,
  kContentTypeParseableMax
};

// This enum is used to record histograms. Do not reorder.
enum class PlayPromiseRejectReason {
  kFailedAutoplayPolicy = 0,
  kNoSupportedSources,
  kInterruptedByPause,
  kInterruptedByLoad,
  kCount,
};

void ReportContentTypeResultToUMA(String content_type,
                                  MIMETypeRegistry::SupportsType result) {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      EnumerationHistogram, content_type_parseable_histogram,
      ("Media.MediaElement.ContentTypeParseable", kContentTypeParseableMax));
  ParsedContentType parsed_content_type(content_type);
  ContentTypeParseableResult uma_result = kIsNotSupportedNotParseable;
  switch (result) {
    case MIMETypeRegistry::kIsSupported:
      uma_result = parsed_content_type.IsValid() ? kIsSupportedParseable
                                                 : kIsSupportedNotParseable;
      break;
    case MIMETypeRegistry::kMayBeSupported:
      uma_result = parsed_content_type.IsValid() ? kMayBeSupportedParseable
                                                 : kMayBeSupportedNotParseable;
      break;
    case MIMETypeRegistry::kIsNotSupported:
      uma_result = parsed_content_type.IsValid() ? kIsNotSupportedParseable
                                                 : kIsNotSupportedNotParseable;
      break;
  }
  content_type_parseable_histogram.Count(uma_result);
}

String UrlForLoggingMedia(const KURL& url) {
  static const unsigned kMaximumURLLengthForLogging = 128;

  if (url.GetString().length() < kMaximumURLLengthForLogging)
    return url.GetString();
  return url.GetString().Substring(0, kMaximumURLLengthForLogging) + "...";
}

const char* BoolString(bool val) {
  return val ? "true" : "false";
}

String BuildElementErrorMessage(const String& error) {
  // Prepend a UA-specific-error code before the first ':', to enable better
  // collection and aggregation of UA-specific-error codes from
  // MediaError.message by web apps. WebMediaPlayer::GetErrorMessage() should
  // similarly conform to this format.
  DEFINE_STATIC_LOCAL(const String, element_error_prefix,
                      ("MEDIA_ELEMENT_ERROR: "));
  StringBuilder builder;
  builder.Append(element_error_prefix);
  builder.Append(error);
  return builder.ToString();
}

class AudioSourceProviderClientLockScope {
  STACK_ALLOCATED();

 public:
  AudioSourceProviderClientLockScope(MediaPlayer& element)
      : client_(element.AudioSourceNode()) {
    if (client_)
      client_->lock();
  }
  ~AudioSourceProviderClientLockScope() {
    if (client_)
      client_->unlock();
  }

 private:
  Member<AudioSourceProviderClient> client_;
};

bool CanLoadURL(const KURL& url, const String& content_type_str) {
  DEFINE_STATIC_LOCAL(const String, codecs, ("codecs"));

  ContentType content_type(content_type_str);
  String content_mime_type = content_type.GetType().DeprecatedLower();
  String content_type_codecs = content_type.Parameter(codecs);

  // If the MIME type is missing or is not meaningful, try to figure it out from
  // the URL.
  if (content_mime_type.IsEmpty() ||
      content_mime_type == "application/octet-stream" ||
      content_mime_type == "text/plain") {
    if (url.ProtocolIsData())
      content_mime_type = MimeTypeFromDataURL(url.GetString());
  }

  // If no MIME type is specified, always attempt to load.
  if (content_mime_type.IsEmpty())
    return true;

  // 4.8.12.3 MIME types - In the absence of a specification to the contrary,
  // the MIME type "application/octet-stream" when used with parameters, e.g.
  // "application/octet-stream;codecs=theora", is a type that the user agent
  // knows it cannot render.
  if (content_mime_type != "application/octet-stream" ||
      content_type_codecs.IsEmpty()) {
    return MIMETypeRegistry::SupportsMediaMIMEType(content_mime_type,
                                                   content_type_codecs) !=
           MIMETypeRegistry::kIsNotSupported;
  }

  return false;
}

String PreloadTypeToString(WebMediaPlayer::Preload preload_type) {
  switch (preload_type) {
    case WebMediaPlayer::kPreloadNone:
      return "none";
    case WebMediaPlayer::kPreloadMetaData:
      return "metadata";
    case WebMediaPlayer::kPreloadAuto:
      return "auto";
  }

  NOTREACHED();
  return String();
}

void RecordPlayPromiseRejected(PlayPromiseRejectReason reason) {
  DEFINE_STATIC_LOCAL(EnumerationHistogram, histogram,
                      ("Media.MediaElement.PlayPromiseReject",
                       static_cast<int>(PlayPromiseRejectReason::kCount)));
  histogram.Count(static_cast<int>(reason));
}

}  // anonymous namespace

MIMETypeRegistry::SupportsType MediaPlayer::GetSupportsType(
    const ContentType& content_type) {
  DEFINE_STATIC_LOCAL(const String, codecs, ("codecs"));

  String type = content_type.GetType().DeprecatedLower();
  // The codecs string is not lower-cased because MP4 values are case sensitive
  // per http://tools.ietf.org/html/rfc4281#page-7.
  String type_codecs = content_type.Parameter(codecs);

  if (type.IsEmpty())
    return MIMETypeRegistry::kIsNotSupported;

  // 4.8.12.3 MIME types - The canPlayType(type) method must return the empty
  // string if type is a type that the user agent knows it cannot render or is
  // the type "application/octet-stream"
  if (type == "application/octet-stream")
    return MIMETypeRegistry::kIsNotSupported;

  // Check if stricter parsing of |contentType| will cause problems.
  // TODO(jrummell): Either switch to ParsedContentType or remove this UMA,
  // depending on the results reported.
  MIMETypeRegistry::SupportsType result =
      MIMETypeRegistry::SupportsMediaMIMEType(type, type_codecs);
  ReportContentTypeResultToUMA(content_type.Raw(), result);
  return result;
}

URLRegistry* MediaPlayer::media_stream_registry_ = 0;

void MediaPlayer::SetMediaStreamRegistry(URLRegistry* registry) {
  DCHECK(!media_stream_registry_);
  media_stream_registry_ = registry;
}

bool MediaPlayer::IsMediaStreamURL(const String& url) {
  return media_stream_registry_ ? media_stream_registry_->Contains(url) : false;
}

bool MediaPlayer::IsHLSURL(const KURL& url) {
  // Keep the same logic as in media_codec_util.h.
  if (url.IsNull() || url.IsEmpty())
    return false;

  if (!url.IsLocalFile() && !url.ProtocolIs("http") && !url.ProtocolIs("https"))
    return false;

  return url.GetString().Contains("m3u8");
}

bool MediaPlayer::MediaTracksEnabledInternally() {
  return RuntimeEnabledFeatures::AudioVideoTracksEnabled() ||
         RuntimeEnabledFeatures::BackgroundVideoTrackOptimizationEnabled();
}

MediaPlayer::MediaPlayer(ExecutionContext* context,
                         EventTarget* owner,
                         MediaPlayerClient* client)
    : ContextLifecycleObserver(context),
      client_(client),
      load_timer_(TaskRunnerHelper::Get(TaskType::kUnthrottled, context),
                  this,
                  &MediaPlayer::LoadTimerFired),
      progress_event_timer_(
          TaskRunnerHelper::Get(TaskType::kUnthrottled, context),
          this,
          &MediaPlayer::ProgressEventTimerFired),
      playback_progress_timer_(
          TaskRunnerHelper::Get(TaskType::kUnthrottled, context),
          this,
          &MediaPlayer::PlaybackProgressTimerFired),
      viewport_fill_debouncer_timer_(
          TaskRunnerHelper::Get(TaskType::kUnthrottled, context),
          this,
          &MediaPlayer::ViewportFillDebouncerTimerFired),
      check_viewport_intersection_timer_(
          TaskRunnerHelper::Get(TaskType::kUnthrottled, context),
          this,
          &MediaPlayer::CheckViewportIntersectionTimerFired),
      played_time_ranges_(),
      async_event_queue_(MediaElementEventQueue::Create(owner, context)),
      playback_rate_(1.0f),
      default_playback_rate_(1.0f),
      network_state_(kNetworkEmpty),
      ready_state_(kHaveNothing),
      ready_state_maximum_(kHaveNothing),
      volume_(1.0f),
      last_seek_time_(0),
      previous_progress_time_(std::numeric_limits<double>::max()),
      duration_(std::numeric_limits<double>::quiet_NaN()),
      last_time_update_event_wall_time_(0),
      last_time_update_event_media_time_(
          std::numeric_limits<double>::quiet_NaN()),
      default_playback_start_position_(0),
      load_state_(kWaitingForSource),
      deferred_load_state_(kNotDeferred),
      deferred_load_timer_(
          TaskRunnerHelper::Get(TaskType::kUnthrottled, context),
          this,
          &MediaPlayer::DeferredLoadTimerFired),
      web_layer_(nullptr),
      display_mode_(kUnknown),
      official_playback_position_(0),
      official_playback_position_needs_update_(true),
      fragment_end_time_(std::numeric_limits<double>::quiet_NaN()),
      pending_action_flags_(0),
      playing_(false),
      should_delay_load_event_(false),
      have_fired_loaded_data_(false),
      can_autoplay_(true),
      muted_(false),
      paused_(true),
      seeking_(false),
      sent_stalled_event_(false),
      ignore_preload_none_(false),
      text_tracks_visible_(false),
      should_perform_automatic_track_selection_(true),
      tracks_are_ready_(true),
      processing_preference_change_(false),
      playing_remotely_(false),
      in_overlay_fullscreen_video_(false),
      mostly_filling_viewport_(false),
      audio_source_node_(nullptr),
      remote_playback_client_(nullptr) {
  BLINK_MEDIA_LOG << "MediaPlayer(" << (void*)this << ")";
}

MediaPlayer::~MediaPlayer() {
  BLINK_MEDIA_LOG << "~MediaPlayer(" << (void*)this << ")";

  // audio_source_node_ is explicitly cleared by AudioNode::dispose().
  // Since AudioNode::dispose() is guaranteed to be always called before
  // the AudioNode is destructed, audio_source_node_ is explicitly cleared
  // even if the AudioNode and the MediaPlayer die together.
  DCHECK(!audio_source_node_);
}

void MediaPlayer::Dispose() {
  CloseMediaSource();

  // Destroying the player may cause a resource load to be canceled,
  // which could result in LocalDOMWindow::dispatchWindowLoadEvent() being
  // called via ResourceFetch::didLoadResource(), then
  // FrameLoader::checkCompleted(). But it's guaranteed that the load event
  // doesn't get dispatched during the object destruction.
  // See Document::isDelayingLoadEvent().
  // Also see http://crbug.com/275223 for more details.
  ClearMediaPlayerAndAudioSourceProviderClientWithoutLocking();
}

void MediaPlayer::ScheduleTextTrackResourceLoad() {
  BLINK_MEDIA_LOG << "scheduleTextTrackResourceLoad(" << (void*)this << ")";

  pending_action_flags_ |= kLoadTextTrackResource;

  if (!load_timer_.IsActive())
    load_timer_.StartOneShot(0, BLINK_FROM_HERE);
}

void MediaPlayer::ScheduleNextSourceChild() {
  // Schedule the timer to try the next <source> element WITHOUT resetting state
  // ala invokeLoadAlgorithm.
  pending_action_flags_ |= kLoadMediaResource;
  load_timer_.StartOneShot(0, BLINK_FROM_HERE);
}

void MediaPlayer::ScheduleEvent(const AtomicString& event_name) {
  ScheduleEvent(Event::CreateCancelable(event_name));
}

void MediaPlayer::ScheduleEvent(Event* event) {
#if LOG_MEDIA_EVENTS
  BLINK_MEDIA_LOG << "ScheduleEvent(" << (void*)this << ")"
                  << " - scheduling '" << event->type() << "'";
#endif
  async_event_queue_->EnqueueEvent(BLINK_FROM_HERE, event);
}

void MediaPlayer::LoadTimerFired(TimerBase*) {
  if (pending_action_flags_ & kLoadMediaResource) {
    if (load_state_ == kLoadingFromSourceElement)
      LoadNextSourceChild();
    else
      LoadInternal();
  }

  pending_action_flags_ = 0;
}

MediaError* MediaPlayer::error() const {
  return error_;
}

const String& MediaPlayer::src() const {
  return src_;
}

void MediaPlayer::setSrc(const String& url) {
  BLINK_MEDIA_LOG << "setSrc(" << (void*)this << ")";
  src_ = url;
  InvokeLoadAlgorithm();
}

void MediaPlayer::SetSrcObject(MediaStreamDescriptor* src_object) {
  BLINK_MEDIA_LOG << "setSrcObject(" << (void*)this << ")";
  src_object_ = src_object;
  InvokeLoadAlgorithm();
}

const String& MediaPlayer::crossOrigin() const {
  return cross_origin_;
}

void MediaPlayer::setCrossOrigin(const String& origin) {
  cross_origin_ = origin;
}

MediaPlayer::NetworkState MediaPlayer::getNetworkState() const {
  return network_state_;
}

String MediaPlayer::canPlayType(const String& mime_type) const {
  MIMETypeRegistry::SupportsType support =
      GetSupportsType(ContentType(mime_type));
  String can_play;

  // 4.8.12.3
  switch (support) {
    case MIMETypeRegistry::kIsNotSupported:
      can_play = g_empty_string;
      break;
    case MIMETypeRegistry::kMayBeSupported:
      can_play = "maybe";
      break;
    case MIMETypeRegistry::kIsSupported:
      can_play = "probably";
      break;
  }

  BLINK_MEDIA_LOG << "canPlayType(" << (void*)this << ", " << mime_type
                  << ") -> " << can_play;

  return can_play;
}

void MediaPlayer::load() {
  BLINK_MEDIA_LOG << "load(" << (void*)this << ")";

  ignore_preload_none_ = true;
  InvokeLoadAlgorithm();
}

// TODO(srirama.m): Currently ignore_preload_none_ is reset before calling
// invokeLoadAlgorithm() in all places except load(). Move it inside here
// once microtask is implemented for "Await a stable state" step
// in resource selection algorithm.
void MediaPlayer::InvokeLoadAlgorithm() {
  BLINK_MEDIA_LOG << "invokeLoadAlgorithm(" << (void*)this << ")";

  // Perform the cleanup required for the resource load algorithm to run.
  StopPeriodicTimers();
  load_timer_.Stop();
  CancelDeferredLoad();
  // FIXME: Figure out appropriate place to reset LoadTextTrackResource if
  // necessary and set pending_action_flags_ to 0 here.
  pending_action_flags_ &= ~kLoadMediaResource;
  sent_stalled_event_ = false;
  have_fired_loaded_data_ = false;
  display_mode_ = kUnknown;

  // 1 - Abort any already-running instance of the resource selection algorithm
  // for this element.
  load_state_ = kWaitingForSource;
  current_source_node_ = nullptr;

  // 2 - Let pending tasks be a list of tasks from the media element's media
  // element task source in one of the task queues.
  //
  // 3 - For each task in the pending tasks that would run resolve pending
  // play promises or project pending play prmoises algorithms, immediately
  // resolve or reject those promises in the order the corresponding tasks
  // were queued.
  //
  // TODO(mlamouri): the promises are first resolved then rejected but the
  // order between resolved/rejected promises isn't respected. This could be
  // improved when the same task is used for both cases.
  //
  // TODO(mlamouri): don't run the callback synchronously if we are not allowed
  // to run scripts. It can happen in some edge cases. https://crbug.com/660382
  if (play_promise_resolve_task_handle_.IsActive() &&
      !ScriptForbiddenScope::IsScriptForbidden()) {
    play_promise_resolve_task_handle_.Cancel();
    ResolveScheduledPlayPromises();
  }
  if (play_promise_reject_task_handle_.IsActive() &&
      !ScriptForbiddenScope::IsScriptForbidden()) {
    play_promise_reject_task_handle_.Cancel();
    RejectScheduledPlayPromises();
  }

  // 4 - Remove each task in pending tasks from its task queue.
  CancelPendingEventsAndCallbacks();

  // 5 - If the media element's networkState is set to NETWORK_LOADING or
  // NETWORK_IDLE, queue a task to fire a simple event named abort at the media
  // element.
  if (network_state_ == kNetworkLoading || network_state_ == kNetworkIdle)
    ScheduleEvent(EventTypeNames::abort);

  ResetMediaPlayerAndMediaSource();

  // 6 - If the media element's networkState is not set to NETWORK_EMPTY, then
  // run these substeps
  if (network_state_ != kNetworkEmpty) {
    // 4.1 - Queue a task to fire a simple event named emptied at the media
    // element.
    ScheduleEvent(EventTypeNames::emptied);

    // 4.2 - If a fetching process is in progress for the media element, the
    // user agent should stop it.
    SetNetworkState(kNetworkEmpty);

    // 4.5 - If readyState is not set to kHaveNothing, then set it to that
    // state.
    ready_state_ = kHaveNothing;
    ready_state_maximum_ = kHaveNothing;

    DCHECK(!paused_ || play_promise_resolvers_.IsEmpty());

    // 4.6 - If the paused attribute is false, then run these substeps
    if (!paused_) {
      // 4.6.1 - Set the paused attribute to true.
      paused_ = true;

      // 4.6.2 - Take pending play promises and reject pending play promises
      // with the result and an "AbortError" DOMException.
      RecordPlayPromiseRejected(PlayPromiseRejectReason::kInterruptedByLoad);
      RejectPlayPromises(kAbortError,
                         "The play() request was interrupted by a new load "
                         "request. https://goo.gl/LdLk22");
    }

    // 4.7 - If seeking is true, set it to false.
    seeking_ = false;

    // 4.8 - Set the current playback position to 0.
    //       Set the official playback position to 0.
    //       If this changed the official playback position, then queue a task
    //       to fire a simple event named timeupdate at the media element.
    // 4.9 - Set the initial playback position to 0.
    SetOfficialPlaybackPosition(0);
    ScheduleTimeupdateEvent(false);
  } else if (!paused_) {
    // TODO(foolip): There is a proposal to always reset the paused state
    // in the media element load algorithm, to avoid a bogus play() promise
    // rejection: https://github.com/whatwg/html/issues/869
    // This is where that change would have an effect, and it is measured to
    // verify the assumption that it's a very rare situation.
    UseCounter::Count(GetExecutionContext(),
                      WebFeature::kHTMLMediaElementLoadNetworkEmptyNotPaused);
  }

  // 7 - Set the playbackRate attribute to the value of the defaultPlaybackRate
  // attribute.
  setPlaybackRate(defaultPlaybackRate());

  // 8 - Set the error attribute to null and the can autoplay flag to true.
  error_ = nullptr;
  can_autoplay_ = true;

  // 9 - Invoke the media element's resource selection algorithm.
  InvokeResourceSelectionAlgorithm();

  // 10 - Note: Playback of any previously playing media resource for this
  // element stops.
}

void MediaPlayer::InvokeResourceSelectionAlgorithm() {
  BLINK_MEDIA_LOG << "invokeResourceSelectionAlgorithm(" << (void*)this << ")";
  // The resource selection algorithm
  // 1 - Set the networkState to NETWORK_NO_SOURCE
  SetNetworkState(kNetworkNoSource);

  // 2 - Set the element's show poster flag to true
  // TODO(srirama.m): Introduce show poster flag and update it as per spec

  played_time_ranges_ = TimeRanges::Create();

  // FIXME: Investigate whether these can be moved into network_state_ !=
  // kNetworkEmpty block above
  // so they are closer to the relevant spec steps.
  last_seek_time_ = 0;
  duration_ = std::numeric_limits<double>::quiet_NaN();

  // 3 - Set the media element's delaying-the-load-event flag to true (this
  // delays the load event)
  SetShouldDelayLoadEvent(true);

  // 4 - Await a stable state, allowing the task that invoked this algorithm to
  // continue
  // TODO(srirama.m): Remove scheduleNextSourceChild() and post a microtask
  // instead.  See http://crbug.com/593289 for more details.
  ScheduleNextSourceChild();
}

void MediaPlayer::LoadInternal() {
  SelectMediaResource();
}

void MediaPlayer::SelectMediaResource() {
  BLINK_MEDIA_LOG << "selectMediaResource(" << (void*)this << ")";

  enum Mode { kObject, kAttribute, kChildren, kNothing };
  Mode mode = kNothing;

  // 6 - If the media element has an assigned media provider object, then let
  //     mode be object.
  if (src_object_) {
    mode = kObject;
  } else if (src_) {
    // Otherwise, if the media element has no assigned media provider object
    // but has a src attribute, then let mode be attribute.
    mode = kAttribute;
  } else {
    // Otherwise the media element has no assigned media provider object and
    // has neither a src attribute nor a source element child: set the
    // networkState to kNetworkEmpty, and abort these steps; the synchronous
    // section ends.
    load_state_ = kWaitingForSource;
    SetShouldDelayLoadEvent(false);
    SetNetworkState(kNetworkEmpty);
    UpdateDisplayState();

    BLINK_MEDIA_LOG << "selectMediaResource(" << (void*)this
                    << "), nothing to load";
    return;
  }

  // 7 - Set the media element's networkState to NETWORK_LOADING.
  SetNetworkState(kNetworkLoading);

  // 8 - Queue a task to fire a simple event named loadstart at the media
  // element.
  ScheduleEvent(EventTypeNames::loadstart);

  // 9 - Run the appropriate steps...
  switch (mode) {
    case kObject:
      LoadSourceFromObject();
      BLINK_MEDIA_LOG << "selectMediaResource(" << (void*)this
                      << ", using 'srcObject' attribute";
      break;
    case kAttribute:
      LoadSourceFromAttribute();
      BLINK_MEDIA_LOG << "selectMediaResource(" << (void*)this
                      << "), using 'src' attribute url";
      break;
    case kChildren:
      LoadNextSourceChild();
      BLINK_MEDIA_LOG << "selectMediaResource(" << (void*)this
                      << "), using source element";
      break;
    default:
      NOTREACHED();
  }
}

void MediaPlayer::LoadSourceFromObject() {
  DCHECK(src_object_);
  load_state_ = kLoadingFromSrcObject;

  // No type is available when the resource comes from the 'srcObject'
  // attribute.
  LoadResource(WebMediaPlayerSource(WebMediaStream(src_object_)), String());
}

void MediaPlayer::LoadSourceFromAttribute() {
  load_state_ = kLoadingFromSrcAttr;

  // If the src attribute's value is the empty string ... jump down to the
  // failed step below
  if (src_.IsEmpty()) {
    BLINK_MEDIA_LOG << "LoadSourceFromAttribute(" << (void*)this
                    << "), empty 'src'";
    MediaLoadingFailed(WebMediaPlayer::kNetworkStateFormatError,
                       BuildElementErrorMessage("Empty src attribute"));
    return;
  }

  KURL media_url = GetExecutionContext()->CompleteURL(src_);
  if (!IsSafeToLoadURL(media_url, kComplain)) {
    MediaLoadingFailed(
        WebMediaPlayer::kNetworkStateFormatError,
        BuildElementErrorMessage("Media load rejected by URL safety check"));
    return;
  }

  // No type is available when the url comes from the 'src' attribute so
  // MediaPlayer will have to pick a media engine based on the file extension.
  LoadResource(WebMediaPlayerSource(WebURL(media_url)), String());
}

void MediaPlayer::LoadNextSourceChild() {
  String content_type;
  KURL media_url = SelectNextSourceChild(&content_type, kComplain);
  if (!media_url.IsValid()) {
    WaitForSourceChange();
    return;
  }

  // Reset the MediaPlayer and MediaSource if any
  ResetMediaPlayerAndMediaSource();

  load_state_ = kLoadingFromSourceElement;
  LoadResource(WebMediaPlayerSource(WebURL(media_url)), content_type);
}

void MediaPlayer::LoadResource(const WebMediaPlayerSource& source,
                               const String& content_type) {
  KURL url;
  if (source.IsURL()) {
    url = source.GetAsURL();
    DCHECK(IsSafeToLoadURL(url, kComplain));
    BLINK_MEDIA_LOG << "loadResource(" << (void*)this << ", "
                    << UrlForLoggingMedia(url) << ", " << content_type << ")";
  }

  if (!GetExecutionContext()) {
    MediaLoadingFailed(WebMediaPlayer::kNetworkStateFormatError,
                       BuildElementErrorMessage(
                           "Resource load failure: document has no frame"));
    return;
  }

  // The resource fetch algorithm
  SetNetworkState(kNetworkLoading);

  // Set current_src_ *before* changing to the cache url, the fact that we are
  // loading from the app cache is an internal detail not exposed through the
  // media element API.
  current_src_ = url;

  if (audio_source_node_)
    audio_source_node_->OnCurrentSrcChanged(current_src_);

  BLINK_MEDIA_LOG << "loadResource(" << (void*)this << ") - current_src_ -> "
                  << UrlForLoggingMedia(current_src_);

  StartProgressEventTimer();

  // Reset display mode to force a recalculation of what to show because we are
  // resetting the player.
  SetDisplayMode(kUnknown);

  SetPlayerPreload();

  DCHECK(!media_source_);

  bool attempt_load = true;

  bool can_load_resource =
      source.IsMediaStream() || CanLoadURL(url, content_type);
  if (attempt_load && can_load_resource) {
    DCHECK(!GetWebMediaPlayer());

    // Conditionally defer the load if effective preload is 'none'.
    // Skip this optional deferral for MediaStream sources or any blob URL,
    // including MediaSource blob URLs.
    if (!source.IsMediaStream() && !url.ProtocolIs("blob") &&
        EffectivePreloadType() == WebMediaPlayer::kPreloadNone) {
      BLINK_MEDIA_LOG << "loadResource(" << (void*)this
                      << ") : Delaying load because preload == 'none'";
      DeferLoad();
    } else {
      StartPlayerLoad();
    }
  } else {
    MediaLoadingFailed(
        WebMediaPlayer::kNetworkStateFormatError,
        BuildElementErrorMessage(attempt_load
                                     ? "Unable to load URL due to content type"
                                     : "Unable to attach MediaSource"));
  }

  // If there is no poster to display, allow the media engine to render video
  // frames as soon as they are available.
  UpdateDisplayState();
}

void MediaPlayer::StartPlayerLoad() {
  DCHECK(!web_media_player_);

  WebMediaPlayerSource source;
  if (src_object_) {
    source = WebMediaPlayerSource(WebMediaStream(src_object_));
  } else {
    // Filter out user:pass as those two URL components aren't
    // considered for media resource fetches (including for the CORS
    // use-credentials mode.) That behavior aligns with Gecko, with IE
    // being more restrictive and not allowing fetches to such URLs.
    //
    // Spec reference: http://whatwg.org/c/#concept-media-load-resource
    //
    // FIXME: when the HTML spec switches to specifying resource
    // fetches in terms of Fetch (http://fetch.spec.whatwg.org), and
    // along with that potentially also specifying a setting for its
    // 'authentication flag' to control how user:pass embedded in a
    // media resource URL should be treated, then update the handling
    // here to match.
    KURL request_url = current_src_;
    if (!request_url.User().IsEmpty())
      request_url.SetUser(String());
    if (!request_url.Pass().IsEmpty())
      request_url.SetPass(String());

    KURL kurl(kParsedURLString, request_url);
    source = WebMediaPlayerSource(WebURL(kurl));
  }

  // TODO(srirama.m): Figure out how frame can be null when
  // coming from executeDeferredLoad()
  if (!GetExecutionContext()) {
    MediaLoadingFailed(
        WebMediaPlayer::kNetworkStateFormatError,
        BuildElementErrorMessage("Player load failure: document has no frame"));
    return;
  }

  web_media_player_ = client_->CreateWebMediaPlayer(source, this);
  if (!web_media_player_) {
    MediaLoadingFailed(WebMediaPlayer::kNetworkStateFormatError,
                       BuildElementErrorMessage(
                           "Player load failure: error creating media player"));
    return;
  }

  // Make sure if we create/re-create the WebMediaPlayer that we update our
  // wrapper.
  audio_source_provider_.Wrap(web_media_player_->GetAudioSourceProvider());
  web_media_player_->SetVolume(EffectiveMediaVolume());

  web_media_player_->SetPoster(PosterImageURL());

  web_media_player_->SetPreload(EffectivePreloadType());

  web_media_player_->Load(GetLoadType(), source, CorsMode());

  web_media_player_->BecameDominantVisibleContent(mostly_filling_viewport_);
}

void MediaPlayer::SetPlayerPreload() {
  if (web_media_player_)
    web_media_player_->SetPreload(EffectivePreloadType());

  if (LoadIsDeferred() &&
      EffectivePreloadType() != WebMediaPlayer::kPreloadNone)
    StartDeferredLoad();
}

bool MediaPlayer::LoadIsDeferred() const {
  return deferred_load_state_ != kNotDeferred;
}

void MediaPlayer::DeferLoad() {
  // This implements the "optional" step 4 from the resource fetch algorithm
  // "If mode is remote".
  DCHECK(!deferred_load_timer_.IsActive());
  DCHECK_EQ(deferred_load_state_, kNotDeferred);
  // 1. Set the networkState to NETWORK_IDLE.
  // 2. Queue a task to fire a simple event named suspend at the element.
  ChangeNetworkStateFromLoadingToIdle();
  // 3. Queue a task to set the element's delaying-the-load-event
  // flag to false. This stops delaying the load event.
  deferred_load_timer_.StartOneShot(0, BLINK_FROM_HERE);
  // 4. Wait for the task to be run.
  deferred_load_state_ = kWaitingForStopDelayingLoadEventTask;
  // Continued in executeDeferredLoad().
}

void MediaPlayer::CancelDeferredLoad() {
  deferred_load_timer_.Stop();
  deferred_load_state_ = kNotDeferred;
}

void MediaPlayer::ExecuteDeferredLoad() {
  DCHECK_GE(deferred_load_state_, kWaitingForTrigger);

  // resource fetch algorithm step 4 - continued from deferLoad().

  // 5. Wait for an implementation-defined event (e.g. the user requesting that
  // the media element begin playback).  This is assumed to be whatever 'event'
  // ended up calling this method.
  CancelDeferredLoad();
  // 6. Set the element's delaying-the-load-event flag back to true (this
  // delays the load event again, in case it hasn't been fired yet).
  SetShouldDelayLoadEvent(true);
  // 7. Set the networkState to NETWORK_LOADING.
  SetNetworkState(kNetworkLoading);

  StartProgressEventTimer();

  StartPlayerLoad();
}

void MediaPlayer::StartDeferredLoad() {
  if (deferred_load_state_ == kWaitingForTrigger) {
    ExecuteDeferredLoad();
    return;
  }
  if (deferred_load_state_ == kExecuteOnStopDelayingLoadEventTask)
    return;
  DCHECK_EQ(deferred_load_state_, kWaitingForStopDelayingLoadEventTask);
  deferred_load_state_ = kExecuteOnStopDelayingLoadEventTask;
}

void MediaPlayer::DeferredLoadTimerFired(TimerBase*) {
  SetShouldDelayLoadEvent(false);

  if (deferred_load_state_ == kExecuteOnStopDelayingLoadEventTask) {
    ExecuteDeferredLoad();
    return;
  }
  DCHECK_EQ(deferred_load_state_, kWaitingForStopDelayingLoadEventTask);
  deferred_load_state_ = kWaitingForTrigger;
}

WebMediaPlayer::LoadType MediaPlayer::GetLoadType() const {
  if (media_source_)
    return WebMediaPlayer::kLoadTypeMediaSource;

  if (src_object_ ||
      (!current_src_.IsNull() && IsMediaStreamURL(current_src_.GetString())))
    return WebMediaPlayer::kLoadTypeMediaStream;

  return WebMediaPlayer::kLoadTypeURL;
}

bool MediaPlayer::IsSafeToLoadURL(const KURL& url,
                                  InvalidURLAction action_if_invalid) {
  if (!url.IsValid()) {
    BLINK_MEDIA_LOG << "isSafeToLoadURL(" << (void*)this << ", "
                    << UrlForLoggingMedia(url)
                    << ") -> FALSE because url is invalid";
    return false;
  }

  if (!GetExecutionContext() ||
      !GetExecutionContext()->GetSecurityOrigin()->CanDisplay(url)) {
    if (action_if_invalid == kComplain) {
      GetExecutionContext()->AddConsoleMessage(ConsoleMessage::Create(
          kSecurityMessageSource, kErrorMessageLevel,
          "Not allowed to load local resource: " + url.ElidedString()));
    }
    BLINK_MEDIA_LOG << "isSafeToLoadURL(" << (void*)this << ", "
                    << UrlForLoggingMedia(url)
                    << ") -> FALSE rejected by SecurityOrigin";
    return false;
  }

  if (!GetExecutionContext()->GetContentSecurityPolicy()->AllowMediaFromSource(
          url)) {
    BLINK_MEDIA_LOG << "isSafeToLoadURL(" << (void*)this << ", "
                    << UrlForLoggingMedia(url)
                    << ") -> rejected by Content Security Policy";
    return false;
  }

  return true;
}

bool MediaPlayer::IsMediaDataCORSSameOrigin(SecurityOrigin* origin) const {
  // hasSingleSecurityOrigin() tells us whether the origin in the src is
  // the same as the actual request (i.e. after redirect).
  // didPassCORSAccessCheck() means it was a successful CORS-enabled fetch
  // (vs. non-CORS-enabled or failed).
  // taintsCanvas() does checkAccess() on the URL plus allow data sources,
  // to ensure that it is not a URL that requires CORS (basically same
  // origin).
  return HasSingleSecurityOrigin() &&
         ((GetWebMediaPlayer() &&
           GetWebMediaPlayer()->DidPassCORSAccessCheck()) ||
          !origin->TaintsCanvas(currentSrc()));
}

bool MediaPlayer::IsInCrossOriginFrame() const {
  return false;
}

void MediaPlayer::StartProgressEventTimer() {
  if (progress_event_timer_.IsActive())
    return;

  previous_progress_time_ = WTF::CurrentTime();
  // 350ms is not magic, it is in the spec!
  progress_event_timer_.StartRepeating(0.350, BLINK_FROM_HERE);
}

void MediaPlayer::WaitForSourceChange() {
  BLINK_MEDIA_LOG << "waitForSourceChange(" << (void*)this << ")";

  StopPeriodicTimers();
  load_state_ = kWaitingForSource;

  // 6.17 - Waiting: Set the element's networkState attribute to the
  // NETWORK_NO_SOURCE value
  SetNetworkState(kNetworkNoSource);

  // 6.18 - Set the element's delaying-the-load-event flag to false. This stops
  // delaying the load event.
  SetShouldDelayLoadEvent(false);

  UpdateDisplayState();
}

void MediaPlayer::NoneSupported(const String& message) {
  BLINK_MEDIA_LOG << "NoneSupported(" << (void*)this << ", message='" << message
                  << "')";

  StopPeriodicTimers();
  load_state_ = kWaitingForSource;
  current_source_node_ = nullptr;

  // 4.8.12.5
  // The dedicated media source failure steps are the following steps:

  // 1 - Set the error attribute to a new MediaError object whose code attribute
  // is set to MEDIA_ERR_SRC_NOT_SUPPORTED.
  error_ = MediaError::Create(MediaError::kMediaErrSrcNotSupported, message);

  // 3 - Set the element's networkState attribute to the NETWORK_NO_SOURCE
  // value.
  SetNetworkState(kNetworkNoSource);

  // 4 - Set the element's show poster flag to true.
  UpdateDisplayState();

  // 5 - Fire a simple event named error at the media element.
  ScheduleEvent(EventTypeNames::error);

  // 6 - Reject pending play promises with NotSupportedError.
  ScheduleRejectPlayPromises(kNotSupportedError);

  CloseMediaSource();

  // 7 - Set the element's delaying-the-load-event flag to false. This stops
  // delaying the load event.
  SetShouldDelayLoadEvent(false);
}

void MediaPlayer::MediaEngineError(MediaError* err) {
  DCHECK_GE(ready_state_, kHaveMetadata);
  BLINK_MEDIA_LOG << "mediaEngineError(" << (void*)this << ", "
                  << static_cast<int>(err->code()) << ")";

  // 1 - The user agent should cancel the fetching process.
  StopPeriodicTimers();
  load_state_ = kWaitingForSource;

  // 2 - Set the error attribute to a new MediaError object whose code attribute
  // is set to MEDIA_ERR_NETWORK/MEDIA_ERR_DECODE.
  error_ = err;

  // 3 - Queue a task to fire a simple event named error at the media element.
  ScheduleEvent(EventTypeNames::error);

  // 4 - Set the element's networkState attribute to the NETWORK_IDLE value.
  SetNetworkState(kNetworkIdle);

  // 5 - Set the element's delaying-the-load-event flag to false. This stops
  // delaying the load event.
  SetShouldDelayLoadEvent(false);

  // 6 - Abort the overall resource selection algorithm.
  current_source_node_ = nullptr;
}

void MediaPlayer::CancelPendingEventsAndCallbacks() {
  BLINK_MEDIA_LOG << "cancelPendingEventsAndCallbacks(" << (void*)this << ")";
  async_event_queue_->CancelAllEvents();
}

void MediaPlayer::NetworkStateChanged() {
  SetNetworkState(GetWebMediaPlayer()->GetNetworkState());
}

void MediaPlayer::MediaLoadingFailed(WebMediaPlayer::NetworkState error,
                                     const String& message) {
  BLINK_MEDIA_LOG << "MediaLoadingFailed(" << (void*)this << ", "
                  << static_cast<int>(error) << ", message='" << message
                  << "')";

  StopPeriodicTimers();

  // If we failed while trying to load a <source> element, the movie was never
  // parsed, and there are more <source> children, schedule the next one
  if (ready_state_ < kHaveMetadata &&
      load_state_ == kLoadingFromSourceElement) {
    // resource selection algorithm
    // Step 9.Otherwise.9 - Failed with elements: Queue a task, using the DOM
    // manipulation task source, to fire a simple event named error at the
    // candidate element.
    if (current_source_node_) {
      current_source_node_->ScheduleErrorEvent();
    } else {
      BLINK_MEDIA_LOG << "mediaLoadingFailed(" << (void*)this
                      << ") - error event not sent, <source> was removed";
    }

    BLINK_MEDIA_LOG << "mediaLoadingFailed(" << (void*)this
                    << ") - no more <source> elements, waiting";
    WaitForSourceChange();
    return;
  }

  if (error == WebMediaPlayer::kNetworkStateNetworkError &&
      ready_state_ >= kHaveMetadata) {
    MediaEngineError(MediaError::Create(MediaError::kMediaErrNetwork, message));
  } else if (error == WebMediaPlayer::kNetworkStateDecodeError) {
    MediaEngineError(MediaError::Create(MediaError::kMediaErrDecode, message));
  } else if ((error == WebMediaPlayer::kNetworkStateFormatError ||
              error == WebMediaPlayer::kNetworkStateNetworkError) &&
             load_state_ == kLoadingFromSrcAttr) {
    if (message.IsEmpty()) {
      // Generate a more meaningful error message to differentiate the two types
      // of MEDIA_SRC_ERR_NOT_SUPPORTED.
      NoneSupported(BuildElementErrorMessage(
          error == WebMediaPlayer::kNetworkStateFormatError ? "Format error"
                                                            : "Network error"));
    } else {
      NoneSupported(message);
    }
  }

  UpdateDisplayState();
}

void MediaPlayer::SetNetworkState(WebMediaPlayer::NetworkState state) {
  BLINK_MEDIA_LOG << "setNetworkState(" << (void*)this << ", "
                  << static_cast<int>(state) << ") - current state is "
                  << static_cast<int>(network_state_);

  if (state == WebMediaPlayer::kNetworkStateEmpty) {
    // Just update the cached state and leave, we can't do anything.
    SetNetworkState(kNetworkEmpty);
    return;
  }

  if (state == WebMediaPlayer::kNetworkStateFormatError ||
      state == WebMediaPlayer::kNetworkStateNetworkError ||
      state == WebMediaPlayer::kNetworkStateDecodeError) {
    MediaLoadingFailed(state, web_media_player_->GetErrorMessage());
    return;
  }

  if (state == WebMediaPlayer::kNetworkStateIdle) {
    if (network_state_ > kNetworkIdle) {
      ChangeNetworkStateFromLoadingToIdle();
      SetShouldDelayLoadEvent(false);
    } else {
      SetNetworkState(kNetworkIdle);
    }
  }

  if (state == WebMediaPlayer::kNetworkStateLoading) {
    if (network_state_ < kNetworkLoading || network_state_ == kNetworkNoSource)
      StartProgressEventTimer();
    SetNetworkState(kNetworkLoading);
  }

  if (state == WebMediaPlayer::kNetworkStateLoaded) {
    if (network_state_ != kNetworkIdle)
      ChangeNetworkStateFromLoadingToIdle();
  }
}

void MediaPlayer::ChangeNetworkStateFromLoadingToIdle() {
  progress_event_timer_.Stop();

  // Schedule one last progress event so we guarantee that at least one is fired
  // for files that load very quickly.
  if (GetWebMediaPlayer() && GetWebMediaPlayer()->DidLoadingProgress())
    ScheduleEvent(EventTypeNames::progress);
  ScheduleEvent(EventTypeNames::suspend);
  SetNetworkState(kNetworkIdle);
}

void MediaPlayer::ReadyStateChanged() {
  SetReadyState(static_cast<ReadyState>(GetWebMediaPlayer()->GetReadyState()));
}

void MediaPlayer::SetReadyState(ReadyState state) {
  BLINK_MEDIA_LOG << "setReadyState(" << (void*)this << ", "
                  << static_cast<int>(state) << ") - current state is "
                  << static_cast<int>(ready_state_);

  // Set "wasPotentiallyPlaying" BEFORE updating ready_state_,
  // potentiallyPlaying() uses it
  bool was_potentially_playing = PotentiallyPlaying();

  ReadyState old_state = ready_state_;
  ReadyState new_state = state;

  bool tracks_are_ready = true;

  if (new_state == old_state && tracks_are_ready_ == tracks_are_ready)
    return;

  tracks_are_ready_ = tracks_are_ready;

  if (tracks_are_ready) {
    ready_state_ = new_state;
  } else {
    // If a media file has text tracks the readyState may not progress beyond
    // kHaveFutureData until the text tracks are ready, regardless of the state
    // of the media file.
    if (new_state <= kHaveMetadata)
      ready_state_ = new_state;
    else
      ready_state_ = kHaveCurrentData;
  }

  if (old_state > ready_state_maximum_)
    ready_state_maximum_ = old_state;

  if (network_state_ == kNetworkEmpty)
    return;

  if (seeking_) {
    // 4.8.12.9, step 9 note: If the media element was potentially playing
    // immediately before it started seeking, but seeking caused its readyState
    // attribute to change to a value lower than kHaveFutureData, then a waiting
    // will be fired at the element.
    if (was_potentially_playing && ready_state_ < kHaveFutureData)
      ScheduleEvent(EventTypeNames::waiting);

    // 4.8.12.9 steps 12-14
    if (ready_state_ >= kHaveCurrentData)
      FinishSeek();
  } else {
    if (was_potentially_playing && ready_state_ < kHaveFutureData) {
      // Force an update to official playback position. Automatic updates from
      // currentPlaybackPosition() will be blocked while ready_state_ remains
      // < kHaveFutureData. This blocking is desired after 'waiting' has been
      // fired, but its good to update it one final time to accurately reflect
      // media time at the moment we ran out of data to play.
      SetOfficialPlaybackPosition(CurrentPlaybackPosition());

      // 4.8.12.8
      ScheduleTimeupdateEvent(false);
      ScheduleEvent(EventTypeNames::waiting);
    }
  }

  // Once enough of the media data has been fetched to determine the duration of
  // the media resource, its dimensions, and other metadata...
  if (ready_state_ >= kHaveMetadata && old_state < kHaveMetadata) {
    CreatePlaceholderTracksIfNecessary();

    SelectInitialTracksIfNecessary();

    MediaFragmentURIParser fragment_parser(current_src_);
    fragment_end_time_ = fragment_parser.EndTime();

    // Set the current playback position and the official playback position to
    // the earliest possible position.
    SetOfficialPlaybackPosition(EarliestPossiblePosition());

    duration_ = web_media_player_->Duration();
    ScheduleEvent(EventTypeNames::durationchange);
    ScheduleEvent(EventTypeNames::loadedmetadata);

    bool jumped = false;
    if (default_playback_start_position_ > 0) {
      Seek(default_playback_start_position_);
      jumped = true;
    }
    default_playback_start_position_ = 0;

    double initial_playback_position = fragment_parser.StartTime();
    if (std::isnan(initial_playback_position))
      initial_playback_position = 0;

    if (!jumped && initial_playback_position > 0) {
      UseCounter::Count(GetExecutionContext(),
                        WebFeature::kHTMLMediaElementSeekToFragmentStart);
      Seek(initial_playback_position);
      jumped = true;
    }
  }

  bool should_update_display_state = false;

  if (ready_state_ >= kHaveCurrentData && old_state < kHaveCurrentData &&
      !have_fired_loaded_data_) {
    // Force an update to official playback position to catch non-zero start
    // times that were not known at kHaveMetadata, but are known now that the
    // first packets have been demuxed.
    SetOfficialPlaybackPosition(CurrentPlaybackPosition());

    have_fired_loaded_data_ = true;
    should_update_display_state = true;
    ScheduleEvent(EventTypeNames::loadeddata);
    SetShouldDelayLoadEvent(false);
  }

  bool is_potentially_playing = PotentiallyPlaying();
  if (ready_state_ == kHaveFutureData && old_state <= kHaveCurrentData &&
      tracks_are_ready) {
    ScheduleEvent(EventTypeNames::canplay);
    if (is_potentially_playing)
      ScheduleNotifyPlaying();
    should_update_display_state = true;
  }

  if (ready_state_ == kHaveEnoughData && old_state < kHaveEnoughData &&
      tracks_are_ready) {
    if (old_state <= kHaveCurrentData) {
      ScheduleEvent(EventTypeNames::canplay);
      if (is_potentially_playing)
        ScheduleNotifyPlaying();
    }

    ScheduleEvent(EventTypeNames::canplaythrough);

    should_update_display_state = true;
  }

  if (should_update_display_state)
    UpdateDisplayState();

  UpdatePlayState();
}

void MediaPlayer::ProgressEventTimerFired(TimerBase*) {
  if (network_state_ != kNetworkLoading)
    return;

  double time = WTF::CurrentTime();
  double timedelta = time - previous_progress_time_;

  if (GetWebMediaPlayer() && GetWebMediaPlayer()->DidLoadingProgress()) {
    ScheduleEvent(EventTypeNames::progress);
    previous_progress_time_ = time;
    sent_stalled_event_ = false;
  } else if (timedelta > 3.0 && !sent_stalled_event_) {
    ScheduleEvent(EventTypeNames::stalled);
    sent_stalled_event_ = true;
    SetShouldDelayLoadEvent(false);
  }
}

void MediaPlayer::AddPlayedRange(double start, double end) {
  BLINK_MEDIA_LOG << "addPlayedRange(" << (void*)this << ", " << start << ", "
                  << end << ")";
  if (!played_time_ranges_)
    played_time_ranges_ = TimeRanges::Create();
  played_time_ranges_->Add(start, end);
}

bool MediaPlayer::SupportsSave() const {
  return GetWebMediaPlayer() && GetWebMediaPlayer()->SupportsSave();
}

void MediaPlayer::SetIgnorePreloadNone() {
  BLINK_MEDIA_LOG << "setIgnorePreloadNone(" << (void*)this << ")";
  ignore_preload_none_ = true;
  SetPlayerPreload();
}

void MediaPlayer::Seek(double time) {
  BLINK_MEDIA_LOG << "seek(" << (void*)this << ", " << time << ")";

  // 2 - If the media element's readyState is HAVE_NOTHING, abort these steps.
  // FIXME: remove web_media_player_ check once we figure out how
  // web_media_player_ is going out of sync with readystate.
  // web_media_player_ is cleared but readystate is not set to HAVE_NOTHING.
  if (!web_media_player_ || ready_state_ == kHaveNothing)
    return;

  // Ignore preload none and start load if necessary.
  SetIgnorePreloadNone();

  // Get the current time before setting seeking_, last_seek_time_ is returned
  // once it is set.
  double now = currentTime();

  // 3 - If the element's seeking IDL attribute is true, then another instance
  // of this algorithm is already running. Abort that other instance of the
  // algorithm without waiting for the step that it is running to complete.
  // Nothing specific to be done here.

  // 4 - Set the seeking IDL attribute to true.
  // The flag will be cleared when the engine tells us the time has actually
  // changed.
  seeking_ = true;

  // 6 - If the new playback position is later than the end of the media
  // resource, then let it be the end of the media resource instead.
  time = std::min(time, duration());

  // 7 - If the new playback position is less than the earliest possible
  // position, let it be that position instead.
  time = std::max(time, EarliestPossiblePosition());

  // Ask the media engine for the time value in the movie's time scale before
  // comparing with current time. This is necessary because if the seek time is
  // not equal to currentTime but the delta is less than the movie's time scale,
  // we will ask the media engine to "seek" to the current movie time, which may
  // be a noop and not generate a timechanged callback. This means seeking_
  // will never be cleared and we will never fire a 'seeked' event.
  double media_time = GetWebMediaPlayer()->MediaTimeForTimeValue(time);
  if (time != media_time) {
    BLINK_MEDIA_LOG << "seek(" << (void*)this << ", " << time
                    << ") - media timeline equivalent is " << media_time;
    time = media_time;
  }

  // 8 - If the (possibly now changed) new playback position is not in one of
  // the ranges given in the seekable attribute, then let it be the position in
  // one of the ranges given in the seekable attribute that is the nearest to
  // the new playback position. ... If there are no ranges given in the seekable
  // attribute then set the seeking IDL attribute to false and abort these
  // steps.
  TimeRanges* seekable_ranges = seekable();

  if (!seekable_ranges->length()) {
    seeking_ = false;
    return;
  }
  time = seekable_ranges->Nearest(time, now);

  if (playing_ && last_seek_time_ < now)
    AddPlayedRange(last_seek_time_, now);

  last_seek_time_ = time;

  // 10 - Queue a task to fire a simple event named seeking at the element.
  ScheduleEvent(EventTypeNames::seeking);

  // 11 - Set the current playback position to the given new playback position.
  GetWebMediaPlayer()->Seek(time);

  // 14-17 are handled, if necessary, when the engine signals a readystate
  // change or otherwise satisfies seek completion and signals a time change.
}

void MediaPlayer::FinishSeek() {
  BLINK_MEDIA_LOG << "finishSeek(" << (void*)this << ")";

  // 14 - Set the seeking IDL attribute to false.
  seeking_ = false;

  // Force an update to officialPlaybackPosition. Periodic updates generally
  // handle this, but may be skipped paused or waiting for data.
  SetOfficialPlaybackPosition(CurrentPlaybackPosition());

  // 16 - Queue a task to fire a simple event named timeupdate at the element.
  ScheduleTimeupdateEvent(false);

  // 17 - Queue a task to fire a simple event named seeked at the element.
  ScheduleEvent(EventTypeNames::seeked);

  SetDisplayMode(kVideo);
}

MediaPlayer::ReadyState MediaPlayer::getReadyState() const {
  return ready_state_;
}

bool MediaPlayer::HasVideo() const {
  return GetWebMediaPlayer() && GetWebMediaPlayer()->HasVideo();
}

bool MediaPlayer::HasAudio() const {
  return GetWebMediaPlayer() && GetWebMediaPlayer()->HasAudio();
}

bool MediaPlayer::seeking() const {
  return seeking_;
}

// https://www.w3.org/TR/html51/semantics-embedded-content.html#earliest-possible-position
// The earliest possible position is not explicitly exposed in the API; it
// corresponds to the start time of the first range in the seekable attribute’s
// TimeRanges object, if any, or the current playback position otherwise.
double MediaPlayer::EarliestPossiblePosition() const {
  TimeRanges* seekable_ranges = seekable();
  if (seekable_ranges && seekable_ranges->length() > 0)
    return seekable_ranges->start(0, ASSERT_NO_EXCEPTION);

  return CurrentPlaybackPosition();
}

double MediaPlayer::CurrentPlaybackPosition() const {
  // "Official" playback position won't take updates from "current" playback
  // position until ready_state_ > kHaveMetadata, but other callers (e.g.
  // pauseInternal) may still request currentPlaybackPosition at any time.
  // From spec: "Media elements have a current playback position, which must
  // initially (i.e., in the absence of media data) be zero seconds."
  if (ready_state_ == kHaveNothing)
    return 0;

  if (GetWebMediaPlayer())
    return GetWebMediaPlayer()->CurrentTime();

  if (ready_state_ >= kHaveMetadata) {
    BLINK_MEDIA_LOG
        << __func__ << " readyState = " << ready_state_
        << " but no webMediaPlayer to provide currentPlaybackPosition";
  }

  return 0;
}

double MediaPlayer::OfficialPlaybackPosition() const {
  // Hold updates to official playback position while paused or waiting for more
  // data. The underlying media player may continue to make small advances in
  // currentTime (e.g. as samples in the last rendered audio buffer are played
  // played out), but advancing currentTime while paused/waiting sends a mixed
  // signal about the state of playback.
  bool waiting_for_data = ready_state_ <= kHaveCurrentData;
  if (official_playback_position_needs_update_ && !paused_ &&
      !waiting_for_data) {
    SetOfficialPlaybackPosition(CurrentPlaybackPosition());
  }

#if LOG_OFFICIAL_TIME_STATUS
  static const double kMinCachedDeltaForWarning = 0.01;
  double delta =
      std::abs(official_playback_position_ - CurrentPlaybackPosition());
  if (delta > kMinCachedDeltaForWarning) {
    BLINK_MEDIA_LOG << "CurrentTime(" << (void*)this
                    << ") - WARNING, cached time is " << delta
                    << "seconds off of media time when paused/waiting";
  }
#endif

  return official_playback_position_;
}

void MediaPlayer::SetOfficialPlaybackPosition(double position) const {
#if LOG_OFFICIAL_TIME_STATUS
  BLINK_MEDIA_LOG << "SetOfficialPlaybackPosition(" << (void*)this
                  << ") was:" << official_playback_position_
                  << " now:" << position;
#endif

  // Internal player position may advance slightly beyond duration because
  // many files use imprecise duration. Clamp official position to duration when
  // known. Duration may be unknown when readyState < HAVE_METADATA.
  official_playback_position_ =
      std::isnan(duration()) ? position : std::min(duration(), position);

  if (official_playback_position_ != position) {
    BLINK_MEDIA_LOG << "setOfficialPlaybackPosition(" << (void*)this
                    << ") position:" << position
                    << " truncated to duration:" << official_playback_position_;
  }

  // Once set, official playback position should hold steady until the next
  // stable state. We approximate this by using a microtask to mark the
  // need for an update after the current (micro)task has completed. When
  // needed, the update is applied in the next call to
  // officialPlaybackPosition().
  official_playback_position_needs_update_ = false;
  Microtask::EnqueueMicrotask(
      WTF::Bind(&MediaPlayer::RequireOfficialPlaybackPositionUpdate,
                WrapWeakPersistent(this)));
}

void MediaPlayer::RequireOfficialPlaybackPositionUpdate() const {
  official_playback_position_needs_update_ = true;
}

double MediaPlayer::currentTime() const {
  if (default_playback_start_position_)
    return default_playback_start_position_;

  if (seeking_) {
    BLINK_MEDIA_LOG << "currentTime(" << (void*)this
                    << ") - seeking, returning " << last_seek_time_;
    return last_seek_time_;
  }

  return OfficialPlaybackPosition();
}

void MediaPlayer::setCurrentTime(double time) {
  // If the media element's readyState is kHaveNothing, then set the default
  // playback start position to that time.
  if (ready_state_ == kHaveNothing) {
    default_playback_start_position_ = time;
    return;
  }

  Seek(time);
}

double MediaPlayer::duration() const {
  return duration_;
}

bool MediaPlayer::paused() const {
  return paused_;
}

double MediaPlayer::defaultPlaybackRate() const {
  return default_playback_rate_;
}

void MediaPlayer::setDefaultPlaybackRate(double rate) {
  if (default_playback_rate_ == rate)
    return;

  default_playback_rate_ = rate;
  ScheduleEvent(EventTypeNames::ratechange);
}

double MediaPlayer::playbackRate() const {
  return playback_rate_;
}

void MediaPlayer::setPlaybackRate(double rate) {
  BLINK_MEDIA_LOG << "setPlaybackRate(" << (void*)this << ", " << rate << ")";

  if (playback_rate_ != rate) {
    playback_rate_ = rate;
    ScheduleEvent(EventTypeNames::ratechange);
  }

  UpdatePlaybackRate();
}

MediaPlayer::DirectionOfPlayback MediaPlayer::GetDirectionOfPlayback() const {
  return playback_rate_ >= 0 ? kForward : kBackward;
}

void MediaPlayer::UpdatePlaybackRate() {
  // FIXME: remove web_media_player_ check once we figure out how
  // web_media_player_ is going out of sync with readystate.
  // web_media_player_ is cleared but readystate is not set to kHaveNothing.
  if (web_media_player_ && PotentiallyPlaying())
    GetWebMediaPlayer()->SetRate(playbackRate());
}

bool MediaPlayer::ended() const {
  // 4.8.12.8 Playing the media resource
  // The ended attribute must return true if the media element has ended
  // playback and the direction of playback is forwards, and false otherwise.
  return EndedPlayback() && GetDirectionOfPlayback() == kForward;
}

bool MediaPlayer::autoplay() const {
  return autoplay_;
}

void MediaPlayer::setAutoplay(bool b) {
  autoplay_ = b;
}

String MediaPlayer::preload() const {
  return PreloadTypeToString(PreloadType());
}

void MediaPlayer::setPreload(const String& preload) {
  BLINK_MEDIA_LOG << "setPreload(" << (void*)this << ", " << preload << ")";
  preload_ = preload;
}

WebMediaPlayer::Preload MediaPlayer::PreloadType() const {
  const String& preload = preload_;
  if (DeprecatedEqualIgnoringCase(preload, "none")) {
    UseCounter::Count(GetExecutionContext(),
                      WebFeature::kHTMLMediaElementPreloadNone);
    return WebMediaPlayer::kPreloadNone;
  }

  if (DeprecatedEqualIgnoringCase(preload, "metadata")) {
    UseCounter::Count(GetExecutionContext(),
                      WebFeature::kHTMLMediaElementPreloadMetadata);
    return WebMediaPlayer::kPreloadMetaData;
  }

  // Force preload to 'metadata' on cellular connections.
  if (GetNetworkStateNotifier().IsCellularConnectionType()) {
    UseCounter::Count(GetExecutionContext(),
                      WebFeature::kHTMLMediaElementPreloadForcedMetadata);
    return WebMediaPlayer::kPreloadMetaData;
  }

  if (DeprecatedEqualIgnoringCase(preload, "auto")) {
    UseCounter::Count(GetExecutionContext(),
                      WebFeature::kHTMLMediaElementPreloadAuto);
    return WebMediaPlayer::kPreloadAuto;
  }

  // "The attribute's missing value default is user-agent defined, though the
  // Metadata state is suggested as a compromise between reducing server load
  // and providing an optimal user experience."

  // The spec does not define an invalid value default:
  // https://www.w3.org/Bugs/Public/show_bug.cgi?id=28950

  // TODO(foolip): Try to make "metadata" the default preload state:
  // https://crbug.com/310450
  UseCounter::Count(GetExecutionContext(),
                    WebFeature::kHTMLMediaElementPreloadDefault);
  return WebMediaPlayer::kPreloadAuto;
}

String MediaPlayer::EffectivePreload() const {
  return PreloadTypeToString(EffectivePreloadType());
}

WebMediaPlayer::Preload MediaPlayer::EffectivePreloadType() const {
  if (autoplay())
    return WebMediaPlayer::kPreloadAuto;

  WebMediaPlayer::Preload preload = PreloadType();
  if (ignore_preload_none_ && preload == WebMediaPlayer::kPreloadNone)
    return WebMediaPlayer::kPreloadMetaData;

  return preload;
}

ScriptPromise MediaPlayer::playForBindings(ScriptState* script_state) {
  // We have to share the same logic for internal and external callers. The
  // internal callers do not want to receive a Promise back but when ::play()
  // is called, |play_promise_resolvers_| needs to be populated. What this code
  // does is to populate |play_promise_resolvers_| before calling ::play() and
  // remove the Promise if ::play() failed.
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  play_promise_resolvers_.push_back(resolver);

  Nullable<ExceptionCode> code = Play();
  if (!code.IsNull()) {
    DCHECK(!play_promise_resolvers_.IsEmpty());
    play_promise_resolvers_.pop_back();

    String message;
    switch (code.Get()) {
      case kNotAllowedError:
        message = "play() can only be initiated by a user gesture.";
        RecordPlayPromiseRejected(
            PlayPromiseRejectReason::kFailedAutoplayPolicy);
        break;
      case kNotSupportedError:
        message = "The element has no supported sources.";
        RecordPlayPromiseRejected(PlayPromiseRejectReason::kNoSupportedSources);
        break;
      default:
        NOTREACHED();
    }
    resolver->Reject(DOMException::Create(code.Get(), message));
    return promise;
  }

  return promise;
}

Nullable<ExceptionCode> MediaPlayer::Play() {
  BLINK_MEDIA_LOG << "play(" << (void*)this << ")";

  if (error_ && error_->code() == MediaError::kMediaErrSrcNotSupported)
    return kNotSupportedError;

  PlayInternal();

  return nullptr;
}

void MediaPlayer::PlayInternal() {
  BLINK_MEDIA_LOG << "playInternal(" << (void*)this << ")";

  // 4.8.12.8. Playing the media resource
  if (network_state_ == kNetworkEmpty)
    InvokeResourceSelectionAlgorithm();

  // Generally "ended" and "looping" are exclusive. Here, the loop attribute
  // is ignored to seek back to start in case loop was set after playback
  // ended. See http://crbug.com/364442
  if (EndedPlayback(LoopCondition::kIgnored))
    Seek(0);

  if (paused_) {
    paused_ = false;
    ScheduleEvent(EventTypeNames::play);

    if (ready_state_ <= kHaveCurrentData)
      ScheduleEvent(EventTypeNames::waiting);
    else if (ready_state_ >= kHaveFutureData)
      ScheduleNotifyPlaying();
  } else if (ready_state_ >= kHaveFutureData) {
    ScheduleResolvePlayPromises();
  }

  can_autoplay_ = false;

  SetIgnorePreloadNone();
  UpdatePlayState();
}

void MediaPlayer::pause() {
  BLINK_MEDIA_LOG << "pause(" << (void*)this << ")";

  PauseInternal();
}

void MediaPlayer::PauseInternal() {
  BLINK_MEDIA_LOG << "pauseInternal(" << (void*)this << ")";

  if (network_state_ == kNetworkEmpty)
    InvokeResourceSelectionAlgorithm();

  can_autoplay_ = false;

  if (!paused_) {
    paused_ = true;
    ScheduleTimeupdateEvent(false);
    ScheduleEvent(EventTypeNames::pause);

    // Force an update to official playback position. Automatic updates from
    // currentPlaybackPosition() will be blocked while paused_ = true. This
    // blocking is desired while paused, but its good to update it one final
    // time to accurately reflect movie time at the moment we paused.
    SetOfficialPlaybackPosition(CurrentPlaybackPosition());

    ScheduleRejectPlayPromises(kAbortError);
  }

  UpdatePlayState();
}

void MediaPlayer::RequestRemotePlayback() {
  if (GetWebMediaPlayer())
    GetWebMediaPlayer()->RequestRemotePlayback();
}

void MediaPlayer::RequestRemotePlaybackControl() {
  if (GetWebMediaPlayer())
    GetWebMediaPlayer()->RequestRemotePlaybackControl();
}

void MediaPlayer::RequestRemotePlaybackStop() {
  if (GetWebMediaPlayer())
    GetWebMediaPlayer()->RequestRemotePlaybackStop();
}

void MediaPlayer::CloseMediaSource() {
  if (!media_source_)
    return;

  media_source_->Close();
  media_source_ = nullptr;
}

bool MediaPlayer::loop() const {
  return loop_;
}

void MediaPlayer::setLoop(bool b) {
  BLINK_MEDIA_LOG << "setLoop(" << (void*)this << ", " << BoolString(b) << ")";
  loop_ = b;
}

bool MediaPlayer::ShouldShowControls(
    const RecordMetricsBehavior record_metrics) const {
  return false;
}

double MediaPlayer::volume() const {
  return volume_;
}

void MediaPlayer::setVolume(double vol, ExceptionState& exception_state) {
  BLINK_MEDIA_LOG << "setVolume(" << (void*)this << ", " << vol << ")";

  if (volume_ == vol)
    return;

  if (vol < 0.0f || vol > 1.0f) {
    exception_state.ThrowDOMException(
        kIndexSizeError,
        ExceptionMessages::IndexOutsideRange(
            "volume", vol, 0.0, ExceptionMessages::kInclusiveBound, 1.0,
            ExceptionMessages::kInclusiveBound));
    return;
  }

  volume_ = vol;

  if (GetWebMediaPlayer())
    GetWebMediaPlayer()->SetVolume(EffectiveMediaVolume());
  ScheduleEvent(EventTypeNames::volumechange);
}

bool MediaPlayer::muted() const {
  return muted_;
}

void MediaPlayer::setMuted(bool muted) {
  BLINK_MEDIA_LOG << "setMuted(" << (void*)this << ", " << BoolString(muted)
                  << ")";

  if (muted_ == muted)
    return;

  muted_ = muted;

  ScheduleEvent(EventTypeNames::volumechange);

  // This is called after the volumechange event to make sure isAutoplayingMuted
  // returns the right value when webMediaPlayer receives the volume update.
  if (GetWebMediaPlayer())
    GetWebMediaPlayer()->SetVolume(EffectiveMediaVolume());
}

bool MediaPlayer::defaultMuted() const {
  return false;
}

void MediaPlayer::setDefaultMuted(bool) {}

double MediaPlayer::EffectiveMediaVolume() const {
  if (muted_)
    return 0;

  return volume_;
}

// The spec says to fire periodic timeupdate events (those sent while playing)
// every "15 to 250ms", we choose the slowest frequency
static const double kMaxTimeupdateEventFrequency = 0.25;

void MediaPlayer::StartPlaybackProgressTimer() {
  if (playback_progress_timer_.IsActive())
    return;

  previous_progress_time_ = WTF::CurrentTime();
  playback_progress_timer_.StartRepeating(kMaxTimeupdateEventFrequency,
                                          BLINK_FROM_HERE);
}

void MediaPlayer::PlaybackProgressTimerFired(TimerBase*) {
  if (!std::isnan(fragment_end_time_) && currentTime() >= fragment_end_time_ &&
      GetDirectionOfPlayback() == kForward) {
    fragment_end_time_ = std::numeric_limits<double>::quiet_NaN();
    if (!paused_) {
      UseCounter::Count(GetExecutionContext(),
                        WebFeature::kHTMLMediaElementPauseAtFragmentEnd);
      // changes paused to true and fires a simple event named pause at the
      // media element.
      PauseInternal();
    }
  }

  if (!seeking_)
    ScheduleTimeupdateEvent(true);

  if (!playbackRate())
    return;
}

void MediaPlayer::ScheduleTimeupdateEvent(bool periodic_event) {
  // Per spec, consult current playback position to check for changing time.
  double media_time = CurrentPlaybackPosition();
  double now = WTF::CurrentTime();

  bool have_not_recently_fired_timeupdate =
      (now - last_time_update_event_wall_time_) >= kMaxTimeupdateEventFrequency;
  bool media_time_has_progressed =
      media_time != last_time_update_event_media_time_;

  // Non-periodic timeupdate events must always fire as mandated by the spec,
  // otherwise we shouldn't fire duplicate periodic timeupdate events when the
  // movie time hasn't changed.
  if (!periodic_event ||
      (have_not_recently_fired_timeupdate && media_time_has_progressed)) {
    ScheduleEvent(EventTypeNames::timeupdate);
    last_time_update_event_wall_time_ = now;
    last_time_update_event_media_time_ = media_time;
  }
}

void MediaPlayer::TogglePlayState() {
  if (paused())
    Play();
  else
    pause();
}

KURL MediaPlayer::SelectNextSourceChild(String* content_type,
                                        InvalidURLAction action_if_invalid) {
  return KURL();
}

void MediaPlayer::TimeChanged() {
  BLINK_MEDIA_LOG << "timeChanged(" << (void*)this << ")";

  // 4.8.12.9 steps 12-14. Needed if no ReadyState change is associated with the
  // seek.
  if (seeking_ && ready_state_ >= kHaveCurrentData &&
      !GetWebMediaPlayer()->Seeking())
    FinishSeek();

  // Always call scheduleTimeupdateEvent when the media engine reports a time
  // discontinuity, it will only queue a 'timeupdate' event if we haven't
  // already posted one at the current movie time.
  ScheduleTimeupdateEvent(false);

  double now = CurrentPlaybackPosition();
  double dur = duration();

  // When the current playback position reaches the end of the media resource
  // when the direction of playback is forwards, then the user agent must follow
  // these steps:
  if (!std::isnan(dur) && dur && now >= dur &&
      GetDirectionOfPlayback() == kForward) {
    // If the media element has a loop attribute specified
    if (loop()) {
      //  then seek to the earliest possible position of the media resource and
      //  abort these steps.
      Seek(EarliestPossiblePosition());
    } else {
      // If the media element has still ended playback, and the direction of
      // playback is still forwards, and paused is false,
      if (!paused_) {
        // changes paused to true and fires a simple event named pause at the
        // media element.
        paused_ = true;
        ScheduleEvent(EventTypeNames::pause);
        ScheduleRejectPlayPromises(kAbortError);
      }
      // Queue a task to fire a simple event named ended at the media element.
      ScheduleEvent(EventTypeNames::ended);
    }
  }
  UpdatePlayState();
}

void MediaPlayer::DurationChanged() {
  BLINK_MEDIA_LOG << "durationChanged(" << (void*)this << ")";

  // durationChanged() is triggered by media player.
  CHECK(web_media_player_);
  double new_duration = web_media_player_->Duration();

  // If the duration is changed such that the *current playback position* ends
  // up being greater than the time of the end of the media resource, then the
  // user agent must also seek to the time of the end of the media resource.
  DurationChanged(new_duration, CurrentPlaybackPosition() > new_duration);
}

void MediaPlayer::DurationChanged(double duration, bool request_seek) {
  BLINK_MEDIA_LOG << "durationChanged(" << (void*)this << ", " << duration
                  << ", " << BoolString(request_seek) << ")";

  // Abort if duration unchanged.
  if (duration_ == duration)
    return;

  BLINK_MEDIA_LOG << "durationChanged(" << (void*)this << ") : " << duration_
                  << " -> " << duration;
  duration_ = duration;
  ScheduleEvent(EventTypeNames::durationchange);

  if (request_seek)
    Seek(duration);
}

void MediaPlayer::PlaybackStateChanged() {
  BLINK_MEDIA_LOG << "playbackStateChanged(" << (void*)this << ")";

  if (!GetWebMediaPlayer())
    return;

  if (GetWebMediaPlayer()->Paused())
    PauseInternal();
  else
    PlayInternal();
}

void MediaPlayer::RequestSeek(double time) {
  // The player is the source of this seek request.
  setCurrentTime(time);
}

void MediaPlayer::RemoteRouteAvailabilityChanged(
    WebRemotePlaybackAvailability availability) {
  if (RemotePlaybackClient() &&
      !RuntimeEnabledFeatures::NewRemotePlaybackPipelineEnabled()) {
    // The new remote playback pipeline is using the Presentation API for
    // remote playback device availability monitoring.
    RemotePlaybackClient()->AvailabilityChanged(availability);
  }
}

bool MediaPlayer::HasRemoteRoutes() const {
  // TODO(mlamouri): used by MediaControlsPainter; should be refactored out.
  return RemotePlaybackClient() &&
         RemotePlaybackClient()->RemotePlaybackAvailable();
}

void MediaPlayer::ConnectedToRemoteDevice() {
  playing_remotely_ = true;
  if (RemotePlaybackClient())
    RemotePlaybackClient()->StateChanged(WebRemotePlaybackState::kConnecting);
}

void MediaPlayer::DisconnectedFromRemoteDevice() {
  playing_remotely_ = false;
  if (RemotePlaybackClient())
    RemotePlaybackClient()->StateChanged(WebRemotePlaybackState::kDisconnected);
}

void MediaPlayer::CancelledRemotePlaybackRequest() {
  if (RemotePlaybackClient())
    RemotePlaybackClient()->PromptCancelled();
}

void MediaPlayer::RemotePlaybackStarted() {
  if (RemotePlaybackClient())
    RemotePlaybackClient()->StateChanged(WebRemotePlaybackState::kConnected);
}

void MediaPlayer::RemotePlaybackCompatibilityChanged(const WebURL&,
                                                     bool is_compatible) {}

bool MediaPlayer::HasSelectedVideoTrack() {
  DCHECK(RuntimeEnabledFeatures::BackgroundVideoTrackOptimizationEnabled());

  return false;
}

WebMediaPlayer::TrackId MediaPlayer::GetSelectedVideoTrackId() {
  DCHECK(RuntimeEnabledFeatures::BackgroundVideoTrackOptimizationEnabled());
  DCHECK(HasSelectedVideoTrack());

  return WebMediaPlayer::TrackId();
}

bool MediaPlayer::IsAutoplayingMuted() {
  return false;
}

// MediaPlayerPresentation methods
void MediaPlayer::Repaint() {
  if (web_layer_)
    web_layer_->Invalidate();

  UpdateDisplayState();
}

void MediaPlayer::SizeChanged() {
  BLINK_MEDIA_LOG << "sizeChanged(" << (void*)this << ")";

  DCHECK(HasVideo());  // "resize" makes no sense in absence of video.
}

TimeRanges* MediaPlayer::buffered() const {
  if (media_source_)
    return media_source_->Buffered();

  if (!GetWebMediaPlayer())
    return TimeRanges::Create();

  return TimeRanges::Create(GetWebMediaPlayer()->Buffered());
}

TimeRanges* MediaPlayer::played() {
  if (playing_) {
    double time = currentTime();
    if (time > last_seek_time_)
      AddPlayedRange(last_seek_time_, time);
  }

  if (!played_time_ranges_)
    played_time_ranges_ = TimeRanges::Create();

  return played_time_ranges_->Copy();
}

TimeRanges* MediaPlayer::seekable() const {
  if (!GetWebMediaPlayer())
    return TimeRanges::Create();

  if (media_source_)
    return media_source_->Seekable();

  return TimeRanges::Create(GetWebMediaPlayer()->Seekable());
}

bool MediaPlayer::PotentiallyPlaying() const {
  // "pausedToBuffer" means the media engine's rate is 0, but only because it
  // had to stop playing when it ran out of buffered data. A movie in this state
  // is "potentially playing", modulo the checks in couldPlayIfEnoughData().
  bool paused_to_buffer =
      ready_state_maximum_ >= kHaveFutureData && ready_state_ < kHaveFutureData;
  return (paused_to_buffer || ready_state_ >= kHaveFutureData) &&
         CouldPlayIfEnoughData();
}

bool MediaPlayer::CouldPlayIfEnoughData() const {
  return !paused() && !EndedPlayback() && !StoppedDueToErrors();
}

bool MediaPlayer::EndedPlayback(LoopCondition loop_condition) const {
  double dur = duration();
  if (std::isnan(dur))
    return false;

  // 4.8.12.8 Playing the media resource

  // A media element is said to have ended playback when the element's
  // readyState attribute is HAVE_METADATA or greater,
  if (ready_state_ < kHaveMetadata)
    return false;

  // and the current playback position is the end of the media resource and the
  // direction of playback is forwards, Either the media element does not have a
  // loop attribute specified,
  double now = CurrentPlaybackPosition();
  if (GetDirectionOfPlayback() == kForward) {
    return dur > 0 && now >= dur &&
           (loop_condition == LoopCondition::kIgnored || !loop());
  }

  // or the current playback position is the earliest possible position and the
  // direction of playback is backwards
  DCHECK_EQ(GetDirectionOfPlayback(), kBackward);
  return now <= EarliestPossiblePosition();
}

bool MediaPlayer::StoppedDueToErrors() const {
  if (ready_state_ >= kHaveMetadata && error_) {
    TimeRanges* seekable_ranges = seekable();
    if (!seekable_ranges->Contain(currentTime()))
      return true;
  }

  return false;
}

void MediaPlayer::UpdatePlayState() {
  bool is_playing = GetWebMediaPlayer() && !GetWebMediaPlayer()->Paused();
  bool should_be_playing = PotentiallyPlaying();

  BLINK_MEDIA_LOG << "updatePlayState(" << (void*)this
                  << ") - shouldBePlaying = " << BoolString(should_be_playing)
                  << ", isPlaying = " << BoolString(is_playing);

  if (should_be_playing) {
    SetDisplayMode(kVideo);

    if (!is_playing) {
      // Set rate, muted before calling play in case they were set before the
      // media engine was setup.  The media engine should just stash the rate
      // and muted values since it isn't already playing.
      GetWebMediaPlayer()->SetRate(playbackRate());
      GetWebMediaPlayer()->SetVolume(EffectiveMediaVolume());
      GetWebMediaPlayer()->Play();
    }

    StartPlaybackProgressTimer();
    playing_ = true;
  } else {  // Should not be playing right now
    if (is_playing) {
      GetWebMediaPlayer()->Pause();
    }

    playback_progress_timer_.Stop();
    playing_ = false;
    double time = currentTime();
    if (time > last_seek_time_)
      AddPlayedRange(last_seek_time_, time);
  }
}

void MediaPlayer::StopPeriodicTimers() {
  progress_event_timer_.Stop();
  playback_progress_timer_.Stop();
  check_viewport_intersection_timer_.Stop();
}

void MediaPlayer::ClearMediaPlayerAndAudioSourceProviderClientWithoutLocking() {
  GetAudioSourceProvider().SetClient(nullptr);
  if (web_media_player_) {
    audio_source_provider_.Wrap(nullptr);
    web_media_player_.reset();
  }
}

void MediaPlayer::ClearMediaPlayer() {
  CloseMediaSource();

  CancelDeferredLoad();

  {
    AudioSourceProviderClientLockScope scope(*this);
    ClearMediaPlayerAndAudioSourceProviderClientWithoutLocking();
  }

  StopPeriodicTimers();
  load_timer_.Stop();

  pending_action_flags_ = 0;
  load_state_ = kWaitingForSource;

  // We can't cast if we don't have a media player.
  playing_remotely_ = false;
  RemoteRouteAvailabilityChanged(WebRemotePlaybackAvailability::kUnknown);
}
/*
void MediaPlayer::ContextDestroyed(ExecutionContext*) {
  BLINK_MEDIA_LOG << "contextDestroyed(" << (void*)this << ")";

  // Close the async event queue so that no events are enqueued.
  CancelPendingEventsAndCallbacks();
  async_event_queue_->Close();

  // Clear everything in the Media Element
  ClearMediaPlayer();
  ready_state_ = kHaveNothing;
  ready_state_maximum_ = kHaveNothing;
  SetNetworkState(kNetworkEmpty);
  SetShouldDelayLoadEvent(false);
  current_source_node_ = nullptr;
  official_playback_position_ = 0;
  official_playback_position_needs_update_ = true;
  GetCueTimeline().UpdateActiveCues(0);
  playing_ = false;
  paused_ = true;
  seeking_ = false;

  StopPeriodicTimers();

  // Ensure that hasPendingActivity() is not preventing garbage collection,
  // since otherwise this media element will simply leak.
  DCHECK(!HasPendingActivity());
}

bool MediaPlayer::HasPendingActivity() const {
  // The delaying-the-load-event flag is set by resource selection algorithm
  // when looking for a resource to load, before networkState has reached to
  // kNetworkLoading.
  if (should_delay_load_event_)
    return true;

  // When networkState is kNetworkLoading, progress and stalled events may be
  // fired.
  if (network_state_ == kNetworkLoading)
    return true;

  {
    // Disable potential updating of playback position, as that will
    // require v8 allocations; not allowed while GCing
    // (hasPendingActivity() is called during a v8 GC.)
    AutoReset<bool> scope(&official_playback_position_needs_update_, false);

    // When playing or if playback may continue, timeupdate events may be fired.
    if (CouldPlayIfEnoughData())
      return true;
  }

  // When the seek finishes timeupdate and seeked events will be fired.
  if (seeking_)
    return true;

  // When connected to a MediaSource, e.g. setting MediaSource.duration will
  // cause a durationchange event to be fired.
  if (media_source_)
    return true;

  // Wait for any pending events to be fired.
  if (async_event_queue_->HasPendingEvents())
    return true;

  return false;
}
*/
WebLayer* MediaPlayer::PlatformLayer() const {
  return web_layer_;
}

bool MediaPlayer::HasClosedCaptions() const {
  return false;
}

bool MediaPlayer::TextTracksVisible() const {
  return text_tracks_visible_;
}

// static
void MediaPlayer::AssertShadowRootChildren(ShadowRoot& shadow_root) {
#if DCHECK_IS_ON()
  // There can be up to three children: media remoting interstitial, text track
  // container, and media controls. The media controls has to be the last child
  // if presend, and has to be the next sibling of the text track container if
  // both present. When present, media remoting interstitial has to be the first
  // child.
  unsigned number_of_children = shadow_root.CountChildren();
  DCHECK_LE(number_of_children, 3u);
  Node* first_child = shadow_root.firstChild();
  Node* last_child = shadow_root.lastChild();
  if (number_of_children == 1) {
    DCHECK(first_child->IsTextTrackContainer() ||
           first_child->IsMediaControls() ||
           first_child->IsMediaRemotingInterstitial());
  } else if (number_of_children == 2) {
    DCHECK(first_child->IsTextTrackContainer() ||
           first_child->IsMediaRemotingInterstitial());
    DCHECK(last_child->IsTextTrackContainer() || last_child->IsMediaControls());
    if (first_child->IsTextTrackContainer())
      DCHECK(last_child->IsMediaControls());
  } else if (number_of_children == 3) {
    Node* second_child = first_child->nextSibling();
    DCHECK(first_child->IsMediaRemotingInterstitial());
    DCHECK(second_child->IsTextTrackContainer());
    DCHECK(last_child->IsMediaControls());
  }
#endif
}

unsigned MediaPlayer::webkitAudioDecodedByteCount() const {
  if (!GetWebMediaPlayer())
    return 0;
  return GetWebMediaPlayer()->AudioDecodedByteCount();
}

unsigned MediaPlayer::webkitVideoDecodedByteCount() const {
  if (!GetWebMediaPlayer())
    return 0;
  return GetWebMediaPlayer()->VideoDecodedByteCount();
}

void MediaPlayer::SetShouldDelayLoadEvent(bool should_delay) {
  if (should_delay_load_event_ == should_delay)
    return;

  BLINK_MEDIA_LOG << "setShouldDelayLoadEvent(" << (void*)this << ", "
                  << BoolString(should_delay) << ")";

  should_delay_load_event_ = should_delay;
}

// TODO(srirama.m): Merge it to resetMediaElement if possible and remove it.
void MediaPlayer::ResetMediaPlayerAndMediaSource() {
  CloseMediaSource();

  {
    AudioSourceProviderClientLockScope scope(*this);
    ClearMediaPlayerAndAudioSourceProviderClientWithoutLocking();
  }

  // We haven't yet found out if any remote routes are available.
  playing_remotely_ = false;
  RemoteRouteAvailabilityChanged(WebRemotePlaybackAvailability::kUnknown);

  if (audio_source_node_)
    GetAudioSourceProvider().SetClient(audio_source_node_);
}

void MediaPlayer::SetAudioSourceNode(AudioSourceProviderClient* source_node) {
  audio_source_node_ = source_node;

  AudioSourceProviderClientLockScope scope(*this);
  GetAudioSourceProvider().SetClient(audio_source_node_);
}

WebMediaPlayer::CORSMode MediaPlayer::CorsMode() const {
  const String& cross_origin_mode = cross_origin_;
  if (cross_origin_mode.IsNull())
    return WebMediaPlayer::kCORSModeUnspecified;
  if (DeprecatedEqualIgnoringCase(cross_origin_mode, "use-credentials"))
    return WebMediaPlayer::kCORSModeUseCredentials;
  return WebMediaPlayer::kCORSModeAnonymous;
}

void MediaPlayer::SetWebLayer(WebLayer* web_layer) {}

WebMediaPlayer::TrackId MediaPlayer::AddAudioTrack(
    const WebString&,
    WebMediaPlayerClient::AudioTrackKind,
    const WebString&,
    const WebString&,
    bool) {
  return WebMediaPlayer::TrackId();
}

void MediaPlayer::RemoveAudioTrack(WebMediaPlayer::TrackId) {}

WebMediaPlayer::TrackId MediaPlayer::AddVideoTrack(
    const WebString&,
    WebMediaPlayerClient::VideoTrackKind,
    const WebString&,
    const WebString&,
    bool) {
  return WebMediaPlayer::TrackId();
}

void MediaPlayer::RemoveVideoTrack(WebMediaPlayer::TrackId) {}

void MediaPlayer::AddTextTrack(WebInbandTextTrack*) {}

void MediaPlayer::RemoveTextTrack(WebInbandTextTrack*) {}

void MediaPlayer::MediaSourceOpened(WebMediaSource* web_media_source) {
  SetShouldDelayLoadEvent(false);
  media_source_->SetWebMediaSourceAndOpen(WTF::WrapUnique(web_media_source));
}

DEFINE_TRACE(MediaPlayer) {
  visitor->Trace(played_time_ranges_);
  visitor->Trace(async_event_queue_);
  visitor->Trace(error_);
  visitor->Trace(current_source_node_);
  visitor->Trace(next_child_node_to_consider_);
  visitor->Trace(media_source_);
  visitor->Trace(cue_timeline_);
  visitor->Trace(play_promise_resolvers_);
  visitor->Trace(play_promise_resolve_list_);
  visitor->Trace(play_promise_reject_list_);
  visitor->Trace(audio_source_provider_);
  visitor->Trace(src_object_);
  visitor->template RegisterWeakMembers<MediaPlayer,
                                        &MediaPlayer::ClearWeakMembers>(this);

  ContextLifecycleObserver::Trace(visitor);
}

void MediaPlayer::CreatePlaceholderTracksIfNecessary() {}

void MediaPlayer::SelectInitialTracksIfNecessary() {}

void MediaPlayer::SetNetworkState(NetworkState state) {
  if (network_state_ == state)
    return;

  network_state_ = state;
}

void MediaPlayer::VideoWillBeDrawnToCanvas() const {
  UseCounter::Count(GetExecutionContext(), WebFeature::kVideoInCanvas);
}

void MediaPlayer::ScheduleResolvePlayPromises() {
  // TODO(mlamouri): per spec, we should create a new task but we can't create
  // a new cancellable task without cancelling the previous one. There are two
  // approaches then: cancel the previous task and create a new one with the
  // appended promise list or append the new promise to the current list. The
  // latter approach is preferred because it might be the less observable
  // change.
  DCHECK(play_promise_resolve_list_.IsEmpty() ||
         play_promise_resolve_task_handle_.IsActive());
  if (play_promise_resolvers_.IsEmpty())
    return;

  play_promise_resolve_list_.AppendVector(play_promise_resolvers_);
  play_promise_resolvers_.clear();

  if (play_promise_resolve_task_handle_.IsActive())
    return;

  play_promise_resolve_task_handle_ =
      TaskRunnerHelper::Get(TaskType::kMediaElementEvent, GetExecutionContext())
          ->PostCancellableTask(
              BLINK_FROM_HERE,
              WTF::Bind(&MediaPlayer::ResolveScheduledPlayPromises,
                        WrapWeakPersistent(this)));
}

void MediaPlayer::ScheduleRejectPlayPromises(ExceptionCode code) {
  // TODO(mlamouri): per spec, we should create a new task but we can't create
  // a new cancellable task without cancelling the previous one. There are two
  // approaches then: cancel the previous task and create a new one with the
  // appended promise list or append the new promise to the current list. The
  // latter approach is preferred because it might be the less observable
  // change.
  DCHECK(play_promise_reject_list_.IsEmpty() ||
         play_promise_reject_task_handle_.IsActive());
  if (play_promise_resolvers_.IsEmpty())
    return;

  play_promise_reject_list_.AppendVector(play_promise_resolvers_);
  play_promise_resolvers_.clear();

  if (play_promise_reject_task_handle_.IsActive())
    return;

  // TODO(nhiroki): Bind this error code to a cancellable task instead of a
  // member field.
  play_promise_error_code_ = code;
  play_promise_reject_task_handle_ =
      TaskRunnerHelper::Get(TaskType::kMediaElementEvent, GetExecutionContext())
          ->PostCancellableTask(
              BLINK_FROM_HERE,
              WTF::Bind(&MediaPlayer::RejectScheduledPlayPromises,
                        WrapWeakPersistent(this)));
}

void MediaPlayer::ScheduleNotifyPlaying() {
  ScheduleEvent(EventTypeNames::playing);
  ScheduleResolvePlayPromises();
}

void MediaPlayer::ResolveScheduledPlayPromises() {
  for (auto& resolver : play_promise_resolve_list_)
    resolver->Resolve();

  play_promise_resolve_list_.clear();
}

void MediaPlayer::RejectScheduledPlayPromises() {
  // TODO(mlamouri): the message is generated based on the code because
  // arguments can't be passed to a cancellable task. In order to save space
  // used by the object, the string isn't saved.
  DCHECK(play_promise_error_code_ == kAbortError ||
         play_promise_error_code_ == kNotSupportedError);
  if (play_promise_error_code_ == kAbortError) {
    RecordPlayPromiseRejected(PlayPromiseRejectReason::kInterruptedByPause);
    RejectPlayPromisesInternal(kAbortError,
                               "The play() request was interrupted by a call "
                               "to pause(). https://goo.gl/LdLk22");
  } else {
    RecordPlayPromiseRejected(PlayPromiseRejectReason::kNoSupportedSources);
    RejectPlayPromisesInternal(
        kNotSupportedError,
        "Failed to load because no supported source was found.");
  }
}

void MediaPlayer::RejectPlayPromises(ExceptionCode code,
                                     const String& message) {
  play_promise_reject_list_.AppendVector(play_promise_resolvers_);
  play_promise_resolvers_.clear();
  RejectPlayPromisesInternal(code, message);
}

void MediaPlayer::RejectPlayPromisesInternal(ExceptionCode code,
                                             const String& message) {
  DCHECK(code == kAbortError || code == kNotSupportedError);

  for (auto& resolver : play_promise_reject_list_)
    resolver->Reject(DOMException::Create(code, message));

  play_promise_reject_list_.clear();
}

EnumerationHistogram& MediaPlayer::ShowControlsHistogram() const {
  DEFINE_STATIC_LOCAL(EnumerationHistogram, histogram,
                      ("Media.Controls.Show.Audio", kMediaControlsShowMax));
  return histogram;
}

void MediaPlayer::ClearWeakMembers(Visitor* visitor) {
  if (!ThreadHeap::IsHeapObjectAlive(audio_source_node_)) {
    GetAudioSourceProvider().SetClient(nullptr);
    audio_source_node_ = nullptr;
  }
}

void MediaPlayer::AudioSourceProviderImpl::Wrap(
    WebAudioSourceProvider* provider) {
  MutexLocker locker(provide_input_lock);

  if (web_audio_source_provider_ && provider != web_audio_source_provider_)
    web_audio_source_provider_->SetClient(nullptr);

  web_audio_source_provider_ = provider;
  if (web_audio_source_provider_)
    web_audio_source_provider_->SetClient(client_.Get());
}

void MediaPlayer::AudioSourceProviderImpl::SetClient(
    AudioSourceProviderClient* client) {
  MutexLocker locker(provide_input_lock);

  if (client)
    client_ = new MediaPlayer::AudioClientImpl(client);
  else
    client_.Clear();

  if (web_audio_source_provider_)
    web_audio_source_provider_->SetClient(client_.Get());
}

void MediaPlayer::AudioSourceProviderImpl::ProvideInput(
    AudioBus* bus,
    size_t frames_to_process) {
  DCHECK(bus);

  MutexTryLocker try_locker(provide_input_lock);
  if (!try_locker.Locked() || !web_audio_source_provider_ || !client_.Get()) {
    bus->Zero();
    return;
  }

  // Wrap the AudioBus channel data using WebVector.
  size_t n = bus->NumberOfChannels();
  WebVector<float*> web_audio_data(n);
  for (size_t i = 0; i < n; ++i)
    web_audio_data[i] = bus->Channel(i)->MutableData();

  web_audio_source_provider_->ProvideInput(web_audio_data, frames_to_process);
}

void MediaPlayer::AudioClientImpl::SetFormat(size_t number_of_channels,
                                             float sample_rate) {
  if (client_)
    client_->SetFormat(number_of_channels, sample_rate);
}

DEFINE_TRACE(MediaPlayer::AudioClientImpl) {
  visitor->Trace(client_);
}

DEFINE_TRACE(MediaPlayer::AudioSourceProviderImpl) {
  visitor->Trace(client_);
}

void MediaPlayer::OnBecamePersistentVideo(bool) {}

void MediaPlayer::ActivateViewportIntersectionMonitoring(bool activate) {
  if (activate && !check_viewport_intersection_timer_.IsActive()) {
    check_viewport_intersection_timer_.StartRepeating(
        kCheckViewportIntersectionIntervalSeconds, BLINK_FROM_HERE);
  } else if (!activate) {
    check_viewport_intersection_timer_.Stop();
  }
}

void MediaPlayer::MediaRemotingStarted(
    const WebString& remote_device_friendly_name) {}

void MediaPlayer::MediaRemotingStopped() {}

bool MediaPlayer::HasNativeControls() {
  return ShouldShowControls(RecordMetricsBehavior::kDoRecord);
}

bool MediaPlayer::IsAudioElement() {
  return true;
}

WebMediaPlayer::DisplayType MediaPlayer::DisplayType() const {
  return WebMediaPlayer::DisplayType::kInline;
}

void MediaPlayer::CheckViewportIntersectionTimerFired(TimerBase*) {}

void MediaPlayer::ViewportFillDebouncerTimerFired(TimerBase*) {
  mostly_filling_viewport_ = true;
  if (web_media_player_)
    web_media_player_->BecameDominantVisibleContent(mostly_filling_viewport_);
}

}  // namespace blink
