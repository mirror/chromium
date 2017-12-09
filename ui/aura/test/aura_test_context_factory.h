// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_AURA_TEST_CONTEXT_FACTORY_H_
#define UI_AURA_TEST_AURA_TEST_CONTEXT_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/viz/common/surfaces/frame_sink_id_allocator.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "ui/compositor/test/fake_context_factory.h"

namespace viz {
class TestLayerTreeFrameSinkClient;
}

namespace aura {
namespace test {

class AuraTestContextFactory : public ui::FakeContextFactory,
                               public ui::ContextFactoryPrivate {
 public:
  AuraTestContextFactory();
  ~AuraTestContextFactory() override;

  // ui::FakeContextFactory
  void CreateLayerTreeFrameSink(
      base::WeakPtr<ui::Compositor> compositor) override;

  // ui::ContextFactoryPrivate:
  std::unique_ptr<ui::Reflector> CreateReflector(
      ui::Compositor* mirrored_compositor,
      ui::Layer* mirroring_layer) override;
  void RemoveReflector(ui::Reflector* reflector) override;
  viz::FrameSinkId AllocateFrameSinkId() override;
  viz::FrameSinkManagerImpl* GetFrameSinkManager() override;
  viz::HostFrameSinkManager* GetHostFrameSinkManager() override;
  void SetDisplayVisible(ui::Compositor* compositor, bool visible) override;
  void ResizeDisplay(ui::Compositor* compositor,
                     const gfx::Size& size) override;
  void SetDisplayColorMatrix(ui::Compositor* compositor,
                             const SkMatrix44& matrix) override;
  void SetDisplayColorSpace(ui::Compositor* compositor,
                            const gfx::ColorSpace& blending_color_space,
                            const gfx::ColorSpace& output_color_space) override;
  void SetAuthoritativeVSyncInterval(ui::Compositor* compositor,
                                     base::TimeDelta interval) override;
  void SetDisplayVSyncParameters(ui::Compositor* compositor,
                                 base::TimeTicks timebase,
                                 base::TimeDelta interval) override;
  void IssueExternalBeginFrame(ui::Compositor* compositor,
                               const viz::BeginFrameArgs& args) override;
  void SetOutputIsSecure(ui::Compositor* compositor, bool secure) override;

 private:
  std::set<std::unique_ptr<viz::TestLayerTreeFrameSinkClient>>
      frame_sink_clients_;
  viz::FrameSinkManagerImpl frame_sink_manager_impl_;
  viz::HostFrameSinkManager host_frame_sink_manager_;
  viz::FrameSinkIdAllocator frame_sink_id_allocator_;

  DISALLOW_COPY_AND_ASSIGN(AuraTestContextFactory);
};

}  // namespace test
}  // namespace aura

#endif  // UI_AURA_TEST_AURA_TEST_CONTEXT_FACTORY_H_<
