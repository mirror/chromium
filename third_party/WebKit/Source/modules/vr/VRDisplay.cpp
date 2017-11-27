// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/vr/VRDisplay.h"

#include "build/build_config.h"
#include "core/css/CSSPropertyValueSet.h"
#include "core/dom/DOMException.h"
#include "core/dom/FrameRequestCallbackCollection.h"
#include "core/dom/ScriptedAnimationController.h"
#include "core/dom/UserGestureIndicator.h"
#include "core/frame/Frame.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameView.h"
#include "core/frame/UseCounter.h"
#include "core/imagebitmap/ImageBitmap.h"
#include "core/inspector/ConsoleMessage.h"
#include "core/layout/LayoutView.h"
#include "core/loader/DocumentLoader.h"
#include "core/paint/compositing/PaintLayerCompositor.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "modules/EventTargetModules.h"
#include "modules/vr/NavigatorVR.h"
#include "modules/vr/VRController.h"
#include "modules/vr/VRDisplayCapabilities.h"
#include "modules/vr/VREyeParameters.h"
#include "modules/vr/VRFrameData.h"
#include "modules/vr/VRLayerInit.h"
#include "modules/vr/VRPose.h"
#include "modules/vr/VRStageParameters.h"
#include "modules/webgl/WebGLRenderingContextBase.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "platform/Histogram.h"
#include "platform/graphics/GpuMemoryBufferImageCopy.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/wtf/AutoReset.h"
#include "platform/wtf/Time.h"
#include "public/platform/Platform.h"
#include "public/platform/TaskType.h"

#include <array>
#include "core/dom/ExecutionContext.h"

//#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "ui/gfx/gpu_fence_passthrough.h"

// WARNING: error checking has a huge performance cost. Only
// enable this when actively debugging problems.
#define EXPENSIVE_GL_ERROR_CHECKING 0

#if EXPENSIVE_GL_ERROR_CHECKING
#define CHECK_ERR                                            \
  do {                                                       \
    DVLOG(2) << __FUNCTION__ << ";;; error check START";     \
    GLint err;                                               \
    while ((err = context_gl_->GetError()) != GL_NO_ERROR) { \
      LOG(INFO) << __FUNCTION__ << ";;; GL ERROR " << err;   \
    }                                                        \
    DVLOG(2) << __FUNCTION__ << ";;; error check DONE";      \
  } while (0)
#else
#define CHECK_ERR \
  do {            \
  } while (0)
#endif

#if 0
#undef DVLOG
#define DVLOG(x) LOG(INFO)
#endif

namespace blink {

namespace {

VREye StringToVREye(const String& which_eye) {
  if (which_eye == "left")
    return kVREyeLeft;
  if (which_eye == "right")
    return kVREyeRight;
  return kVREyeNone;
}

class VRDisplayFrameRequestCallback
    : public FrameRequestCallbackCollection::FrameCallback {
 public:
  explicit VRDisplayFrameRequestCallback(VRDisplay* vr_display)
      : vr_display_(vr_display) {}
  ~VRDisplayFrameRequestCallback() override {}
  void Invoke(double high_res_time_ms) override {
    double monotonic_time;
    if (!vr_display_->GetDocument() || !vr_display_->GetDocument()->Loader()) {
      monotonic_time = WTF::MonotonicallyIncreasingTime();
    } else {
      // Convert document-zero time back to monotonic time.
      double reference_monotonic_time = vr_display_->GetDocument()
                                            ->Loader()
                                            ->GetTiming()
                                            .ReferenceMonotonicTime();
      monotonic_time = (high_res_time_ms / 1000.0) + reference_monotonic_time;
    }
    vr_display_->OnMagicWindowVSync(monotonic_time);
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(vr_display_);

    FrameRequestCallbackCollection::FrameCallback::Trace(visitor);
  }

  Member<VRDisplay> vr_display_;
};

}  // namespace

VRDisplay::VRDisplay(
    NavigatorVR* navigator_vr,
    device::mojom::blink::VRMagicWindowProviderPtr magic_window_provider,
    device::mojom::blink::VRDisplayHostPtr display,
    device::mojom::blink::VRDisplayClientRequest request)
    : PausableObject(navigator_vr->GetDocument()),
      navigator_vr_(navigator_vr),
      capabilities_(new VRDisplayCapabilities()),
      eye_parameters_left_(new VREyeParameters()),
      eye_parameters_right_(new VREyeParameters()),
      magic_window_provider_(std::move(magic_window_provider)),
      display_(std::move(display)),
      submit_frame_client_binding_(this),
      display_client_binding_(this, std::move(request)) {
  PauseIfNeeded();  // Initialize SuspendabaleObject.
}

VRDisplay::~VRDisplay() {}

void VRDisplay::Pause() {}

void VRDisplay::Unpause() {
  RequestVSync();
}

VRController* VRDisplay::Controller() {
  return navigator_vr_->Controller();
}

void VRDisplay::Update(const device::mojom::blink::VRDisplayInfoPtr& display) {
  display_id_ = display->index;
  display_name_ = display->displayName;
  is_connected_ = true;

  capabilities_->SetHasPosition(display->capabilities->hasPosition);
  capabilities_->SetHasExternalDisplay(
      display->capabilities->hasExternalDisplay);
  capabilities_->SetCanPresent(display->capabilities->canPresent);
  capabilities_->SetMaxLayers(display->capabilities->canPresent ? 1 : 0);

  // Ignore non presenting delegate
  bool is_valid = display->leftEye->renderWidth > 0;
  bool need_on_present_change = false;
  if (is_presenting_ && is_valid && !is_valid_device_for_presenting_) {
    need_on_present_change = true;
  }
  is_valid_device_for_presenting_ = is_valid;
  eye_parameters_left_->Update(display->leftEye);
  eye_parameters_right_->Update(display->rightEye);

  if (!display->stageParameters.is_null()) {
    if (!stage_parameters_)
      stage_parameters_ = new VRStageParameters();
    stage_parameters_->Update(display->stageParameters);
  } else {
    stage_parameters_ = nullptr;
  }

  if (need_on_present_change) {
    OnPresentChange();
  }
}

bool VRDisplay::getFrameData(VRFrameData* frame_data) {
  if (!FocusedOrPresenting() || !frame_pose_ || display_blurred_)
    return false;

  if (!frame_data)
    return false;

  if (!in_animation_frame_) {
    Document* doc = navigator_vr_->GetDocument();
    if (doc) {
      doc->AddConsoleMessage(
          ConsoleMessage::Create(kRenderingMessageSource, kWarningMessageLevel,
                                 "getFrameData must be called within a "
                                 "VRDisplay.requestAnimationFrame callback."));
    }
    return false;
  }

  if (depth_near_ == depth_far_)
    return false;

  return frame_data->Update(frame_pose_, eye_parameters_left_,
                            eye_parameters_right_, depth_near_, depth_far_);
}

VREyeParameters* VRDisplay::getEyeParameters(const String& which_eye) {
  switch (StringToVREye(which_eye)) {
    case kVREyeLeft:
      return eye_parameters_left_;
    case kVREyeRight:
      return eye_parameters_right_;
    default:
      return nullptr;
  }
}

void VRDisplay::RequestVSync() {
  DVLOG(2) << __FUNCTION__
           << " start: pending_vrdisplay_raf_=" << pending_vrdisplay_raf_
           << " in_animation_frame_=" << in_animation_frame_
           << " did_submit_this_frame_=" << did_submit_this_frame_;
  if (!pending_vrdisplay_raf_)
    return;
  Document* doc = navigator_vr_->GetDocument();
  if (!doc || !display_)
    return;
  // If we've switched from magic window to presenting, cancel the Document rAF
  // and start the VrPresentationProvider VSync.
  if (is_presenting_ && pending_vsync_id_ != -1) {
    doc->CancelAnimationFrame(pending_vsync_id_);
    pending_vsync_ = false;
    pending_vsync_id_ = -1;
  }
  if (display_blurred_ || pending_vsync_)
    return;

  if (!is_presenting_) {
    magic_window_provider_->GetPose(ConvertToBaseCallback(
        WTF::Bind(&VRDisplay::OnMagicWindowPose, WrapWeakPersistent(this))));
    pending_vsync_ = true;
    pending_vsync_id_ =
        doc->RequestAnimationFrame(new VRDisplayFrameRequestCallback(this));
    return;
  }
  DCHECK(vr_presentation_provider_.is_bound());

  // The logic here is a bit subtle. We get called from one of the following
  // four contexts:
  //
  // (a) from requestAnimationFrame if outside an animating context (i.e. the
  //     first rAF call from inside a getVRDisplays() promise)
  //
  // (b) from requestAnimationFrame in an animating context if the JS code
  //     calls rAF after submitFrame.
  //
  // (c) from submitFrame if that is called after rAF.
  //
  // (d) from ProcessScheduledAnimations if a rAF callback finishes without
  //     submitting a frame.
  //
  // These cases are mutually exclusive which prevents duplicate RequestVSync
  // calls. Case (a) only applies outside an animating context
  // (in_animation_frame_ is false), and (b,c,d) all require an animating
  // context. While in an animating context, submitFrame is called either
  // before rAF (b), after rAF (c), or not at all (d). If rAF isn't called at
  // all, there won't be future frames.

  pending_vsync_ = true;
  vr_presentation_provider_->GetVSync(ConvertToBaseCallback(
      WTF::Bind(&VRDisplay::OnPresentingVSync, WrapWeakPersistent(this))));

  DVLOG(2) << __FUNCTION__ << " done: pending_vsync_=" << pending_vsync_;
}

int VRDisplay::requestAnimationFrame(V8FrameRequestCallback* callback) {
  DVLOG(2) << __FUNCTION__;
  Document* doc = this->GetDocument();
  if (!doc)
    return 0;
  pending_vrdisplay_raf_ = true;

  // We want to delay the GetVSync call while presenting to ensure it doesn't
  // arrive earlier than frame submission, but other than that we want to call
  // it as early as possible. See comments inside RequestVSync() for more
  // details on the applicable cases.
  if (!in_animation_frame_ || did_submit_this_frame_) {
    RequestVSync();
  }
  FrameRequestCallbackCollection::V8FrameCallback* frame_callback =
      FrameRequestCallbackCollection::V8FrameCallback::Create(callback);
  frame_callback->SetUseLegacyTimeBase(false);
  return EnsureScriptedAnimationController(doc).RegisterCallback(
      frame_callback);
}

void VRDisplay::cancelAnimationFrame(int id) {
  DVLOG(2) << __FUNCTION__;
  if (!scripted_animation_controller_)
    return;
  scripted_animation_controller_->CancelCallback(id);
}

void VRDisplay::OnBlur() {
  DVLOG(1) << __FUNCTION__;
  display_blurred_ = true;
  navigator_vr_->EnqueueVREvent(VRDisplayEvent::Create(
      EventTypeNames::vrdisplayblur, true, false, this, ""));
}

void VRDisplay::OnFocus() {
  DVLOG(1) << __FUNCTION__;
  display_blurred_ = false;
  RequestVSync();

  navigator_vr_->EnqueueVREvent(VRDisplayEvent::Create(
      EventTypeNames::vrdisplayfocus, true, false, this, ""));
}

void ReportPresentationResult(PresentationResult result) {
  // Note that this is called twice for each call to requestPresent -
  // one to declare that requestPresent was called, and one for the
  // result.
  DEFINE_STATIC_LOCAL(
      EnumerationHistogram, vr_presentation_result_histogram,
      ("VRDisplayPresentResult",
       static_cast<int>(PresentationResult::kPresentationResultMax)));
  vr_presentation_result_histogram.Count(static_cast<int>(result));
}

ScriptPromise VRDisplay::requestPresent(ScriptState* script_state,
                                        const HeapVector<VRLayerInit>& layers) {
  DVLOG(1) << __FUNCTION__;
  ExecutionContext* execution_context = ExecutionContext::From(script_state);
  UseCounter::Count(execution_context, WebFeature::kVRRequestPresent);
  if (!execution_context->IsSecureContext()) {
    UseCounter::Count(execution_context,
                      WebFeature::kVRRequestPresentInsecureOrigin);
  }

  ReportPresentationResult(PresentationResult::kRequested);

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // If the VRDisplay does not advertise the ability to present reject the
  // request.
  if (!capabilities_->canPresent()) {
    DOMException* exception =
        DOMException::Create(kInvalidStateError, "VRDisplay cannot present.");
    resolver->Reject(exception);
    ReportPresentationResult(PresentationResult::kVRDisplayCannotPresent);
    return promise;
  }

  bool first_present = !is_presenting_;
  Document* doc = GetDocument();

  // Initiating VR presentation is only allowed in response to a user gesture.
  // If the VRDisplay is already presenting, however, repeated calls are
  // allowed outside a user gesture so that the presented content may be
  // updated.
  if (first_present) {
    if (!Frame::HasTransientUserActivation(doc ? doc->GetFrame() : nullptr)) {
      DOMException* exception = DOMException::Create(
          kInvalidStateError, "API can only be initiated by a user gesture.");
      resolver->Reject(exception);
      ReportPresentationResult(PresentationResult::kNotInitiatedByUserGesture);
      return promise;
    }

    // When we are requesting to start presentation with a user action or the
    // display has activated, record the user action.
    Platform::Current()->RecordAction(
        UserMetricsAction("VR.WebVR.requestPresent"));
  }

  // A valid number of layers must be provided in order to present.
  if (layers.size() == 0 || layers.size() > capabilities_->maxLayers()) {
    ForceExitPresent();
    DOMException* exception =
        DOMException::Create(kInvalidStateError, "Invalid number of layers.");
    resolver->Reject(exception);
    ReportPresentationResult(PresentationResult::kInvalidNumberOfLayers);
    return promise;
  }

  // If what we were given has an invalid source, need to exit fullscreen with
  // previous, valid source, so delay m_layer reassignment
  if (layers[0].source().IsNull()) {
    ForceExitPresent();
    DOMException* exception =
        DOMException::Create(kInvalidStateError, "Invalid layer source.");
    resolver->Reject(exception);
    ReportPresentationResult(PresentationResult::kInvalidLayerSource);
    return promise;
  }
  layer_ = layers[0];

  CanvasRenderingContext* rendering_context;
  if (layer_.source().IsHTMLCanvasElement()) {
    rendering_context =
        layer_.source().GetAsHTMLCanvasElement()->RenderingContext();
  } else {
    DCHECK(layer_.source().IsOffscreenCanvas());
    rendering_context =
        layer_.source().GetAsOffscreenCanvas()->RenderingContext();
  }

  if (!rendering_context || !rendering_context->Is3d()) {
    ForceExitPresent();
    DOMException* exception = DOMException::Create(
        kInvalidStateError, "Layer source must have a WebGLRenderingContext");
    resolver->Reject(exception);
    ReportPresentationResult(
        PresentationResult::kLayerSourceMissingWebGLContext);
    return promise;
  }

  // Save the WebGL script and underlying GL contexts for use by submitFrame().
  rendering_context_ = ToWebGLRenderingContextBase(rendering_context);
  context_gl_ = rendering_context_->ContextGL();

  Nullable<WebGLContextAttributes> attrs;
  rendering_context_->getContextAttributes(attrs);
  if (attrs.Get().antialias()) {
    ahb_sample_count_ = 4;
  } else {
    ahb_sample_count_ = 0;
  }
  LOG(INFO) << __FUNCTION__ << ";;; attrs.antialias=" << attrs.Get().antialias()
            << " ahb_sample_count_=" << ahb_sample_count_;

  if ((layer_.leftBounds().size() != 0 && layer_.leftBounds().size() != 4) ||
      (layer_.rightBounds().size() != 0 && layer_.rightBounds().size() != 4)) {
    ForceExitPresent();
    DOMException* exception = DOMException::Create(
        kInvalidStateError,
        "Layer bounds must either be an empty array or have 4 values");
    resolver->Reject(exception);
    ReportPresentationResult(PresentationResult::kInvalidLayerBounds);
    return promise;
  }

  for (float value : layer_.leftBounds()) {
    if (std::isnan(value)) {
      ForceExitPresent();
      DOMException* exception = DOMException::Create(
          kInvalidStateError, "Layer bounds must not contain NAN values");
      resolver->Reject(exception);
      ReportPresentationResult(PresentationResult::kInvalidLayerBounds);
      return promise;
    }
  }

  for (float value : layer_.rightBounds()) {
    if (std::isnan(value)) {
      ForceExitPresent();
      DOMException* exception = DOMException::Create(
          kInvalidStateError, "Layer bounds must not contain NAN values");
      resolver->Reject(exception);
      ReportPresentationResult(PresentationResult::kInvalidLayerBounds);
      return promise;
    }
  }

  if (!pending_present_resolvers_.IsEmpty()) {
    // If we are waiting on the results of a previous requestPresent call don't
    // fire a new request, just cache the resolver and resolve it when the
    // original request returns.
    pending_present_resolvers_.push_back(resolver);
  } else if (first_present) {
    if (!display_) {
      ForceExitPresent();
      DOMException* exception = DOMException::Create(
          kInvalidStateError, "The service is no longer active.");
      resolver->Reject(exception);
      return promise;
    }

    pending_present_resolvers_.push_back(resolver);
    device::mojom::blink::VRSubmitFrameClientPtr submit_frame_client;
    submit_frame_client_binding_.Close();
    submit_frame_client_binding_.Bind(mojo::MakeRequest(&submit_frame_client));
    display_->RequestPresent(
        std::move(submit_frame_client),
        mojo::MakeRequest(&vr_presentation_provider_),
        ConvertToBaseCallback(
            WTF::Bind(&VRDisplay::OnPresentComplete, WrapPersistent(this))));
    vr_presentation_provider_.set_connection_error_handler(
        ConvertToBaseCallback(
            WTF::Bind(&VRDisplay::OnPresentationProviderConnectionError,
                      WrapWeakPersistent(this))));
    pending_present_request_ = true;
  } else {
    UpdateLayerBounds();
    resolver->Resolve();
    ReportPresentationResult(PresentationResult::kSuccessAlreadyPresenting);
  }

  return promise;
}

void VRDisplay::OnPresentComplete(bool success) {
  pending_present_request_ = false;
  if (success) {
    this->BeginPresent();
  } else {
    this->ForceExitPresent();
    DOMException* exception = DOMException::Create(
        kNotAllowedError, "Presentation request was denied.");

    while (!pending_present_resolvers_.IsEmpty()) {
      ScriptPromiseResolver* resolver = pending_present_resolvers_.TakeFirst();
      resolver->Reject(exception);
    }
  }
}

ScriptPromise VRDisplay::exitPresent(ScriptState* script_state) {
  DVLOG(1) << __FUNCTION__;
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  if (!is_presenting_) {
    // Can't stop presenting if we're not presenting.
    DOMException* exception = DOMException::Create(
        kInvalidStateError, "VRDisplay is not presenting.");
    resolver->Reject(exception);
    return promise;
  }

  if (!display_) {
    DOMException* exception =
        DOMException::Create(kInvalidStateError, "VRService is not available.");
    resolver->Reject(exception);
    return promise;
  }
  display_->ExitPresent();

  resolver->Resolve();

  StopPresenting();

  return promise;
}

void VRDisplay::BeginPresent() {
  Document* doc = this->GetDocument();
  if (capabilities_->hasExternalDisplay()) {
    // Presenting with external displays has to make a copy of the image
    // since the canvas may still be visible at the same time.
    present_image_needs_copy_ = true;
    wait_for_previous_render_ = WaitPrevStrategy::NEVER;
  } else {
    if (layer_.source().IsHTMLCanvasElement()) {
      // TODO(klausw,crbug.com/698923): suppress compositor updates
      // since they aren't needed, they do a fair amount of extra
      // work.
    } else {
      DCHECK(layer_.source().IsOffscreenCanvas());
      // TODO(junov, crbug.com/695497): Implement OffscreenCanvas presentation
      ForceExitPresent();
      DOMException* exception = DOMException::Create(
          kInvalidStateError, "OffscreenCanvas presentation not implemented.");
      while (!pending_present_resolvers_.IsEmpty()) {
        ScriptPromiseResolver* resolver =
            pending_present_resolvers_.TakeFirst();
        resolver->Reject(exception);
      }
      ReportPresentationResult(
          PresentationResult::kPresentationNotSupportedByDisplay);
      return;
    }
  }

  if (doc) {
    Platform::Current()->RecordRapporURL("VR.WebVR.PresentSuccess",
                                         WebURL(doc->Url()));
  }
  if (!FocusedOrPresenting() && display_blurred_) {
    // Presentation doesn't care about focus, so if we're blurred because of
    // focus, then unblur.
    OnFocus();
  }
  is_presenting_ = true;
  // Call RequestVSync to switch from the (internal) document rAF to the
  // VrPresentationProvider VSync.
  RequestVSync();
  ReportPresentationResult(PresentationResult::kSuccess);

  UpdateLayerBounds();

  while (!pending_present_resolvers_.IsEmpty()) {
    ScriptPromiseResolver* resolver = pending_present_resolvers_.TakeFirst();
    resolver->Resolve();
  }
  OnPresentChange();

  // For GVR, we shut down normal vsync processing during VR presentation.
  // Run window.rAF once manually so that applications get a chance to
  // schedule a VRDisplay.rAF in case they do so only while presenting.
  if (!pending_vrdisplay_raf_ && !capabilities_->hasExternalDisplay()) {
    double timestamp = WTF::MonotonicallyIncreasingTime();
    Platform::Current()->CurrentThread()->GetWebTaskRunner()->PostTask(
        BLINK_FROM_HERE, WTF::Bind(&VRDisplay::ProcessScheduledWindowAnimations,
                                   WrapWeakPersistent(this), timestamp));
  }
}

// Need to close service if exists and then free rendering context.
void VRDisplay::ForceExitPresent() {
  if (display_) {
    display_->ExitPresent();
  }
  StopPresenting();
}

void VRDisplay::UpdateLayerBounds() {
  if (!display_)
    return;

  // Left eye defaults
  if (layer_.leftBounds().size() != 4)
    layer_.setLeftBounds({0.0f, 0.0f, 0.5f, 1.0f});
  // Right eye defaults
  if (layer_.rightBounds().size() != 4)
    layer_.setRightBounds({0.5f, 0.0f, 0.5f, 1.0f});

  const Vector<float>& left = layer_.leftBounds();
  const Vector<float>& right = layer_.rightBounds();

  vr_presentation_provider_->UpdateLayerBounds(
      vr_frame_id_, WebFloatRect(left[0], left[1], left[2], left[3]),
      WebFloatRect(right[0], right[1], right[2], right[3]),
      WebSize(source_width_, source_height_));
}

HeapVector<VRLayerInit> VRDisplay::getLayers() {
  HeapVector<VRLayerInit> layers;

  if (is_presenting_) {
    layers.push_back(layer_);
  }

  return layers;
}

void VRDisplay::submitFrame() {
  DVLOG(2) << __FUNCTION__;
  CHECK_ERR;

  if (!display_)
    return;
  TRACE_EVENT1("gpu", "submitFrame", "frame", vr_frame_id_);

  Document* doc = this->GetDocument();
  if (!doc)
    return;

  if (!is_presenting_) {
    doc->AddConsoleMessage(ConsoleMessage::Create(
        kRenderingMessageSource, kWarningMessageLevel,
        "submitFrame has no effect when the VRDisplay is not presenting."));
    return;
  }

  if (!in_animation_frame_) {
    doc->AddConsoleMessage(
        ConsoleMessage::Create(kRenderingMessageSource, kWarningMessageLevel,
                               "submitFrame must be called within a "
                               "VRDisplay.requestAnimationFrame callback."));
    return;
  }

  if (!context_gl_) {
    // Something got confused, we can't submit frames without a GL context.
    return;
  }

  // No frame Id to write before submitting the frame.
  if (vr_frame_id_ < 0) {
    // TODO(klausw): There used to be a submitFrame here, but we can't
    // submit without a frameId and associated pose data. Just drop it.
    return;
  }

  // Can't submit frames when the page isn't visible. This can happen  because
  // we don't use the unified BeginFrame rendering path for WebVR so visibility
  // updates aren't synchronized with WebVR VSync.
  if (!doc->GetPage()->IsPageVisible())
    return;

  // Check if the canvas got resized, if yes send a bounds update.
  int current_width = rendering_context_->drawingBufferWidth();
  int current_height = rendering_context_->drawingBufferHeight();
  if ((current_width != source_width_ || current_height != source_height_) &&
      current_width != 0 && current_height != 0) {
    source_width_ = current_width;
    source_height_ = current_height;
    UpdateLayerBounds();
  }

  // Wait for the previous render to finish, to avoid losing frames in the
  // Android Surface / GLConsumer pair. Early wait here is appropriate when
  // using a GpuFence to separate drawing, the new frame isn't complete yet at
  // this stage.
  if (wait_for_previous_render_ == WaitPrevStrategy::BEFORE_BITMAP) {
    TRACE_EVENT0("gpu", "waitForPreviousRenderToFinish");
    while (pending_previous_frame_render_) {
      if (!submit_frame_client_binding_.WaitForIncomingMethodCall()) {
        DLOG(ERROR) << "Failed to receive SubmitFrame response";
        break;
      }
    }
  }

  if (prev_frame_fence_) {
    DVLOG(2) << __FUNCTION__ << ";;; InsertClientGpuFenceCHROMIUM(prev_frame_fence_)";
    GLuint id = context_gl_->InsertClientGpuFenceCHROMIUM(prev_frame_fence_->AsClientGpuFence());
    CHECK_ERR;
    context_gl_->WaitGpuFenceCHROMIUM(id);
    CHECK_ERR;
    context_gl_->DestroyGpuFenceCHROMIUM(id);
    CHECK_ERR;
    prev_frame_fence_.reset();
  }
  CHECK_ERR;
  DVLOG(2) << __FUNCTION__;

  TRACE_EVENT_FLOW_STEP0("gpu", "vrframe", vr_frame_id_, "submitFrame");
  scoped_refptr<Image> image_ref = nullptr;
  if (!gvr_zero_copy_) {
    TRACE_EVENT_BEGIN0("gpu", "VRDisplay::GetStaticBitmapImage");
    image_ref = rendering_context_->GetStaticBitmapImage();
    TRACE_EVENT_END0("gpu", "VRDisplay::GetStaticBitmapImage");
  }

  // Hardware-accelerated rendering should always be texture backed,
  // as implemented by AcceleratedStaticBitmapImage. Ensure this is
  // the case, don't attempt to render if using an unexpected drawing
  // path.
  if (!gvr_zero_copy_ && (!image_ref.get() || !image_ref->IsTextureBacked())) {
    TRACE_EVENT0("gpu", "VRDisplay::GetImage_SlowFallback");
    // We get a non-texture-backed image when running layout tests
    // on desktop builds. Add a slow fallback so that these continue
    // working.
    image_ref = rendering_context_->GetImage(kPreferAcceleration,
                                             kSnapshotReasonCreateImageBitmap);
    if (!image_ref.get() || !image_ref->IsTextureBacked()) {
      NOTREACHED()
          << "WebVR requires hardware-accelerated rendering to texture";
      return;
    }
  }

  if (present_image_needs_copy_) {
#if defined(OS_WIN)
    TRACE_EVENT0("gpu", "VRDisplay::CopyImage");
    if (!frame_copier_) {
      frame_copier_ = std::make_unique<GpuMemoryBufferImageCopy>(context_gl_);
    }
    auto gpu_memory_buffer = frame_copier_->CopyImage(image_ref.get());
    DrawingBuffer::Client* client =
        static_cast<DrawingBuffer::Client*>(rendering_context_.Get());
    client->DrawingBufferClientRestoreTexture2DBinding();
    client->DrawingBufferClientRestoreFramebufferBinding();
    client->DrawingBufferClientRestoreRenderbufferBinding();

    // We decompose the cloned handle, and use it to create a mojo::ScopedHandle
    // which will own cleanup of the handle, and will be passed over IPC.
    gfx::GpuMemoryBufferHandle gpu_handle =
        CloneHandleForIPC(gpu_memory_buffer->GetHandle());
    vr_presentation_provider_->SubmitFrameWithTextureHandle(
        vr_frame_id_, mojo::WrapPlatformFile(gpu_handle.handle.GetHandle()));
#else
    NOTIMPLEMENTED();
#endif
  } else if (gvr_zero_copy_) {
    TRACE_EVENT0("gpu", "VRDisplay::SubmitZeroCopy");
    CHECK_ERR;
    DVLOG(2) << __FUNCTION__;
    // Avoid overstuffed buffers. FIXME!
    // context_gl_->Finish();

    // Unbind the framebuffer before submitting, to ensure everything is
    // flushed by the time we get the native fence sync?
    //
    // According to
    // https://www.khronos.org/registry/OpenGL/extensions/QCOM/QCOM_tiled_rendering.txt,
    // changing FBO or drawing surface implies EndTilingQCOM and is a hint to
    // the driver to start resolving, and we must make sure that these commands
    // are flushed to the driver.

    context_gl_->BindFramebuffer(GL_FRAMEBUFFER, ahb_fbo_);
    CHECK_ERR;
    // Discard attachments to tell GL we're done with them.
    // Not sure if we can discard COLOR_ATTACHMENT0, the content is still
    // needed.
    const GLenum kAttachments[] = {// GL_COLOR_ATTACHMENT0,
                                   GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    context_gl_->DiscardFramebufferEXT(
        GL_FRAMEBUFFER, sizeof(kAttachments) / sizeof(kAttachments[0]),
        kAttachments);
    CHECK_ERR;

    rendering_context_->SetCustomBackbufferFBO(0);
    CHECK_ERR;

    gpu::SyncToken sync_token;

    // uint64_t fence = rendering_context_->CreateNativeSyncPoint();
    uint64_t fence = context_gl_->InsertFenceSyncCHROMIUM();
    CHECK_ERR;

    // Shallow flush is insufficient? "fence sync must be flushed before
    // generating sync token" error. TODO(klausw): that error is likely a red
    // herring caused by GPU context being lost due to a different previous
    // problem.
    {
      TRACE_EVENT0("gpu", "Flush_3");
      context_gl_->ShallowFlushCHROMIUM();
      // context_gl_->OrderingBarrierCHROMIUM();
      // context_gl_->Flush();
      CHECK_ERR;
    }

    {
      TRACE_EVENT0("gpu", "GenSyncTokenCHROMIUM");
      // TODO(klausw): is GenUnverifiedSyncTokenCHROMIUM possible here? Breaks
      // rendering.
      context_gl_->GenSyncTokenCHROMIUM(fence, sync_token.GetData());
      // context_gl_->GenUnverifiedSyncTokenCHROMIUM(fence,
      // sync_token.GetData());
      CHECK_ERR;
    }
    // LOG(INFO) << __FUNCTION__ << ";;; frame=" << vr_frame_id_;
    vr_presentation_provider_->SubmitFrameZeroCopy3(vr_frame_id_, sync_token);
    CHECK_ERR;

    DrawingBuffer::Client* client =
        static_cast<DrawingBuffer::Client*>(rendering_context_.Get());
    client->DrawingBufferClientRestoreFramebufferBinding();
} else {
    DVLOG(2) << __FUNCTION__;
    // The AcceleratedStaticBitmapImage must be kept alive until the
    // mailbox is used via createAndConsumeTextureCHROMIUM, the mailbox
    // itself does not keep it alive. We must keep a reference to the
    // image until the mailbox was consumed.
    StaticBitmapImage* static_image =
        static_cast<StaticBitmapImage*>(image_ref.get());
    TRACE_EVENT_BEGIN0("gpu", "VRDisplay::EnsureMailbox");
    static_image->EnsureMailbox(kVerifiedSyncToken);
    TRACE_EVENT_END0("gpu", "VRDisplay::EnsureMailbox");

    // Wait as late as possible before SubmitFrame to ensure we can do as much
    // work as possible in parallel with the previous frame's rendering. This
    // is used if submitting fully rendered frames to GVR, but is susceptible
    // to bad scheduling such as the fast/slow render pattern.
    if (wait_for_previous_render_ == WaitPrevStrategy::AFTER_BITMAP) {
      TRACE_EVENT0("gpu", "VRDisplay::waitForPreviousRenderToFinish");
      while (pending_previous_frame_render_) {
        if (!submit_frame_client_binding_.WaitForIncomingMethodCall()) {
          DLOG(ERROR) << "Failed to receive SubmitFrame response";
          break;
        }
      }
    }

    // Save a reference to the image to keep it alive until next frame, but
    // first verify that the previous image's transfer has finished before
    // overwriting it. Usually this check is satisfied without waiting.
    {
      TRACE_EVENT0("gpu", "VRDisplay::waitForPreviousTransferToFinish");
      while (pending_submit_frame_) {
        if (!submit_frame_client_binding_.WaitForIncomingMethodCall()) {
          DLOG(ERROR) << "Failed to receive SubmitFrame response";
          break;
        }
      }
    }
    previous_image_ = std::move(image_ref);
    pending_submit_frame_ = true;

    // Create mailbox and sync token for transfer.
    TRACE_EVENT_BEGIN0("gpu", "VRDisplay::GetMailbox");
    auto mailbox = static_image->GetMailbox();
    TRACE_EVENT_END0("gpu", "VRDisplay::GetMailbox");
    auto sync_token = static_image->GetSyncToken();

    TRACE_EVENT_BEGIN0("gpu", "VRDisplay::SubmitFrame");
    vr_presentation_provider_->SubmitFrame(
        vr_frame_id_, gpu::MailboxHolder(mailbox, sync_token, GL_TEXTURE_2D));
    TRACE_EVENT_END0("gpu", "VRDisplay::SubmitFrame");
  }

  pending_previous_frame_render_ = true;
  did_submit_this_frame_ = true;
  // Reset our frame id, since anything we'd want to do (resizing/etc) can
  // no-longer happen to this frame.
  vr_frame_id_ = -1;

  // If we were deferring a rAF-triggered vsync request, do this now.
  // TODO(klausw): does doing this just before submitting reduce downtime
  // between frames?
  RequestVSync();

  // If preserveDrawingBuffer is false, must clear now. Normally this
  // happens as part of compositing, but that's not active while
  // presenting, so run the responsible code directly.
  if (!gvr_zero_copy_) {
    // TODO(klausw): do this in zero copy mode also. Getting errors?
    rendering_context_->MarkCompositedAndClearBackbufferIfNeeded();
  }
  DVLOG(2) << __FUNCTION__;
}

void VRDisplay::OnSubmitFrameTransferred() {
  DVLOG(3) << __FUNCTION__;
  pending_submit_frame_ = false;
}

void VRDisplay::OnSubmitFrameRendered() {
  DVLOG(3) << __FUNCTION__;
  pending_previous_frame_render_ = false;
}

void VRDisplay::OnSubmitFramePostRenderFence(const gfx::GpuFenceHandle& handle) {
  DVLOG(3) << __FUNCTION__ << ";;; got handle, fd=" << handle.native_fd.fd;

  // If we get GpuFences, we want to wait a bit earlier.
  //
  // TODO(klausw): This should really be a flag in VRDisplayCapabilities, but
  // currently this gets set up in gvr_device.cc in a way that makes it hard to
  // apply runtime changes from vr_shell_gl.cc.
  if (wait_for_previous_render_ == WaitPrevStrategy::AFTER_BITMAP) {
    wait_for_previous_render_ = WaitPrevStrategy::BEFORE_BITMAP;
  }
#if 1
  prev_frame_fence_ = base::MakeUnique<gfx::GpuFencePassthrough>(handle);
#else
  // If we don't consume the GpuFence, need to close the FD to avoid a leak.
  close(handle.native_fd.fd);
#endif
}

Document* VRDisplay::GetDocument() {
  return navigator_vr_->GetDocument();
}

void VRDisplay::OnPresentChange() {
  frame_copier_ = nullptr;

  DVLOG(1) << __FUNCTION__ << ": is_presenting_=" << is_presenting_;
  if (is_presenting_ && !is_valid_device_for_presenting_) {
    DVLOG(1) << __FUNCTION__ << ": device not valid, not sending event";
    return;
  }
  navigator_vr_->EnqueueVREvent(VRDisplayEvent::Create(
      EventTypeNames::vrdisplaypresentchange, true, false, this, ""));
}

void VRDisplay::OnChanged(device::mojom::blink::VRDisplayInfoPtr display) {
  Update(display);
}

void VRDisplay::OnExitPresent() {
  StopPresenting();
}

void VRDisplay::OnConnected() {
  navigator_vr_->EnqueueVREvent(VRDisplayEvent::Create(
      EventTypeNames::vrdisplayconnect, true, false, this, "connect"));
}

void VRDisplay::OnDisconnected() {
  navigator_vr_->EnqueueVREvent(VRDisplayEvent::Create(
      EventTypeNames::vrdisplaydisconnect, true, false, this, "disconnect"));
}

void VRDisplay::StopPresenting() {
  if (is_presenting_) {
    if (!capabilities_->hasExternalDisplay()) {
      if (layer_.source().IsHTMLCanvasElement()) {
        // TODO(klausw,crbug.com/698923): If compositor updates are
        // suppressed, restore them here.
      } else {
        // TODO(junov, crbug.com/695497): Implement for OffscreenCanvas
      }
    } else {
      // Can't get into this presentation mode, so nothing to do here.
    }
    is_presenting_ = false;

    OnPresentChange();

    // Record user action for stop presenting.  Note that this could be
    // user-triggered or not.
    Platform::Current()->RecordAction(
        UserMetricsAction("VR.WebVR.StopPresenting"));
  }

  if (gvr_zero_copy_) {
    rendering_context_->SetCustomBackbufferFBO(0);
    ReleaseAHBResources();
    gvr_zero_copy_ = false;
  }
  rendering_context_ = nullptr;
  context_gl_ = nullptr;
  pending_submit_frame_ = false;
  pending_previous_frame_render_ = false;
  did_submit_this_frame_ = false;
}

void VRDisplay::OnActivate(device::mojom::blink::VRDisplayEventReason reason,
                           OnActivateCallback on_handled) {
  Document* doc = GetDocument();
  if (!doc)
    return;

  std::unique_ptr<UserGestureIndicator> gesture_indicator;
  if (reason == device::mojom::blink::VRDisplayEventReason::MOUNTED)
    gesture_indicator = Frame::NotifyUserActivation(doc->GetFrame());

  navigator_vr_->DispatchVREvent(VRDisplayEvent::Create(
      EventTypeNames::vrdisplayactivate, true, false, this, reason));
  std::move(on_handled).Run(!pending_present_request_ && !is_presenting_);
}

void VRDisplay::OnDeactivate(
    device::mojom::blink::VRDisplayEventReason reason) {
  navigator_vr_->EnqueueVREvent(VRDisplayEvent::Create(
      EventTypeNames::vrdisplaydeactivate, true, false, this, reason));
}

void VRDisplay::ProcessScheduledWindowAnimations(double timestamp) {
  TRACE_EVENT1("gpu", "VRDisplay::window.rAF", "frame", vr_frame_id_);
  auto* doc = navigator_vr_->GetDocument();
  if (!doc)
    return;
  auto* page = doc->GetPage();
  if (!page)
    return;

  bool had_pending_vrdisplay_raf = pending_vrdisplay_raf_;
  // TODO(klausw): update timestamp based on scheduling delay?
  page->Animator().ServiceScriptedAnimations(timestamp);

  if (had_pending_vrdisplay_raf != pending_vrdisplay_raf_) {
    DVLOG(1) << __FUNCTION__
             << ": window.rAF fallback successfully scheduled VRDisplay.rAF";
  }

  if (!pending_vrdisplay_raf_) {
    // There wasn't any call to vrDisplay.rAF, so we will not be getting new
    // frames from now on unless the application schedules one down the road in
    // reaction to a separate event or timeout. TODO(klausw,crbug.com/716087):
    // do something more useful here?
    DVLOG(1) << __FUNCTION__
             << ": no scheduled VRDisplay.requestAnimationFrame, presentation "
                "broken?";
  }
}

void VRDisplay::ProcessScheduledAnimations(double timestamp) {
  DVLOG(2) << __FUNCTION__;
  // Check if we still have a valid context, the animation controller
  // or document may have disappeared since we scheduled this.
  Document* doc = this->GetDocument();
  if (!doc || display_blurred_) {
    DVLOG(2) << __FUNCTION__ << ": early exit, doc=" << doc
             << " display_blurred_=" << display_blurred_;
    return;
  }

  if (doc->IsContextPaused()) {
    // We are currently suspended - try ProcessScheduledAnimations again later
    // when we resume.
    return;
  }

  TRACE_EVENT1("gpu", "VRDisplay::OnVSync", "frame", vr_frame_id_);

  if (pending_vrdisplay_raf_ && scripted_animation_controller_) {
    // Run the callback, making sure that in_animation_frame_ is only
    // true for the vrDisplay rAF and not for a legacy window rAF
    // that may be called later.
    AutoReset<bool> animating(&in_animation_frame_, true);
    pending_vrdisplay_raf_ = false;
    did_submit_this_frame_ = false;
    scripted_animation_controller_->ServiceScriptedAnimations(timestamp);
    if (pending_vrdisplay_raf_ && !did_submit_this_frame_) {
      DVLOG(2) << __FUNCTION__ << ": vrDisplay.rAF did not submit a frame";
      RequestVSync();
    }
  }
  if (pending_pose_)
    frame_pose_ = std::move(pending_pose_);

  // Sanity check: If pending_vrdisplay_raf_ is true and the vsync provider
  // is connected, we must now have a pending vsync.
  DCHECK(!pending_vrdisplay_raf_ || pending_vsync_);
}

void VRDisplay::AllocateAHBResources() {
  ahb_fbo_ = 0;
  ahb_depthbuffer_ = 0;
  ahb_texture_ = 0;
  context_gl_->GenFramebuffers(1, &ahb_fbo_);
  CHECK_ERR;
  context_gl_->GenRenderbuffers(1, &ahb_depthbuffer_);
  CHECK_ERR;
  // ahb_texture_ is created per frame and lazily deleted.
}

void VRDisplay::ReleaseAHBResources() {
  if (ahb_texture_ > 0) {
    context_gl_->DeleteTextures(1, &ahb_texture_);
    CHECK_ERR;
    ahb_texture_ = 0;
  }
  if (ahb_depthbuffer_ > 0) {
    context_gl_->DeleteRenderbuffers(1, &ahb_depthbuffer_);
    CHECK_ERR;
    ahb_depthbuffer_ = 0;
  }
  if (ahb_fbo_ > 0) {
    context_gl_->DeleteFramebuffers(1, &ahb_fbo_);
    CHECK_ERR;
    ahb_fbo_ = 0;
  }
}

void VRDisplay::CreateAndBindAHBImage(
    const base::Optional<gpu::MailboxHolder>& buffer_holder) {
  TRACE_EVENT_BEGIN0("gpu", "VRDisplay::BindTexImage");

  // Delete the previously-used texture as late as possible. Rebinding an
  // existing texture blocks for rendering to complete on
  // Texture::SetLevelInfo's "info.image = 0" in texture_manager.cc?
  if (ahb_texture_ > 0) {
    context_gl_->DeleteTextures(1, &ahb_texture_);
  }
  ahb_texture_ = context_gl_->CreateAndConsumeTextureCHROMIUM(
      GL_TEXTURE_2D, buffer_holder->mailbox.name);
  LOG(INFO) << __FUNCTION__ << ";;; texture=" << ahb_texture_;
  CHECK_ERR;
  context_gl_->BindTexture(GL_TEXTURE_2D, ahb_texture_);
  context_gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                             GL_NEAREST);
  context_gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                             GL_NEAREST);
  context_gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                             GL_CLAMP_TO_EDGE);
  context_gl_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                             GL_CLAMP_TO_EDGE);
  CHECK_ERR;
  TRACE_EVENT_END0("gpu", "VRDisplay::BindTexImage");

  context_gl_->FramebufferTexture2DMultisampleEXT(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ahb_texture_, 0,
      ahb_sample_count_);
  CHECK_ERR;

#if EXPENSIVE_GL_ERROR_CHECKING
  auto fbstatus = context_gl_->CheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fbstatus != GL_FRAMEBUFFER_COMPLETE) {
    LOG(ERROR) << __FUNCTION__
               << ";;; FRAMEBUFFER NOT COMPLETE, fbstatus=0x" << std::hex << fbstatus;
    context_gl_->BindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  CHECK_ERR;
#endif

  DrawingBuffer::Client* client =
      static_cast<DrawingBuffer::Client*>(rendering_context_.Get());
  client->DrawingBufferClientRestoreTexture2DBinding();
}

void VRDisplay::BindAHBToBufferHolder(
    const base::Optional<gpu::MailboxHolder>& buffer_holder,
    const blink::WebSize& buffer_size) {
  CHECK_ERR;

  TRACE_EVENT_BEGIN0("gpu", "VRDisplay::WaitSyncToken");
  context_gl_->WaitSyncTokenCHROMIUM(buffer_holder->sync_token.GetConstData());
  TRACE_EVENT_END0("gpu", "VRDisplay::WaitSyncToken");

  if (ahb_fbo_ == 0 || ahb_depthbuffer_ == 0) {
    AllocateAHBResources();
    // Set invalid size to force rebinding.
    ahb_width_ = -1;
    ahb_height_ = -1;
  }

  context_gl_->BindFramebuffer(GL_FRAMEBUFFER, ahb_fbo_);
  CHECK_ERR;

  if (buffer_size.width != ahb_width_ || buffer_size.height != ahb_height_) {
    LOG(INFO) << __FUNCTION__ << ";;; width=" << buffer_size.width << " height=" << buffer_size.height;
    context_gl_->BindFramebuffer(GL_FRAMEBUFFER, ahb_fbo_);
    CHECK_ERR;
    context_gl_->BindRenderbuffer(GL_RENDERBUFFER, ahb_depthbuffer_);
    CHECK_ERR;

    context_gl_->RenderbufferStorageMultisampleEXT(
        GL_RENDERBUFFER, ahb_sample_count_, GL_DEPTH24_STENCIL8_OES,
        buffer_size.width, buffer_size.height);
    CHECK_ERR;

    context_gl_->FramebufferRenderbuffer(GL_FRAMEBUFFER,
                                         GL_DEPTH_STENCIL_ATTACHMENT,
                                         GL_RENDERBUFFER, ahb_depthbuffer_);

    CHECK_ERR;
    ahb_width_ = buffer_size.width;
    ahb_height_ = buffer_size.height;

    DrawingBuffer::Client* client =
        static_cast<DrawingBuffer::Client*>(rendering_context_.Get());
    client->DrawingBufferClientRestoreRenderbufferBinding();
  }

#if 0
  // Invalidate all attachments, we're drawing fresh content. Redundant?
  // TODO(klausw): research and/or benchmark if this is helpful.
  const GLenum kAttachments[] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT,
                                 GL_STENCIL_ATTACHMENT};
  context_gl_->DiscardFramebufferEXT(
      GL_FRAMEBUFFER, sizeof(kAttachments) / sizeof(kAttachments[0]),
      kAttachments);
  CHECK_ERR;
#endif

  CreateAndBindAHBImage(buffer_holder);
  CHECK_ERR;

  rendering_context_->SetCustomBackbufferFBO(ahb_fbo_);
  CHECK_ERR;
  // TODO(klausw): handle preserveBuffer=true

  // Restore context state for the drawing buffer.
  DrawingBuffer::Client* client =
      static_cast<DrawingBuffer::Client*>(rendering_context_.Get());
  client->DrawingBufferClientRestoreFramebufferBinding();
}

void VRDisplay::OnPresentingVSync(
    device::mojom::blink::VRPosePtr pose,
    WTF::TimeDelta time_delta,
    int16_t frame_id,
    device::mojom::blink::VRPresentationProvider::VSyncStatus status,
    const base::Optional<gpu::MailboxHolder>& buffer_holder,
    const blink::WebSize& buffer_size) {
  CHECK_ERR;
  switch (status) {
    case device::mojom::blink::VRPresentationProvider::VSyncStatus::SUCCESS:
      break;
    case device::mojom::blink::VRPresentationProvider::VSyncStatus::CLOSING:
      return;
  }
  pending_vsync_ = false;

  frame_pose_ = std::move(pose);
  vr_frame_id_ = frame_id;

  TRACE_EVENT_FLOW_STEP0("gpu", "vrframe", vr_frame_id_, "PresentingVSync");

  DVLOG(3) << __FUNCTION__ << ";;; have buffer=" << !!buffer_holder;
  if (buffer_holder) {
    TRACE_EVENT0("gpu", "VRDisplay::BufferSetup");

    BindAHBToBufferHolder(buffer_holder, buffer_size);
    gvr_zero_copy_ = true;
    wait_for_previous_render_ = WaitPrevStrategy::BEFORE_BITMAP;
  } else {
    // LOG(INFO) << __FUNCTION__ << ";;; buffer=nullopt";
  }

  // Post a task to handle scheduled animations after the current
  // execution context finishes, so that we yield to non-mojo tasks in
  // between frames. Executing mojo tasks back to back within the same
  // execution context caused extreme input delay due to processing
  // multiple frames without yielding, see crbug.com/701444. I suspect
  // this is due to WaitForIncomingMethodCall receiving the OnVSync
  // but queueing it for immediate execution since it doesn't match
  // the interface being waited on.
  Platform::Current()->CurrentThread()->GetWebTaskRunner()->PostTask(
      BLINK_FROM_HERE,
      WTF::Bind(&VRDisplay::ProcessScheduledAnimations,
                WrapWeakPersistent(this), time_delta.InSecondsF()));
}

void VRDisplay::OnMagicWindowVSync(double timestamp) {
  DVLOG(2) << __FUNCTION__;
  pending_vsync_ = false;
  pending_vsync_id_ = -1;
  vr_frame_id_ = -1;
  Platform::Current()->CurrentThread()->GetWebTaskRunner()->PostTask(
      BLINK_FROM_HERE, WTF::Bind(&VRDisplay::ProcessScheduledAnimations,
                                 WrapWeakPersistent(this), timestamp));
}

void VRDisplay::OnMagicWindowPose(device::mojom::blink::VRPosePtr pose) {
  if (!in_animation_frame_) {
    frame_pose_ = std::move(pose);
  } else {
    pending_pose_ = std::move(pose);
  }
}

void VRDisplay::OnPresentationProviderConnectionError() {
  vr_presentation_provider_.reset();
  ForceExitPresent();
  pending_vsync_ = false;
  RequestVSync();
}

ScriptedAnimationController& VRDisplay::EnsureScriptedAnimationController(
    Document* doc) {
  if (!scripted_animation_controller_)
    scripted_animation_controller_ = ScriptedAnimationController::Create(doc);

  return *scripted_animation_controller_;
}

void VRDisplay::Dispose() {
  display_client_binding_.Close();
  vr_presentation_provider_.reset();
}

ExecutionContext* VRDisplay::GetExecutionContext() const {
  return ContextLifecycleObserver::GetExecutionContext();
}

const AtomicString& VRDisplay::InterfaceName() const {
  return EventTargetNames::VRDisplay;
}

void VRDisplay::ContextDestroyed(ExecutionContext* context) {
  PausableObject::ContextDestroyed(context);
  ForceExitPresent();
  scripted_animation_controller_.Clear();
}

bool VRDisplay::HasPendingActivity() const {
  // Prevent V8 from garbage collecting the wrapper object if there are
  // event listeners attached to it.
  return GetExecutionContext() && HasEventListeners();
}

void VRDisplay::FocusChanged() {
  DVLOG(1) << __FUNCTION__;
  if (navigator_vr_->IsFocused()) {
    OnFocus();
  } else if (!is_presenting_) {
    OnBlur();
  }
}

bool VRDisplay::FocusedOrPresenting() {
  // The browser can't track focus for frames, so we still need to check for
  // focus in the renderer, even if the browser is checking focus before
  // sending input.
  return navigator_vr_->IsFocused() || is_presenting_;
}

void VRDisplay::Trace(blink::Visitor* visitor) {
  visitor->Trace(navigator_vr_);
  visitor->Trace(capabilities_);
  visitor->Trace(stage_parameters_);
  visitor->Trace(eye_parameters_left_);
  visitor->Trace(eye_parameters_right_);
  visitor->Trace(layer_);
  visitor->Trace(rendering_context_);
  visitor->Trace(scripted_animation_controller_);
  visitor->Trace(pending_present_resolvers_);
  EventTargetWithInlineData::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

void VRDisplay::TraceWrappers(const ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(scripted_animation_controller_);
  EventTargetWithInlineData::TraceWrappers(visitor);
}

}  // namespace blink
