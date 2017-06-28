// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_WIDGET_INPUT_HANDLER_IMPL_H_
#define CONTENT_RENDERER_INPUT_WIDGET_INPUT_HANDLER_IMPL_H_

#include "content/common/input/input_handler.mojom.h"
#include "content/renderer/render_frame_impl.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "ui/events/blink/input_handler_proxy.h"
#include "ui/events/blink/input_handler_proxy_client.h"

namespace blink {
namespace scheduler {
class RendererScheduler;
};  // namespace scheduler
};  // namespace blink

namespace content {
class MainThreadEventQueue;

class WidgetInputHandlerImpl : public mojom::WidgetInputHandler,
                               public ui::InputHandlerProxyClient {
 public:
  WidgetInputHandlerImpl(
      base::WeakPtr<RenderWidget> render_widget,
      blink::scheduler::RendererScheduler* renderer_scheduler);
  ~WidgetInputHandlerImpl() override;

  void AddAssociatedInterface(
      mojom::WidgetInputHandlerAssociatedRequest interface_request);

  void AddInterface(mojom::WidgetInputHandlerRequest interface_request);

  void SetFocus(bool focused) override;
  void MouseCaptureLost() override;
  void SetEditCommandsForNextKeyEvent(
      const std::vector<EditCommand>& commands) override;
  void CursorVisibilityChanged(bool visible) override;
  void ImeSetComposition(
      const base::string16& text,
      const std::vector<ui::CompositionUnderline>& underlines,
      const gfx::Range& range,
      int32_t start,
      int32_t end) override;
  void ImeCommitText(const base::string16& text,
                     const std::vector<ui::CompositionUnderline>& underlines,
                     const gfx::Range& range,
                     int32_t relative_cursor_position) override;
  void ImeFinishComposingText(bool keep_selection) override;
  void RequestTextInputStateUpdate() override;
  void RequestCompositionUpdates(bool immediate_request,
                                 bool monitor_request) override;
  void DispatchEvent(std::unique_ptr<content::InputEvent>,
                     DispatchEventCallback callback) override;
  void DispatchNonBlockingEvent(std::unique_ptr<content::InputEvent>) override;

  // InputHandlerProxyClient overrides.
  void WillShutdown() override;
  void TransferActiveWheelFlingAnimation(
      const blink::WebActiveWheelFlingParameters& params) override;
  void DispatchNonBlockingEventToMainThread(
      ui::WebScopedInputEvent event,
      const ui::LatencyInfo& latency_info) override;
  std::unique_ptr<blink::WebGestureCurve> CreateFlingAnimationCurve(
      blink::WebGestureDevice device_source,
      const blink::WebFloatPoint& velocity,
      const blink::WebSize& cumulative_scroll) override;

  void DidOverscroll(const gfx::Vector2dF& accumulated_overscroll,
                     const gfx::Vector2dF& latest_overscroll_delta,
                     const gfx::Vector2dF& current_fling_velocity,
                     const gfx::PointF& causal_event_viewport_point) override;
  void DidStopFlinging() override;
  void DidAnimateForInput() override;
  void GenerateScrollBeginAndSendToMainThread(
      const blink::WebGestureEvent& update_event) override;

  void ObserveGestureEvent(const blink::WebGestureEvent& gesture_event,
                           const cc::InputHandlerScrollResult& scroll_result);

 private:
  bool ShouldProxyToMainThread() const;
  void InitOnCompositorThread(
      const base::WeakPtr<cc::InputHandler>& input_handler,
      bool smooth_scroll_enabled);
  void RunOnMainThread(base::OnceClosure closure);
  void BindNow(mojom::WidgetInputHandlerAssociatedRequest request);
  void BindNow2(mojom::WidgetInputHandlerRequest request);
  void Release();
  void HandledInputEvent(DispatchEventCallback callback,
                         InputEventAckState ack_state,
                         const ui::LatencyInfo& latency_info,
                         std::unique_ptr<ui::DidOverscrollParams>);
  void HandleInputEvent(const ui::WebScopedInputEvent& event,
                        const ui::LatencyInfo& latency,
                        DispatchEventCallback callback);
  void DidHandleInputEventAndOverscroll(
      DispatchEventCallback callback,
      ui::InputHandlerProxy::EventDisposition event_disposition,
      ui::WebScopedInputEvent input_event,
      const ui::LatencyInfo& latency_info,
      std::unique_ptr<ui::DidOverscrollParams> overscroll_params);

  base::WeakPtr<RenderWidget> render_widget_;
  blink::scheduler::RendererScheduler* renderer_scheduler_;
  scoped_refptr<MainThreadEventQueue> input_event_queue_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;

  std::unique_ptr<ui::InputHandlerProxy> input_handler_proxy_;
  mojo::Binding<mojom::WidgetInputHandler> binding_;
  mojo::AssociatedBinding<mojom::WidgetInputHandler> associated_binding_;
  bool has_compositor_;
  IPC::Sender* legacy_host_message_sender_;
  int legacy_host_message_routing_id_;
  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

  base::WeakPtr<WidgetInputHandlerImpl> weak_this_;
  base::WeakPtrFactory<WidgetInputHandlerImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WidgetInputHandlerImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INPUT_WIDGET_INPUT_HANDLER_IMPL_H_
