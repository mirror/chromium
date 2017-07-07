// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/widget_input_handler_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "content/common/input_messages.h"
#include "content/renderer/gpu/render_widget_compositor.h"
#include "content/renderer/ime_event_guard.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/render_widget.h"
#include "third_party/WebKit/public/platform/Platform.h"
#include "third_party/WebKit/public/platform/WebCoalescedInputEvent.h"
#include "third_party/WebKit/public/platform/WebKeyboardEvent.h"
#include "third_party/WebKit/public/platform/scheduler/renderer/renderer_scheduler.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace content {

namespace {

InputEventAckState InputEventDispositionToAck(
    ui::InputHandlerProxy::EventDisposition disposition) {
  switch (disposition) {
    case ui::InputHandlerProxy::DID_HANDLE:
      return INPUT_EVENT_ACK_STATE_CONSUMED;
    case ui::InputHandlerProxy::DID_NOT_HANDLE:
      return INPUT_EVENT_ACK_STATE_NOT_CONSUMED;
    case ui::InputHandlerProxy::DID_NOT_HANDLE_NON_BLOCKING_DUE_TO_FLING:
      return INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING_DUE_TO_FLING;
    case ui::InputHandlerProxy::DROP_EVENT:
      return INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS;
    case ui::InputHandlerProxy::DID_HANDLE_NON_BLOCKING:
      return INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING;
  }
  NOTREACHED();
  return INPUT_EVENT_ACK_STATE_UNKNOWN;
}

std::vector<blink::WebCompositionUnderline>
ConvertUIUnderlinesToBlinkUnderlines(
    const std::vector<ui::CompositionUnderline>& ui_underlines) {
  std::vector<blink::WebCompositionUnderline> underlines;
  for (const auto& underline : ui_underlines) {
    blink::WebCompositionUnderline blink_underline(
        underline.start_offset, underline.end_offset, underline.color,
        underline.thick, underline.background_color);
    underlines.push_back(blink_underline);
  }
  return underlines;
}
}  // namespace

WidgetInputHandlerImpl::WidgetInputHandlerImpl(
    base::WeakPtr<RenderWidget> render_widget,
    blink::scheduler::RendererScheduler* renderer_scheduler)
    : render_widget_(render_widget),
      renderer_scheduler_(renderer_scheduler),
      input_event_queue_(render_widget->GetInputEventQueue()),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      binding_(this),
      associated_binding_(this),
      weak_ptr_factory_(this) {
  weak_this_ = weak_ptr_factory_.GetWeakPtr();
  RenderThreadImpl* render_thread_impl = RenderThreadImpl::current();
  has_compositor_ = render_thread_impl && render_widget->compositor();

  if (render_thread_impl) {
    legacy_host_message_sender_ = render_thread_impl->GetChannel();
    legacy_host_message_routing_id_ = render_widget->routing_id();
  }

  if (has_compositor_) {
    compositor_task_runner_ = render_thread_impl->compositor_task_runner();
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &WidgetInputHandlerImpl::InitOnCompositorThread,
            base::Unretained(this),
            render_widget->compositor()->GetInputHandler(),
            render_widget->compositor_deps()->IsScrollAnimatorEnabled()));
  }
}

WidgetInputHandlerImpl::~WidgetInputHandlerImpl() {}

bool WidgetInputHandlerImpl::ShouldProxyToMainThread() const {
  return has_compositor_ && !main_thread_task_runner_->BelongsToCurrentThread();
}

void WidgetInputHandlerImpl::InitOnCompositorThread(
    const base::WeakPtr<cc::InputHandler>& input_handler,
    bool smooth_scroll_enabled) {
  input_handler_proxy_ = base::MakeUnique<ui::InputHandlerProxy>(
      input_handler.get(), this,
      base::FeatureList::IsEnabled(features::kTouchpadAndWheelScrollLatching));
  input_handler_proxy_->set_smooth_scroll_enabled(smooth_scroll_enabled);
}

void WidgetInputHandlerImpl::AddAssociatedInterface(
    mojom::WidgetInputHandlerAssociatedRequest request) {
  if (has_compositor_) {
    // Mojo channel bound on compositor thread.
    compositor_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&WidgetInputHandlerImpl::BindNow,
                                  base::Unretained(this), std::move(request)));
  } else {
    // Mojo channel bound on main thread.
    BindNow(std::move(request));
  }
}

void WidgetInputHandlerImpl::AddInterface(
    mojom::WidgetInputHandlerRequest request) {
  if (has_compositor_) {
    // Mojo channel bound on compositor thread.
    compositor_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&WidgetInputHandlerImpl::BindNow2,
                                  base::Unretained(this), std::move(request)));
  } else {
    // Mojo channel bound on main thread.
    BindNow2(std::move(request));
  }
}

void WidgetInputHandlerImpl::SetFocus(bool focused) {
  if (ShouldProxyToMainThread()) {
    RunOnMainThread(
        base::Bind(&WidgetInputHandlerImpl::SetFocus, weak_this_, focused));
    return;
  }
  if (!render_widget_)
    return;
  render_widget_->OnSetFocus(focused);
}

void WidgetInputHandlerImpl::MouseCaptureLost() {
  if (ShouldProxyToMainThread()) {
    RunOnMainThread(
        base::Bind(&WidgetInputHandlerImpl::MouseCaptureLost, weak_this_));
    return;
  }
  if (!render_widget_)
    return;
  blink::WebWidget* widget = render_widget_->GetWebWidget();
  if (widget)
    widget->MouseCaptureLost();
}

void WidgetInputHandlerImpl::SetEditCommandsForNextKeyEvent(
    const std::vector<EditCommand>& commands) {
  if (ShouldProxyToMainThread()) {
    RunOnMainThread(
        base::Bind(&WidgetInputHandlerImpl::SetEditCommandsForNextKeyEvent,
                   weak_this_, commands));
    return;
  }
  if (!render_widget_)
    return;
  render_widget_->OnSetEditCommandsForNextKeyEvent(commands);
}

void WidgetInputHandlerImpl::CursorVisibilityChanged(bool visible) {
  if (ShouldProxyToMainThread()) {
    RunOnMainThread(base::Bind(&WidgetInputHandlerImpl::CursorVisibilityChanged,
                               weak_this_, visible));
    return;
  }
  if (!render_widget_)
    return;
  blink::WebWidget* widget = render_widget_->GetWebWidget();
  if (widget)
    widget->SetCursorVisibilityState(visible);
}

void WidgetInputHandlerImpl::ImeSetComposition(
    const base::string16& text,
    const std::vector<ui::CompositionUnderline>& underlines,
    const gfx::Range& range,
    int32_t start,
    int32_t end) {
  if (ShouldProxyToMainThread()) {
    RunOnMainThread(base::Bind(&WidgetInputHandlerImpl::ImeSetComposition,
                               weak_this_, text, underlines, range, start,
                               end));
    return;
  }
  if (!render_widget_)
    return;

  render_widget_->OnImeSetComposition(
      text, ConvertUIUnderlinesToBlinkUnderlines(underlines), range, start,
      end);
}

void WidgetInputHandlerImpl::ImeCommitText(
    const base::string16& text,
    const std::vector<ui::CompositionUnderline>& underlines,
    const gfx::Range& range,
    int32_t relative_cursor_position) {
  if (ShouldProxyToMainThread()) {
    RunOnMainThread(base::Bind(&WidgetInputHandlerImpl::ImeCommitText,
                               weak_this_, text, underlines, range,
                               relative_cursor_position));
    return;
  }
  if (!render_widget_)
    return;

  render_widget_->OnImeCommitText(
      text, ConvertUIUnderlinesToBlinkUnderlines(underlines), range,
      relative_cursor_position);
}

void WidgetInputHandlerImpl::ImeFinishComposingText(bool keep_selection) {
  if (ShouldProxyToMainThread()) {
    RunOnMainThread(base::Bind(&WidgetInputHandlerImpl::ImeFinishComposingText,
                               weak_this_, keep_selection));
    return;
  }
  if (!render_widget_)
    return;
  render_widget_->OnImeFinishComposingText(keep_selection);
}

void WidgetInputHandlerImpl::RequestTextInputStateUpdate() {
  if (ShouldProxyToMainThread()) {
    RunOnMainThread(base::Bind(
        &WidgetInputHandlerImpl::RequestTextInputStateUpdate, weak_this_));
    return;
  }
  if (!render_widget_)
    return;
  render_widget_->OnRequestTextInputStateUpdate();
}

void WidgetInputHandlerImpl::RequestCompositionUpdates(bool immediate_request,
                                                       bool monitor_request) {
  if (ShouldProxyToMainThread()) {
    RunOnMainThread(
        base::Bind(&WidgetInputHandlerImpl::RequestCompositionUpdates,
                   weak_this_, immediate_request, monitor_request));
    return;
  }
  if (!render_widget_)
    return;
  render_widget_->OnRequestCompositionUpdates(immediate_request,
                                              monitor_request);
}

void WidgetInputHandlerImpl::DispatchEvent(
    std::unique_ptr<content::InputEvent> event,
    DispatchEventCallback callback) {
  if (!event || !event->web_event) {
    return;
  }
  if (has_compositor_) {
    CHECK(!main_thread_task_runner_->BelongsToCurrentThread());
    input_handler_proxy_->HandleInputEventWithLatencyInfo(
        std::move(event->web_event), event->latency_info,
        base::BindOnce(
            &WidgetInputHandlerImpl::DidHandleInputEventAndOverscroll,
            base::Unretained(this), std::move(callback)));
  } else {
    RunOnMainThread(base::BindOnce(&WidgetInputHandlerImpl::HandleInputEvent,
                                   weak_this_, std::move(event->web_event),
                                   event->latency_info, std::move(callback)));
  }
}

void WidgetInputHandlerImpl::DispatchNonBlockingEvent(
    std::unique_ptr<content::InputEvent> event) {
  DispatchEvent(std::move(event), DispatchEventCallback());
}

void WidgetInputHandlerImpl::HandleInputEvent(
    const ui::WebScopedInputEvent& event,
    const ui::LatencyInfo& latency,
    DispatchEventCallback callback) {
  if (!render_widget_)
    return;
  auto send_callback =
      base::BindOnce(&WidgetInputHandlerImpl::HandledInputEvent, weak_this_,
                     std::move(callback));

  blink::WebCoalescedInputEvent coalesced_event(*event);
  render_widget_->HandleInputEvent(coalesced_event, latency,
                                   std::move(send_callback));
}

static void CallCallback(
    mojom::WidgetInputHandler::DispatchEventCallback callback,
    InputEventAckState ack_state,
    const ui::LatencyInfo& latency_info,
    std::unique_ptr<ui::DidOverscrollParams> overscroll_params) {
  std::move(callback).Run(
      InputEventAckSource::MAIN_THREAD, latency_info, ack_state,
      overscroll_params
          ? base::Optional<ui::DidOverscrollParams>(*overscroll_params)
          : base::nullopt);
}

void WidgetInputHandlerImpl::HandledInputEvent(
    DispatchEventCallback callback,
    InputEventAckState ack_state,
    const ui::LatencyInfo& latency_info,
    std::unique_ptr<ui::DidOverscrollParams> overscroll_params) {
  if (!callback)
    return;
  if (has_compositor_) {
    compositor_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(CallCallback, std::move(callback), ack_state,
                                  latency_info, std::move(overscroll_params)));
  } else {
    std::move(callback).Run(
        InputEventAckSource::COMPOSITOR_THREAD, latency_info, ack_state,
        overscroll_params
            ? base::Optional<ui::DidOverscrollParams>(*overscroll_params)
            : base::nullopt);
  }
}

void WidgetInputHandlerImpl::RunOnMainThread(base::OnceClosure closure) {
  if (input_event_queue_) {
    input_event_queue_->QueueClosure(std::move(closure));
  } else {
    std::move(closure).Run();
  }
}

void WidgetInputHandlerImpl::Release() {
  if (!main_thread_task_runner_->BelongsToCurrentThread()) {
    // Close the binding on the compositor thread first before telling the main
    // thread to delete this object.
    associated_binding_.Close();
    main_thread_task_runner_->PostTask(
        FROM_HERE, base::Bind(&WidgetInputHandlerImpl::Release, weak_this_));
    return;
  }
  //  delete this;
}

void WidgetInputHandlerImpl::BindNow(
    mojom::WidgetInputHandlerAssociatedRequest request) {
  associated_binding_.Bind(std::move(request));
  associated_binding_.set_connection_error_handler(
      base::Bind(&WidgetInputHandlerImpl::Release, base::Unretained(this)));
}

void WidgetInputHandlerImpl::BindNow2(
    mojom::WidgetInputHandlerRequest request) {
  binding_.Bind(std::move(request));
  binding_.set_connection_error_handler(
      base::Bind(&WidgetInputHandlerImpl::Release, base::Unretained(this)));
}

void WidgetInputHandlerImpl::WillShutdown() {
  // Close the associated binding if there is one
  associated_binding_.Close();
}

void WidgetInputHandlerImpl::TransferActiveWheelFlingAnimation(
    const blink::WebActiveWheelFlingParameters& params) {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&RenderWidget::TransferActiveWheelFlingAnimation,
                            render_widget_, params));
}

void WidgetInputHandlerImpl::DispatchNonBlockingEventToMainThread(
    ui::WebScopedInputEvent event,
    const ui::LatencyInfo& latency_info) {
  DCHECK(input_event_queue_);
  input_event_queue_->HandleEvent(
      std::move(event), latency_info, DISPATCH_TYPE_NON_BLOCKING,
      INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING, HandledEventCallback());
}

std::unique_ptr<blink::WebGestureCurve>
WidgetInputHandlerImpl::CreateFlingAnimationCurve(
    blink::WebGestureDevice device_source,
    const blink::WebFloatPoint& velocity,
    const blink::WebSize& cumulative_scroll) {
  return blink::Platform::Current()->CreateFlingAnimationCurve(
      device_source, velocity, cumulative_scroll);
}

void WidgetInputHandlerImpl::DidOverscroll(
    const gfx::Vector2dF& accumulated_overscroll,
    const gfx::Vector2dF& latest_overscroll_delta,
    const gfx::Vector2dF& current_fling_velocity,
    const gfx::PointF& causal_event_viewport_point) {
  ui::DidOverscrollParams params;
  params.accumulated_overscroll = accumulated_overscroll;
  params.latest_overscroll_delta = latest_overscroll_delta;
  params.current_fling_velocity = current_fling_velocity;
  params.causal_event_viewport_point = causal_event_viewport_point;
  if (legacy_host_message_sender_) {
    legacy_host_message_sender_->Send(new InputHostMsg_DidOverscroll(
        legacy_host_message_routing_id_, params));
  }
}

void WidgetInputHandlerImpl::DidStopFlinging() {
  if (legacy_host_message_sender_) {
    legacy_host_message_sender_->Send(
        new InputHostMsg_DidStopFlinging(legacy_host_message_routing_id_));
  }
}

void WidgetInputHandlerImpl::DidAnimateForInput() {
  renderer_scheduler_->DidAnimateForInputOnCompositorThread();
}

void WidgetInputHandlerImpl::GenerateScrollBeginAndSendToMainThread(
    const blink::WebGestureEvent& update_event) {
  DCHECK_EQ(update_event.GetType(), blink::WebInputEvent::kGestureScrollUpdate);
  blink::WebGestureEvent scroll_begin(update_event);
  scroll_begin.SetType(blink::WebInputEvent::kGestureScrollBegin);
  scroll_begin.data.scroll_begin.inertial_phase =
      update_event.data.scroll_update.inertial_phase;
  scroll_begin.data.scroll_begin.delta_x_hint =
      update_event.data.scroll_update.delta_x;
  scroll_begin.data.scroll_begin.delta_y_hint =
      update_event.data.scroll_update.delta_y;
  scroll_begin.data.scroll_begin.delta_hint_units =
      update_event.data.scroll_update.delta_units;

  DispatchNonBlockingEventToMainThread(
      ui::WebInputEventTraits::Clone(scroll_begin), ui::LatencyInfo());
}

void WidgetInputHandlerImpl::DidHandleInputEventAndOverscroll(
    DispatchEventCallback callback,
    ui::InputHandlerProxy::EventDisposition event_disposition,
    ui::WebScopedInputEvent input_event,
    const ui::LatencyInfo& latency_info,
    std::unique_ptr<ui::DidOverscrollParams> overscroll_params) {
  InputEventAckState ack_state = InputEventDispositionToAck(event_disposition);
  switch (ack_state) {
    case INPUT_EVENT_ACK_STATE_CONSUMED:
      renderer_scheduler_->DidHandleInputEventOnCompositorThread(
          *input_event, blink::scheduler::RendererScheduler::InputEventState::
                            EVENT_CONSUMED_BY_COMPOSITOR);
      break;
    case INPUT_EVENT_ACK_STATE_NOT_CONSUMED:
    case INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING_DUE_TO_FLING:
      renderer_scheduler_->DidHandleInputEventOnCompositorThread(
          *input_event, blink::scheduler::RendererScheduler::InputEventState::
                            EVENT_FORWARDED_TO_MAIN_THREAD);
      break;
    default:
      break;
  }
  if (ack_state == INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING ||
      ack_state == INPUT_EVENT_ACK_STATE_SET_NON_BLOCKING_DUE_TO_FLING ||
      ack_state == INPUT_EVENT_ACK_STATE_NOT_CONSUMED) {
    DCHECK(!overscroll_params);
    InputEventDispatchType dispatch_type = callback.is_null()
                                               ? DISPATCH_TYPE_NON_BLOCKING
                                               : DISPATCH_TYPE_BLOCKING;

    HandledEventCallback handled_event =
        base::BindOnce(&WidgetInputHandlerImpl::HandledInputEvent,
                       base::Unretained(this), std::move(callback));
    input_event_queue_->HandleEvent(std::move(input_event), latency_info,
                                    dispatch_type, ack_state,
                                    std::move(handled_event));
    return;
  }
  if (callback) {
    std::move(callback).Run(
        InputEventAckSource::COMPOSITOR_THREAD, latency_info, ack_state,
        overscroll_params
            ? base::Optional<ui::DidOverscrollParams>(*overscroll_params)
            : base::nullopt);
  }
}

void WidgetInputHandlerImpl::ObserveGestureEvent(
    const blink::WebGestureEvent& gesture_event,
    const cc::InputHandlerScrollResult& scroll_result) {
  if (!main_thread_task_runner_->BelongsToCurrentThread()) {
    compositor_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&WidgetInputHandlerImpl::ObserveGestureEvent,
                       base::Unretained(this), gesture_event, scroll_result));
    return;
  }
  DCHECK(input_handler_proxy_->scroll_elasticity_controller());
  input_handler_proxy_->scroll_elasticity_controller()
      ->ObserveGestureEventAndResult(gesture_event, scroll_result);
}
}  // namespace content
