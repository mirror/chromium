// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_targeter.h"

#include "content/browser/renderer_host/input/timeout_monitor.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "ui/events/blink/blink_event_util.h"

namespace content {

namespace {

bool MergeEventIfPossible(const blink::WebInputEvent& event,
                          ui::WebScopedInputEvent* blink_event) {
  if (ui::CanCoalesce(event, **blink_event)) {
    *blink_event = ui::WebInputEventTraits::Clone(event);
    return true;
  }
  return false;
}

}  // namespace

RenderWidgetTargetResult::RenderWidgetTargetResult() = default;

RenderWidgetTargetResult::RenderWidgetTargetResult(
    const RenderWidgetTargetResult&) = default;

RenderWidgetTargetResult::RenderWidgetTargetResult(
    RenderWidgetHostViewBase* in_view,
    bool in_should_query_view,
    base::Optional<gfx::PointF> in_location)
    : view(in_view),
      should_query_view(in_should_query_view),
      target_location(in_location) {}

RenderWidgetTargetResult::~RenderWidgetTargetResult() = default;

RenderWidgetTargeter::TargetingRequest::TargetingRequest() = default;

RenderWidgetTargeter::TargetingRequest::TargetingRequest(
    TargetingRequest&& request) = default;

RenderWidgetTargeter::TargetingRequest& RenderWidgetTargeter::TargetingRequest::
operator=(TargetingRequest&&) = default;

RenderWidgetTargeter::TargetingRequest::~TargetingRequest() = default;

RenderWidgetTargeter::RenderWidgetTargeter(Delegate* delegate)
    : delegate_(delegate), weak_ptr_factory_(this) {
  DCHECK(delegate_);
}

RenderWidgetTargeter::~RenderWidgetTargeter() = default;

void RenderWidgetTargeter::FindTargetAndDispatch(
    RenderWidgetHostViewBase* root_view,
    const blink::WebInputEvent& event,
    const ui::LatencyInfo& latency) {
  if (request_in_flight_) {
    if (!requests_.empty()) {
      auto& request = requests_.back();
      if (MergeEventIfPossible(event, &request.event))
        return;
    }
    TargetingRequest request;
    request.root_view = root_view->GetWeakPtr();
    request.event = ui::WebInputEventTraits::Clone(event);
    request.latency = latency;
    requests_.push(std::move(request));
    return;
  }

  // TODO(kenrb, sadrul): When all event types support asynchronous hit
  // testing, we should be able to remove the following and instead
  // have FindTargetSynchronously return the view and location to use for the
  // renderer hit test query. Currently it has to return the surface hit test
  // target, for event types that ignore |result.should_query_view|.
  gfx::PointF root_location;
  if (blink::WebInputEvent::IsMouseEventType(event.GetType())) {
    auto mouse_event = static_cast<const blink::WebMouseEvent&>(event);
    root_location = mouse_event.PositionInWidget();
  }
  if (event.GetType() == blink::WebInputEvent::kMouseWheel) {
    auto mouse_wheel_event =
        static_cast<const blink::WebMouseWheelEvent&>(event);
    root_location = mouse_wheel_event.PositionInWidget();
  }
  if (blink::WebInputEvent::IsTouchEventType(event.GetType())) {
    auto touch_event = static_cast<const blink::WebTouchEvent&>(event);
    root_location = touch_event.touches[0].PositionInWidget();
  }
  if (blink::WebInputEvent::IsGestureEventType(event.GetType())) {
    auto gesture_event = static_cast<const blink::WebGestureEvent&>(event);
    root_location = gesture_event.PositionInWidget();
  }

  RenderWidgetTargetResult result =
      delegate_->FindTargetSynchronously(root_view, event);

  RenderWidgetHostViewBase* target = result.view;
  auto* event_ptr = &event;
  if (result.should_query_view) {
    QueryClient(root_view, root_view, *event_ptr, latency, root_location,
                nullptr, gfx::PointF());
  } else {
    FoundTarget(root_view, target, *event_ptr, latency, result.target_location);
  }
}

void RenderWidgetTargeter::ViewWillBeDestroyed(RenderWidgetHostViewBase* view) {
  unresponsive_views_.erase(view);
}

void RenderWidgetTargeter::QueryClient(
    RenderWidgetHostViewBase* root_view,
    RenderWidgetHostViewBase* target,
    const blink::WebInputEvent& event,
    const ui::LatencyInfo& latency,
    const base::Optional<gfx::PointF>& target_location,
    RenderWidgetHostViewBase* last_request_target,
    const gfx::PointF& last_target_location) {
  DCHECK(!request_in_flight_);

  request_in_flight_ = true;
  auto* target_client =
      target->GetRenderWidgetHostImpl()->input_target_client();
  // TODO: Unify the codepaths by converting to ui::WebScopedInputEvent here (or
  // earlier).
  if (blink::WebInputEvent::IsMouseEventType(event.GetType())) {
    async_hit_test_timeout_.reset(new TimeoutMonitor(base::Bind(
        &RenderWidgetTargeter::AsyncHitTestTimedOut,
        weak_ptr_factory_.GetWeakPtr(), root_view->GetWeakPtr(),
        target->GetWeakPtr(), *target_location,
        last_request_target ? last_request_target->GetWeakPtr()
                            : base::WeakPtr<RenderWidgetHostViewBase>(),
        last_target_location, static_cast<const blink::WebMouseEvent&>(event),
        latency)));
    target_client->FrameSinkIdAt(
        gfx::ToCeiledPoint(target_location.value()),
        base::BindOnce(&RenderWidgetTargeter::FoundFrameSinkId,
                       weak_ptr_factory_.GetWeakPtr(), root_view->GetWeakPtr(),
                       target->GetWeakPtr(),
                       static_cast<const blink::WebMouseEvent&>(event), latency,
                       ++last_request_id_, target_location));
  } else if (event.GetType() == blink::WebInputEvent::kMouseWheel) {
    async_hit_test_timeout_.reset(new TimeoutMonitor(base::Bind(
        &RenderWidgetTargeter::AsyncHitTestTimedOut,
        weak_ptr_factory_.GetWeakPtr(), root_view->GetWeakPtr(),
        target->GetWeakPtr(), *target_location,
        last_request_target ? last_request_target->GetWeakPtr()
                            : base::WeakPtr<RenderWidgetHostViewBase>(),
        last_target_location,
        static_cast<const blink::WebMouseWheelEvent&>(event), latency)));
    target_client->FrameSinkIdAt(
        gfx::ToCeiledPoint(target_location.value()),
        base::BindOnce(&RenderWidgetTargeter::FoundFrameSinkId,
                       weak_ptr_factory_.GetWeakPtr(), root_view->GetWeakPtr(),
                       target->GetWeakPtr(),
                       static_cast<const blink::WebMouseWheelEvent&>(event),
                       latency, ++last_request_id_, target_location));
  } else if (blink::WebInputEvent::IsTouchEventType(event.GetType())) {
    auto touch_event = static_cast<const blink::WebTouchEvent&>(event);
    DCHECK(touch_event.GetType() == blink::WebInputEvent::kTouchStart);
    async_hit_test_timeout_.reset(new TimeoutMonitor(base::Bind(
        &RenderWidgetTargeter::AsyncHitTestTimedOut,
        weak_ptr_factory_.GetWeakPtr(), root_view->GetWeakPtr(),
        target->GetWeakPtr(), *target_location,
        last_request_target ? last_request_target->GetWeakPtr()
                            : base::WeakPtr<RenderWidgetHostViewBase>(),
        last_target_location, static_cast<const blink::WebTouchEvent&>(event),
        latency)));
    target_client->FrameSinkIdAt(
        gfx::ToCeiledPoint(target_location.value()),
        base::BindOnce(&RenderWidgetTargeter::FoundFrameSinkId,
                       weak_ptr_factory_.GetWeakPtr(), root_view->GetWeakPtr(),
                       target->GetWeakPtr(),
                       static_cast<const blink::WebTouchEvent&>(event), latency,
                       ++last_request_id_, target_location));
  } else if (blink::WebInputEvent::IsGestureEventType(event.GetType())) {
    auto gesture_event = static_cast<const blink::WebGestureEvent&>(event);
    DCHECK(gesture_event.source_device ==
               blink::WebGestureDevice::kWebGestureDeviceTouchscreen ||
           gesture_event.source_device ==
               blink::WebGestureDevice::kWebGestureDeviceTouchpad);
    async_hit_test_timeout_.reset(new TimeoutMonitor(base::Bind(
        &RenderWidgetTargeter::AsyncHitTestTimedOut,
        weak_ptr_factory_.GetWeakPtr(), root_view->GetWeakPtr(),
        target->GetWeakPtr(), *target_location,
        last_request_target ? last_request_target->GetWeakPtr()
                            : base::WeakPtr<RenderWidgetHostViewBase>(),
        last_target_location, static_cast<const blink::WebGestureEvent&>(event),
        latency)));
    target_client->FrameSinkIdAt(
        gfx::ToCeiledPoint(target_location.value()),
        base::BindOnce(&RenderWidgetTargeter::FoundFrameSinkId,
                       weak_ptr_factory_.GetWeakPtr(), root_view->GetWeakPtr(),
                       target->GetWeakPtr(),
                       static_cast<const blink::WebGestureEvent&>(event),
                       latency, ++last_request_id_, target_location));
  } else {
    // TODO(crbug.com/796656): Handle other types of events.
    NOTREACHED();
  }
  async_hit_test_timeout_->Start(async_hit_test_timeout_delay_);
}

void RenderWidgetTargeter::FlushEventQueue() {
  while (!request_in_flight_ && !requests_.empty()) {
    auto request = std::move(requests_.front());
    requests_.pop();
    // The root-view has gone away. Ignore this event, and try to process the
    // next event.
    if (!request.root_view) {
      continue;
    }
    FindTargetAndDispatch(request.root_view.get(), *request.event,
                          request.latency);
  }
}

void RenderWidgetTargeter::FoundFrameSinkId(
    base::WeakPtr<RenderWidgetHostViewBase> root_view,
    base::WeakPtr<RenderWidgetHostViewBase> target,
    const blink::WebInputEvent& event,
    const ui::LatencyInfo& latency,
    uint32_t request_id,
    const base::Optional<gfx::PointF>& target_location,
    const viz::FrameSinkId& frame_sink_id) {
  if (request_id != last_request_id_ || !request_in_flight_) {
    // This is a response to a request that already timed out, so the event
    // should have already been dispatched. Mark the renderer as responsive
    // and otherwise ignore this response.
    unresponsive_views_.erase(target.get());
    return;
  }

  request_in_flight_ = false;
  async_hit_test_timeout_.reset(nullptr);
  auto* view = delegate_->FindViewFromFrameSinkId(frame_sink_id);
  if (!view)
    view = target.get();

  // If a client was asked to find a target, then it is necessary to keep
  // asking the clients until a client claims an event for itself.
  if (view == target.get() ||
      unresponsive_views_.find(view) != unresponsive_views_.end()) {
    FoundTarget(root_view.get(), view, event, latency, target_location);
  } else {
    base::Optional<gfx::PointF> location = target_location;
    if (target_location) {
      gfx::PointF updated_location = *target_location;
      if (target->TransformPointToCoordSpaceForView(updated_location, view,
                                                    &updated_location)) {
        location.emplace(updated_location);
      }
    }
    QueryClient(root_view.get(), view, event, latency, location, target.get(),
                *target_location);
  }
}

void RenderWidgetTargeter::FoundTarget(
    RenderWidgetHostViewBase* root_view,
    RenderWidgetHostViewBase* target,
    const blink::WebInputEvent& event,
    const ui::LatencyInfo& latency,
    const base::Optional<gfx::PointF>& target_location) {
  if (!root_view)
    return;
  // TODO: Unify position conversion for all event types.
  if (blink::WebInputEvent::IsMouseEventType(event.GetType())) {
    blink::WebMouseEvent mouse_event =
        static_cast<const blink::WebMouseEvent&>(event);
    if (target_location.has_value()) {
      mouse_event.SetPositionInWidget(target_location->x(),
                                      target_location->y());
    }
    if (mouse_event.GetType() != blink::WebInputEvent::kUndefined)
      delegate_->DispatchEventToTarget(root_view, target, mouse_event, latency);
  } else if (event.GetType() == blink::WebInputEvent::kMouseWheel ||
             blink::WebInputEvent::IsTouchEventType(event.GetType()) ||
             blink::WebInputEvent::IsGestureEventType(event.GetType())) {
    DCHECK(!blink::WebInputEvent::IsGestureEventType(event.GetType()) ||
           (static_cast<const blink::WebGestureEvent&>(event).source_device ==
                blink::WebGestureDevice::kWebGestureDeviceTouchscreen ||
            static_cast<const blink::WebGestureEvent&>(event).source_device ==
                blink::WebGestureDevice::kWebGestureDeviceTouchpad));
    delegate_->DispatchEventToTarget(root_view, target, event, latency);
  } else {
    // TODO(crbug.com/796656): Handle other types of events.
    NOTREACHED();
    return;
  }
  FlushEventQueue();
}

void RenderWidgetTargeter::AsyncHitTestTimedOut(
    base::WeakPtr<RenderWidgetHostViewBase> current_request_root_view,
    base::WeakPtr<RenderWidgetHostViewBase> current_request_target,
    const gfx::PointF& current_target_location,
    base::WeakPtr<RenderWidgetHostViewBase> last_request_target,
    const gfx::PointF& last_target_location,
    const blink::WebInputEvent& event,
    const ui::LatencyInfo& latency) {
  DCHECK(request_in_flight_);
  request_in_flight_ = false;

  if (!current_request_root_view)
    return;

  // Mark view as unresponsive so further events will not be sent to it.
  if (current_request_target)
    unresponsive_views_.insert(current_request_target.get());

  if (current_request_root_view.get() == current_request_target.get()) {
    // When a request to the top-level frame times out then the event gets
    // sent there anyway. It will trigger the hung renderer dialog if the
    // renderer fails to process it.
    FoundTarget(current_request_root_view.get(),
                current_request_root_view.get(), event, latency,
                current_target_location);
  } else {
    FoundTarget(current_request_root_view.get(), last_request_target.get(),
                event, latency, last_target_location);
  }
}

}  // namespace content
