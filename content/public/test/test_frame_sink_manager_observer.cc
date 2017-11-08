// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_frame_sink_manager_observer.h"

#include "base/test/test_timeouts.h"
#include "content/browser/frame_host/cross_process_frame_connector.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/renderer_host/frame_connector_delegate.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/renderer_host/render_widget_host_view_child_frame.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/constants.mojom.h"

#if defined(USE_AURA)
#include "content/browser/renderer_host/render_widget_host_view_aura.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#endif  // defined(USE_AURA)

namespace {

bool IsMus() {
#if defined(USE_AURA)
  return aura::Env::GetInstance()->mode() == aura::Env::Mode::MUS;
#else
  return false;
#endif  // defined(USE_AURA)
}

}  // namespace

namespace content {

TestFrameSinkManagerObserver::TestFrameSinkManagerObserver() : binding_(this) {
  service_manager::Connector* connector =
      ServiceManagerConnection::GetForProcess()->GetConnector();
  CHECK(connector);
  if (IsMus()) {
    connector->BindInterface(ui::mojom::kServiceName,
                             &frame_sink_manager_test_connector_ptr_);
  } else {
    connector->BindInterface(content::mojom::kBrowserServiceName,
                             &frame_sink_manager_test_connector_ptr_);
  }

  viz::mojom::FrameSinkManagerTestObserverPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  frame_sink_manager_test_connector_ptr_->AddObserver(std::move(ptr));
}

TestFrameSinkManagerObserver::~TestFrameSinkManagerObserver() {}

void TestFrameSinkManagerObserver::WaitForChildFrameSurfaceReady(
    content::RenderFrameHost* child_frame) {
  RenderWidgetHostViewBase* child_view =
      static_cast<RenderFrameHostImpl*>(child_frame)
          ->GetRenderWidgetHost()
          ->GetView();
  if (!child_view || !child_view->IsRenderWidgetHostViewChildFrame())
    return;

  RenderWidgetHostViewBase* root_view =
      static_cast<CrossProcessFrameConnector*>(
          static_cast<RenderWidgetHostViewChildFrame*>(child_view)
              ->FrameConnectorForTesting())
          ->GetRootRenderWidgetHostViewForTesting();

  WaitForSurfaceReady(root_view);
}

void TestFrameSinkManagerObserver::WaitForSurfaceReady(
    RenderWidgetHostViewBase* root_view) {
  viz::FrameSinkId frame_sink_id = root_view->GetFrameSinkId();
#if defined(USE_AURA)
  if (IsMus()) {
    RenderWidgetHostViewAura* aura_root_view =
        static_cast<RenderWidgetHostViewAura*>(root_view);
    if (aura_root_view->window()) {
      frame_sink_id = aura_root_view->window()->GetFrameSinkId();
    }
  }
#endif  // defined(USE_AURA)
  while (surfaces_.find(frame_sink_id) == surfaces_.end()) {
    base::RunLoop run_loop;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, run_loop.QuitClosure(), TestTimeouts::tiny_timeout());
    run_loop.Run();
  }
}

// mojom::FrameSinkManagerObserver:
void TestFrameSinkManagerObserver::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info) {
  surfaces_.insert(std::pair<viz::FrameSinkId, viz::SurfaceInfo>(
      surface_info.id().frame_sink_id(), viz::SurfaceInfo(surface_info)));
}

}  // namespace content
