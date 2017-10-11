// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/display.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/test/null_task_runner.h"
#include "cc/test/fake_output_surface.h"
#include "cc/test/scheduler_test_common.h"
#include "cc/test/test_shared_bitmap_manager.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/common/quads/solid_color_draw_quad.h"
#include "components/viz/common/resources/shared_bitmap_manager.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/common/surfaces/local_surface_id_allocator.h"
#include "components/viz/service/display/display_client.h"
#include "components/viz/service/display/display_scheduler.h"
#include "components/viz/service/display/texture_mailbox_deleter.h"
#include "components/viz/service/frame_sinks/compositor_frame_sink_support.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/service/surfaces/surface.h"
#include "components/viz/service/surfaces/surface_manager.h"
#include "components/viz/test/compositor_frame_helpers.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::AnyNumber;

namespace viz {
namespace {

static constexpr FrameSinkId kArbitraryFrameSinkId(3, 3);
static constexpr FrameSinkId kAnotherFrameSinkId(4, 4);

class TestSoftwareOutputDevice : public SoftwareOutputDevice {
 public:
  TestSoftwareOutputDevice() {}

  gfx::Rect damage_rect() const { return damage_rect_; }
  gfx::Size viewport_pixel_size() const { return viewport_pixel_size_; }
};

class TestDisplayScheduler : public DisplayScheduler {
 public:
  explicit TestDisplayScheduler(BeginFrameSource* begin_frame_source,
                                base::SingleThreadTaskRunner* task_runner)
      : DisplayScheduler(begin_frame_source, task_runner, 1),
        damaged(false),
        display_resized_(false),
        has_new_root_surface(false),
        swapped(false) {}

  ~TestDisplayScheduler() override {}

  void DisplayResized() override { display_resized_ = true; }

  void SetNewRootSurface(const SurfaceId& root_surface_id) override {
    has_new_root_surface = true;
  }

  void ProcessSurfaceDamage(const SurfaceId& surface_id,
                            const BeginFrameAck& ack,
                            bool display_damaged) override {
    if (display_damaged) {
      damaged = true;
      needs_draw_ = true;
    }
  }

  void DidSwapBuffers() override { swapped = true; }

  void ResetDamageForTest() {
    damaged = false;
    display_resized_ = false;
    has_new_root_surface = false;
  }

  bool damaged;
  bool display_resized_;
  bool has_new_root_surface;
  bool swapped;
};

class DisplayTest : public testing::Test {
 public:
  DisplayTest()
      : support_(
            CompositorFrameSinkSupport::Create(nullptr,
                                               &manager_,
                                               kArbitraryFrameSinkId,
                                               true /* is_root */,
                                               true /* needs_sync_points */)),
        task_runner_(new base::NullTaskRunner) {}

  ~DisplayTest() override { support_->EvictCurrentSurface(); }

  void SetUpDisplay(const RendererSettings& settings,
                    std::unique_ptr<cc::TestWebGraphicsContext3D> context) {
    begin_frame_source_.reset(new StubBeginFrameSource);

    std::unique_ptr<cc::FakeOutputSurface> output_surface;
    if (context) {
      auto provider = cc::TestContextProvider::Create(std::move(context));
      provider->BindToCurrentThread();
      output_surface = cc::FakeOutputSurface::Create3d(std::move(provider));
    } else {
      auto device = base::MakeUnique<TestSoftwareOutputDevice>();
      software_output_device_ = device.get();
      output_surface = cc::FakeOutputSurface::CreateSoftware(std::move(device));
    }
    output_surface_ = output_surface.get();
    auto scheduler = base::MakeUnique<TestDisplayScheduler>(
        begin_frame_source_.get(), task_runner_.get());
    scheduler_ = scheduler.get();
    display_ = CreateDisplay(settings, kArbitraryFrameSinkId,
                             std::move(scheduler), std::move(output_surface));
    manager_.RegisterBeginFrameSource(begin_frame_source_.get(),
                                      kArbitraryFrameSinkId);
  }

  std::unique_ptr<Display> CreateDisplay(
      const RendererSettings& settings,
      const FrameSinkId& frame_sink_id,
      std::unique_ptr<DisplayScheduler> scheduler,
      std::unique_ptr<OutputSurface> output_surface) {
    auto display = base::MakeUnique<Display>(
        &shared_bitmap_manager_, nullptr /* gpu_memory_buffer_manager */,
        settings, frame_sink_id, std::move(output_surface),
        std::move(scheduler),
        base::MakeUnique<TextureMailboxDeleter>(task_runner_.get()));
    display->SetVisible(true);
    return display;
  }

  void TearDownDisplay() {
    // Only call UnregisterBeginFrameSource if SetupDisplay has been called.
    if (begin_frame_source_)
      manager_.UnregisterBeginFrameSource(begin_frame_source_.get());
  }

 protected:
  void SubmitCompositorFrame(RenderPassList* pass_list,
                             const LocalSurfaceId& local_surface_id) {
    CompositorFrame frame = test::MakeCompositorFrame();
    pass_list->swap(frame.render_pass_list);

    support_->SubmitCompositorFrame(local_surface_id, std::move(frame));
  }

  FrameSinkManagerImpl manager_;
  std::unique_ptr<CompositorFrameSinkSupport> support_;
  LocalSurfaceIdAllocator id_allocator_;
  scoped_refptr<base::NullTaskRunner> task_runner_;
  cc::TestSharedBitmapManager shared_bitmap_manager_;
  std::unique_ptr<BeginFrameSource> begin_frame_source_;
  std::unique_ptr<Display> display_;
  TestSoftwareOutputDevice* software_output_device_ = nullptr;
  cc::FakeOutputSurface* output_surface_ = nullptr;
  TestDisplayScheduler* scheduler_ = nullptr;
};

class StubDisplayClient : public DisplayClient {
 public:
  void DisplayOutputSurfaceLost() override {}
  void DisplayWillDrawAndSwap(bool will_draw_and_swap,
                              const RenderPassList& render_passes) override {}
  void DisplayDidDrawAndSwap() override {}
};

void CopyCallback(bool* called, std::unique_ptr<CopyOutputResult> result) {
  *called = true;
}

// Check that frame is damaged and swapped only under correct conditions.
TEST_F(DisplayTest, DisplayDamaged) {
  RendererSettings settings;
  settings.partial_swap_enabled = true;
  settings.finish_rendering_on_resize = true;
  SetUpDisplay(settings, nullptr);
  gfx::ColorSpace color_space_1 = gfx::ColorSpace::CreateXYZD50();
  gfx::ColorSpace color_space_2 = gfx::ColorSpace::CreateSCRGBLinear();

  StubDisplayClient client;
  display_->Initialize(&client, manager_.surface_manager());
  display_->SetColorSpace(color_space_1, color_space_1);

  LocalSurfaceId local_surface_id(id_allocator_.GenerateId());
  EXPECT_FALSE(scheduler_->damaged);
  EXPECT_FALSE(scheduler_->has_new_root_surface);
  display_->SetLocalSurfaceId(local_surface_id, 1.f);
  EXPECT_FALSE(scheduler_->damaged);
  EXPECT_FALSE(scheduler_->display_resized_);
  EXPECT_TRUE(scheduler_->has_new_root_surface);

  scheduler_->ResetDamageForTest();
  display_->Resize(gfx::Size(100, 100));
  EXPECT_FALSE(scheduler_->damaged);
  EXPECT_TRUE(scheduler_->display_resized_);
  EXPECT_FALSE(scheduler_->has_new_root_surface);

  // First draw from surface should have full damage.
  RenderPassList pass_list;
  auto pass = RenderPass::Create();
  pass->output_rect = gfx::Rect(0, 0, 100, 100);
  pass->damage_rect = gfx::Rect(10, 10, 1, 1);
  pass->id = 1u;
  pass_list.push_back(std::move(pass));

  scheduler_->ResetDamageForTest();
  SubmitCompositorFrame(&pass_list, local_surface_id);
  EXPECT_TRUE(scheduler_->damaged);
  EXPECT_FALSE(scheduler_->display_resized_);
  EXPECT_FALSE(scheduler_->has_new_root_surface);

  EXPECT_FALSE(scheduler_->swapped);
  EXPECT_EQ(0u, output_surface_->num_sent_frames());
  EXPECT_EQ(gfx::ColorSpace(), output_surface_->last_reshape_color_space());
  display_->DrawAndSwap();
  EXPECT_EQ(color_space_1, output_surface_->last_reshape_color_space());
  EXPECT_TRUE(scheduler_->swapped);
  EXPECT_EQ(1u, output_surface_->num_sent_frames());
  EXPECT_EQ(gfx::Size(100, 100),
            software_output_device_->viewport_pixel_size());
  EXPECT_EQ(gfx::Rect(0, 0, 100, 100), software_output_device_->damage_rect());
  {
    // Only damaged portion should be swapped.
    pass = RenderPass::Create();
    pass->output_rect = gfx::Rect(0, 0, 100, 100);
    pass->damage_rect = gfx::Rect(10, 10, 1, 1);
    pass->id = 1u;

    pass_list.push_back(std::move(pass));
    scheduler_->ResetDamageForTest();
    SubmitCompositorFrame(&pass_list, local_surface_id);
    EXPECT_TRUE(scheduler_->damaged);
    EXPECT_FALSE(scheduler_->display_resized_);
    EXPECT_FALSE(scheduler_->has_new_root_surface);

    scheduler_->swapped = false;
    EXPECT_EQ(color_space_1, output_surface_->last_reshape_color_space());
    display_->SetColorSpace(color_space_2, color_space_2);
    display_->DrawAndSwap();
    EXPECT_EQ(color_space_2, output_surface_->last_reshape_color_space());
    EXPECT_TRUE(scheduler_->swapped);
    EXPECT_EQ(2u, output_surface_->num_sent_frames());
    EXPECT_EQ(gfx::Size(100, 100),
              software_output_device_->viewport_pixel_size());
    EXPECT_EQ(gfx::Rect(10, 10, 1, 1), software_output_device_->damage_rect());
  }

  {
    // Pass has no damage so shouldn't be swapped.
    pass = RenderPass::Create();
    pass->output_rect = gfx::Rect(0, 0, 100, 100);
    pass->damage_rect = gfx::Rect(10, 10, 0, 0);
    pass->id = 1u;

    pass_list.push_back(std::move(pass));
    scheduler_->ResetDamageForTest();
    SubmitCompositorFrame(&pass_list, local_surface_id);
    EXPECT_TRUE(scheduler_->damaged);
    EXPECT_FALSE(scheduler_->display_resized_);
    EXPECT_FALSE(scheduler_->has_new_root_surface);

    scheduler_->swapped = false;
    display_->DrawAndSwap();
    EXPECT_TRUE(scheduler_->swapped);
    EXPECT_EQ(2u, output_surface_->num_sent_frames());
  }

  {
    // Pass is wrong size so shouldn't be swapped.
    pass = RenderPass::Create();
    pass->output_rect = gfx::Rect(0, 0, 99, 99);
    pass->damage_rect = gfx::Rect(10, 10, 10, 10);
    pass->id = 1u;

    local_surface_id = id_allocator_.GenerateId();
    display_->SetLocalSurfaceId(local_surface_id, 1.f);

    pass_list.push_back(std::move(pass));
    scheduler_->ResetDamageForTest();
    SubmitCompositorFrame(&pass_list, local_surface_id);
    EXPECT_TRUE(scheduler_->damaged);
    EXPECT_FALSE(scheduler_->display_resized_);
    EXPECT_FALSE(scheduler_->has_new_root_surface);

    scheduler_->swapped = false;
    display_->DrawAndSwap();
    EXPECT_TRUE(scheduler_->swapped);
    EXPECT_EQ(2u, output_surface_->num_sent_frames());
  }

  {
    // Previous frame wasn't swapped, so next swap should have full damage.
    pass = RenderPass::Create();
    pass->output_rect = gfx::Rect(0, 0, 100, 100);
    pass->damage_rect = gfx::Rect(10, 10, 0, 0);
    pass->id = 1u;

    local_surface_id = id_allocator_.GenerateId();
    display_->SetLocalSurfaceId(local_surface_id, 1.f);

    pass_list.push_back(std::move(pass));
    scheduler_->ResetDamageForTest();
    SubmitCompositorFrame(&pass_list, local_surface_id);
    EXPECT_TRUE(scheduler_->damaged);
    EXPECT_FALSE(scheduler_->display_resized_);
    EXPECT_FALSE(scheduler_->has_new_root_surface);

    scheduler_->swapped = false;
    display_->DrawAndSwap();
    EXPECT_TRUE(scheduler_->swapped);
    EXPECT_EQ(3u, output_surface_->num_sent_frames());
    EXPECT_EQ(gfx::Rect(0, 0, 100, 100),
              software_output_device_->damage_rect());
  }

  {
    // Pass has copy output request so should be swapped.
    pass = RenderPass::Create();
    pass->output_rect = gfx::Rect(0, 0, 100, 100);
    pass->damage_rect = gfx::Rect(10, 10, 0, 0);
    bool copy_called = false;
    pass->copy_requests.push_back(std::make_unique<CopyOutputRequest>(
        CopyOutputRequest::ResultFormat::RGBA_BITMAP,
        base::BindOnce(&CopyCallback, &copy_called)));
    pass->id = 1u;

    pass_list.push_back(std::move(pass));
    scheduler_->ResetDamageForTest();
    SubmitCompositorFrame(&pass_list, local_surface_id);
    EXPECT_TRUE(scheduler_->damaged);
    EXPECT_FALSE(scheduler_->display_resized_);
    EXPECT_FALSE(scheduler_->has_new_root_surface);

    scheduler_->swapped = false;
    display_->DrawAndSwap();
    EXPECT_TRUE(scheduler_->swapped);
    EXPECT_EQ(4u, output_surface_->num_sent_frames());
    EXPECT_TRUE(copy_called);
  }

  // Pass has no damage, so shouldn't be swapped, but latency info should be
  // saved for next swap.
  {
    pass = RenderPass::Create();
    pass->output_rect = gfx::Rect(0, 0, 100, 100);
    pass->damage_rect = gfx::Rect(10, 10, 0, 0);
    pass->id = 1u;

    pass_list.push_back(std::move(pass));
    scheduler_->ResetDamageForTest();

    CompositorFrame frame = test::MakeCompositorFrame();
    pass_list.swap(frame.render_pass_list);
    frame.metadata.latency_info.push_back(ui::LatencyInfo());

    support_->SubmitCompositorFrame(local_surface_id, std::move(frame));
    EXPECT_TRUE(scheduler_->damaged);
    EXPECT_FALSE(scheduler_->display_resized_);
    EXPECT_FALSE(scheduler_->has_new_root_surface);

    scheduler_->swapped = false;
    display_->DrawAndSwap();
    EXPECT_TRUE(scheduler_->swapped);
    EXPECT_EQ(4u, output_surface_->num_sent_frames());
  }

  // Resize should cause a swap if no frame was swapped at the previous size.
  {
    local_surface_id = id_allocator_.GenerateId();
    display_->SetLocalSurfaceId(local_surface_id, 1.f);
    scheduler_->swapped = false;
    display_->Resize(gfx::Size(200, 200));
    EXPECT_FALSE(scheduler_->swapped);
    EXPECT_EQ(4u, output_surface_->num_sent_frames());

    pass = RenderPass::Create();
    pass->output_rect = gfx::Rect(0, 0, 200, 200);
    pass->damage_rect = gfx::Rect(10, 10, 10, 10);
    pass->id = 1u;

    pass_list.push_back(std::move(pass));
    scheduler_->ResetDamageForTest();

    CompositorFrame frame = test::MakeCompositorFrame();
    pass_list.swap(frame.render_pass_list);

    support_->SubmitCompositorFrame(local_surface_id, std::move(frame));
    EXPECT_TRUE(scheduler_->damaged);
    EXPECT_FALSE(scheduler_->display_resized_);
    EXPECT_FALSE(scheduler_->has_new_root_surface);

    scheduler_->swapped = false;
    display_->Resize(gfx::Size(100, 100));
    EXPECT_TRUE(scheduler_->swapped);
    EXPECT_EQ(5u, output_surface_->num_sent_frames());

    // Latency info from previous frame should be sent now.
    EXPECT_EQ(1u, output_surface_->last_sent_frame()->latency_info.size());
  }

  {
    local_surface_id = id_allocator_.GenerateId();
    display_->SetLocalSurfaceId(local_surface_id, 1.0f);
    // Surface that's damaged completely should be resized and swapped.
    pass = RenderPass::Create();
    pass->output_rect = gfx::Rect(0, 0, 99, 99);
    pass->damage_rect = gfx::Rect(0, 0, 99, 99);
    pass->id = 1u;

    pass_list.push_back(std::move(pass));
    scheduler_->ResetDamageForTest();
    SubmitCompositorFrame(&pass_list, local_surface_id);
    EXPECT_TRUE(scheduler_->damaged);
    EXPECT_FALSE(scheduler_->display_resized_);
    EXPECT_FALSE(scheduler_->has_new_root_surface);

    scheduler_->swapped = false;
    display_->DrawAndSwap();
    EXPECT_TRUE(scheduler_->swapped);
    EXPECT_EQ(6u, output_surface_->num_sent_frames());
    EXPECT_EQ(gfx::Size(100, 100),
              software_output_device_->viewport_pixel_size());
    EXPECT_EQ(gfx::Rect(0, 0, 100, 100),
              software_output_device_->damage_rect());
    EXPECT_EQ(0u, output_surface_->last_sent_frame()->latency_info.size());
  }
  TearDownDisplay();
}

// Check LatencyInfo storage is cleaned up if it exceeds the limit.
TEST_F(DisplayTest, MaxLatencyInfoCap) {
  RendererSettings settings;
  settings.partial_swap_enabled = true;
  settings.finish_rendering_on_resize = true;
  SetUpDisplay(settings, nullptr);
  gfx::ColorSpace color_space_1 = gfx::ColorSpace::CreateXYZD50();
  gfx::ColorSpace color_space_2 = gfx::ColorSpace::CreateSCRGBLinear();

  StubDisplayClient client;
  display_->Initialize(&client, manager_.surface_manager());
  display_->SetColorSpace(color_space_1, color_space_1);

  LocalSurfaceId local_surface_id(id_allocator_.GenerateId());
  display_->SetLocalSurfaceId(local_surface_id, 1.f);

  scheduler_->ResetDamageForTest();
  display_->Resize(gfx::Size(100, 100));

  RenderPassList pass_list;
  auto pass = RenderPass::Create();
  pass->output_rect = gfx::Rect(0, 0, 100, 100);
  pass->damage_rect = gfx::Rect(10, 10, 1, 1);
  pass->id = 1u;
  pass_list.push_back(std::move(pass));

  scheduler_->ResetDamageForTest();
  SubmitCompositorFrame(&pass_list, local_surface_id);

  display_->DrawAndSwap();

  // This is the same as LatencyInfo::kMaxLatencyInfoNumber.
  const size_t max_latency_info_count = 100;
  for (size_t i = 0; i <= max_latency_info_count; ++i) {
    pass = RenderPass::Create();
    pass->output_rect = gfx::Rect(0, 0, 100, 100);
    pass->damage_rect = gfx::Rect(10, 10, 0, 0);
    pass->id = 1u;

    pass_list.push_back(std::move(pass));
    scheduler_->ResetDamageForTest();

    CompositorFrame frame = test::MakeCompositorFrame();
    pass_list.swap(frame.render_pass_list);
    frame.metadata.latency_info.push_back(ui::LatencyInfo());

    support_->SubmitCompositorFrame(local_surface_id, std::move(frame));

    display_->DrawAndSwap();

    if (i < max_latency_info_count)
      EXPECT_EQ(i + 1, display_->stored_latency_info_size_for_testing());
    else
      EXPECT_EQ(0u, display_->stored_latency_info_size_for_testing());
  }

  TearDownDisplay();
}

class MockedContext : public cc::TestWebGraphicsContext3D {
 public:
  MOCK_METHOD0(shallowFinishCHROMIUM, void());
};

TEST_F(DisplayTest, Finish) {
  LocalSurfaceId local_surface_id1(id_allocator_.GenerateId());
  LocalSurfaceId local_surface_id2(id_allocator_.GenerateId());

  RendererSettings settings;
  settings.partial_swap_enabled = true;
  settings.finish_rendering_on_resize = true;

  auto context = base::MakeUnique<MockedContext>();
  MockedContext* context_ptr = context.get();
  EXPECT_CALL(*context_ptr, shallowFinishCHROMIUM()).Times(0);

  SetUpDisplay(settings, std::move(context));

  StubDisplayClient client;
  display_->Initialize(&client, manager_.surface_manager());

  display_->SetLocalSurfaceId(local_surface_id1, 1.f);

  display_->Resize(gfx::Size(100, 100));

  {
    RenderPassList pass_list;
    auto pass = RenderPass::Create();
    pass->output_rect = gfx::Rect(0, 0, 100, 100);
    pass->damage_rect = gfx::Rect(10, 10, 1, 1);
    pass->id = 1u;
    pass_list.push_back(std::move(pass));

    SubmitCompositorFrame(&pass_list, local_surface_id1);
  }

  display_->DrawAndSwap();

  // First resize and draw shouldn't finish.
  testing::Mock::VerifyAndClearExpectations(context_ptr);

  EXPECT_CALL(*context_ptr, shallowFinishCHROMIUM());
  display_->Resize(gfx::Size(150, 150));
  testing::Mock::VerifyAndClearExpectations(context_ptr);

  // Another resize without a swap doesn't need to finish.
  EXPECT_CALL(*context_ptr, shallowFinishCHROMIUM()).Times(0);
  display_->SetLocalSurfaceId(local_surface_id2, 1.f);
  display_->Resize(gfx::Size(200, 200));
  testing::Mock::VerifyAndClearExpectations(context_ptr);

  EXPECT_CALL(*context_ptr, shallowFinishCHROMIUM()).Times(0);
  {
    RenderPassList pass_list;
    auto pass = RenderPass::Create();
    pass->output_rect = gfx::Rect(0, 0, 200, 200);
    pass->damage_rect = gfx::Rect(10, 10, 1, 1);
    pass->id = 1u;
    pass_list.push_back(std::move(pass));

    SubmitCompositorFrame(&pass_list, local_surface_id2);
  }

  display_->DrawAndSwap();

  testing::Mock::VerifyAndClearExpectations(context_ptr);

  EXPECT_CALL(*context_ptr, shallowFinishCHROMIUM());
  display_->Resize(gfx::Size(250, 250));
  testing::Mock::VerifyAndClearExpectations(context_ptr);
  TearDownDisplay();
}

class CountLossDisplayClient : public StubDisplayClient {
 public:
  CountLossDisplayClient() = default;

  void DisplayOutputSurfaceLost() override { ++loss_count_; }

  int loss_count() const { return loss_count_; }

 private:
  int loss_count_ = 0;
};

TEST_F(DisplayTest, ContextLossInformsClient) {
  SetUpDisplay(RendererSettings(), cc::TestWebGraphicsContext3D::Create());

  CountLossDisplayClient client;
  display_->Initialize(&client, manager_.surface_manager());

  // Verify DidLoseOutputSurface callback is hooked up correctly.
  EXPECT_EQ(0, client.loss_count());
  output_surface_->context_provider()->ContextGL()->LoseContextCHROMIUM(
      GL_GUILTY_CONTEXT_RESET_ARB, GL_INNOCENT_CONTEXT_RESET_ARB);
  output_surface_->context_provider()->ContextGL()->Flush();
  EXPECT_EQ(1, client.loss_count());
  TearDownDisplay();
}

// Regression test for https://crbug.com/727162: Submitting a CompositorFrame to
// a surface should only cause damage on the Display the surface belongs to.
// There should not be a side-effect on other Displays.
TEST_F(DisplayTest, CompositorFrameDamagesCorrectDisplay) {
  RendererSettings settings;
  LocalSurfaceId local_surface_id(id_allocator_.GenerateId());

  // Set up first display.
  SetUpDisplay(settings, nullptr);
  StubDisplayClient client;
  display_->Initialize(&client, manager_.surface_manager());
  display_->SetLocalSurfaceId(local_surface_id, 1.f);

  // Set up second frame sink + display.
  auto support2 = CompositorFrameSinkSupport::Create(
      nullptr, &manager_, kAnotherFrameSinkId, true /* is_root */,
      true /* needs_sync_points */);
  auto begin_frame_source2 = base::MakeUnique<StubBeginFrameSource>();
  auto scheduler_for_display2 = base::MakeUnique<TestDisplayScheduler>(
      begin_frame_source2.get(), task_runner_.get());
  TestDisplayScheduler* scheduler2 = scheduler_for_display2.get();
  auto display2 = CreateDisplay(
      settings, kAnotherFrameSinkId, std::move(scheduler_for_display2),
      cc::FakeOutputSurface::CreateSoftware(
          base::MakeUnique<TestSoftwareOutputDevice>()));
  manager_.RegisterBeginFrameSource(begin_frame_source2.get(),
                                    kAnotherFrameSinkId);
  StubDisplayClient client2;
  display2->Initialize(&client2, manager_.surface_manager());
  display2->SetLocalSurfaceId(local_surface_id, 1.f);

  display_->Resize(gfx::Size(100, 100));
  display2->Resize(gfx::Size(100, 100));

  scheduler_->ResetDamageForTest();
  scheduler2->ResetDamageForTest();
  EXPECT_FALSE(scheduler_->damaged);
  EXPECT_FALSE(scheduler2->damaged);

  // Submit a frame for display_ with full damage.
  RenderPassList pass_list;
  auto pass = RenderPass::Create();
  pass->output_rect = gfx::Rect(0, 0, 100, 100);
  pass->damage_rect = gfx::Rect(10, 10, 1, 1);
  pass->id = 1;
  pass_list.push_back(std::move(pass));

  SubmitCompositorFrame(&pass_list, local_surface_id);

  // Should have damaged only display_ but not display2.
  EXPECT_TRUE(scheduler_->damaged);
  EXPECT_FALSE(scheduler2->damaged);
  manager_.UnregisterBeginFrameSource(begin_frame_source2.get());
  TearDownDisplay();
}

// Check if draw occlusion does not remove any draw quads when no quads is being
// covered completely.
TEST_F(DisplayTest, DrawOcclusionWithNonCoveringDQ) {
  SetUpDisplay(RendererSettings(), cc::TestWebGraphicsContext3D::Create());

  CountLossDisplayClient client;
  display_->Initialize(&client, manager_.surface_manager());

  CompositorFrame frame = test::MakeCompositorFrame();
  gfx::Rect rect1(0, 0, 100, 100);
  gfx::Rect rect2(50, 50, 100, 100);
  gfx::Rect rect3(25, 25, 50, 100);
  gfx::Rect rect4(150, 0, 50, 50);
  gfx::Rect rect5(0, 0, 120, 120);

  bool is_clipped = false;
  bool are_contents_opaque = true;
  float opacity = 1.f;
  SharedQuadState* shared_quad_state =
      frame.render_pass_list.front()->CreateAndAppendSharedQuadState();
  auto* quad = frame.render_pass_list.front()
                   ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();

  // +----+
  // |    |
  // +----+
  {
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
  }
  SharedQuadState* shared_quad_state2 =
      frame.render_pass_list.front()->CreateAndAppendSharedQuadState();
  auto* quad2 = frame.render_pass_list.front()
                    ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();

  // +----+
  // | +--|-+
  // +----+ |
  //   +----+
  {
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);

    shared_quad_state2->SetAll(gfx::Transform(), rect2, rect2, rect2,
                               is_clipped, are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect2, rect2, SK_ColorBLACK, false);

    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
  }

  // +----+
  // |+--+|
  // +----+
  //  +--+
  {
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);

    shared_quad_state2->SetAll(gfx::Transform(), rect3, rect3, rect3,
                               is_clipped, are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect3, rect3, SK_ColorBLACK, false);

    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
  }

  // +----+   +--+
  // |    |   +--+
  // +----+
  {
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);

    shared_quad_state2->SetAll(gfx::Transform(), rect4, rect4, rect4,
                               is_clipped, are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect4, rect4, SK_ColorBLACK, false);

    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
  }

  // +-----++
  // |     ||
  // +-----+|
  // +------+
  {
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);

    shared_quad_state2->SetAll(gfx::Transform(), rect5, rect5, rect5,
                               is_clipped, are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect5, rect5, SK_ColorBLACK, false);

    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
  }

  TearDownDisplay();
}

// Check if draw occlusion removes draw quads when quads are being covered
// completely.
TEST_F(DisplayTest, CompositorFrameWithOverlapDQ) {
  SetUpDisplay(RendererSettings(), cc::TestWebGraphicsContext3D::Create());

  CountLossDisplayClient client;
  display_->Initialize(&client, manager_.surface_manager());

  CompositorFrame frame = test::MakeCompositorFrame();
  gfx::Rect rect1(0, 0, 100, 100);
  gfx::Rect rect2(25, 25, 50, 50);
  gfx::Rect rect3(50, 50, 50, 25);
  gfx::Rect rect4(0, 0, 50, 50);

  bool is_clipped = false;
  bool are_contents_opaque = true;
  float opacity = 1.f;
  SharedQuadState* shared_quad_state =
      frame.render_pass_list.front()->CreateAndAppendSharedQuadState();
  auto* quad = frame.render_pass_list.front()
                   ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
  SharedQuadState* shared_quad_state2 =
      frame.render_pass_list.front()->CreateAndAppendSharedQuadState();
  auto* quad2 = frame.render_pass_list.front()
                    ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();

  // completely overlapping: +-----+
  //                         |     |
  //                         +-----+
  {
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);
    shared_quad_state2->SetAll(gfx::Transform(), rect1, rect1, rect1,
                               is_clipped, are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect1, rect1, SK_ColorBLACK, false);
    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
  }

  //  +-----+
  //  | +-+ |
  //  | +-+ |
  //  +-----+
  {
    quad2 = frame.render_pass_list.front()
                ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);
    shared_quad_state2->SetAll(gfx::Transform(), rect2, rect2, rect2,
                               is_clipped, are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect2, rect2, SK_ColorBLACK, false);
    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
  }

  // +-----+
  // |  +--|
  // |  +--|
  // +-----+
  {
    quad2 = frame.render_pass_list.front()
                ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);
    shared_quad_state2->SetAll(gfx::Transform(), rect3, rect3, rect3,
                               is_clipped, are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect3, rect3, SK_ColorBLACK, false);
    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
  }

  // +-----++
  // |     ||
  // +-----+|
  // +------+
  {
    quad2 = frame.render_pass_list.front()
                ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);

    shared_quad_state2->SetAll(gfx::Transform(), rect4, rect4, rect4,
                               is_clipped, are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect4, rect4, SK_ColorBLACK, false);

    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
  }
  TearDownDisplay();
}

// Check if draw occlusion works well with scale change transformer.
TEST_F(DisplayTest, CompositorFrameWithTransformer) {
  SetUpDisplay(RendererSettings(), cc::TestWebGraphicsContext3D::Create());

  CountLossDisplayClient client;
  display_->Initialize(&client, manager_.surface_manager());

  // Rect 2, 3, 4 are contained in rect 1 only after applying the scale matrix.
  CompositorFrame frame = test::MakeCompositorFrame();
  gfx::Rect rect1(0, 0, 100, 100);
  gfx::Rect rect2(25, 25, 100, 100);
  gfx::Rect rect3(50, 50, 100, 50);
  gfx::Rect rect4(0, 0, 120, 120);

  // Rect 5, 6, 7 are not contained by rect 1 after applying the scale matrix.
  gfx::Rect rect5(25, 25, 60, 60);
  gfx::Rect rect6(0, 50, 25, 70);
  gfx::Rect rect7(0, 0, 60, 60);

  gfx::Transform half_scale;
  half_scale.Scale(0.5, 0.5);
  gfx::Transform double_scale;
  half_scale.Scale(2, 2);
  bool is_clipped = false;
  bool are_contents_opaque = true;
  float opacity = 1.f;
  SharedQuadState* shared_quad_state =
      frame.render_pass_list.front()->CreateAndAppendSharedQuadState();
  auto* quad = frame.render_pass_list.front()
                   ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
  SharedQuadState* shared_quad_state2 =
      frame.render_pass_list.front()->CreateAndAppendSharedQuadState();
  auto* quad2 = frame.render_pass_list.front()
                    ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();

  {
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);
    shared_quad_state2->SetAll(half_scale, rect2, rect2, rect2, is_clipped,
                               are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect2, rect2, SK_ColorBLACK, false);
    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
  }

  {
    quad2 = frame.render_pass_list.front()
                ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);
    shared_quad_state2->SetAll(half_scale, rect4, rect4, rect4, is_clipped,
                               are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect4, rect4, SK_ColorBLACK, false);
    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
  }

  {
    quad2 = frame.render_pass_list.front()
                ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);

    shared_quad_state2->SetAll(half_scale, rect4, rect4, rect4, is_clipped,
                               are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect4, rect4, SK_ColorBLACK, false);

    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
  }

  {
    quad2 = frame.render_pass_list.front()
                ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);

    shared_quad_state2->SetAll(double_scale, rect5, rect5, rect5, is_clipped,
                               are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect5, rect5, SK_ColorBLACK, false);

    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
  }

  {
    quad2 = frame.render_pass_list.front()
                ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);

    shared_quad_state2->SetAll(half_scale, rect6, rect6, rect6, is_clipped,
                               are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect6, rect6, SK_ColorBLACK, false);

    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
  }

  {
    quad2 = frame.render_pass_list.front()
                ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
    shared_quad_state->SetAll(gfx::Transform(), rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);

    shared_quad_state2->SetAll(half_scale, rect7, rect7, rect7, is_clipped,
                               are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);

    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect7, rect7, SK_ColorBLACK, false);

    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
  }
  TearDownDisplay();
}

// Check if draw occlusion works well with rotation transformer.
//                                   _____
//  +-----+                         /    /
//  |     |   rotation (by 45) ->  /____/     rect (a smaller rect)->    +---+
//  +-----+                                                              +---+
TEST_F(DisplayTest, CompositorFrameWithRotation) {
  SetUpDisplay(RendererSettings(), cc::TestWebGraphicsContext3D::Create());

  CountLossDisplayClient client;
  display_->Initialize(&client, manager_.surface_manager());

  // rect 2 is inside rect 1 initially.
  CompositorFrame frame = test::MakeCompositorFrame();
  gfx::Rect rect1(0, 0, 100, 100);
  gfx::Rect rect2(75, 75, 10, 10);

  gfx::Transform rotate;
  rotate.RotateAboutYAxis(45);
  bool is_clipped = false;
  bool are_contents_opaque = true;
  float opacity = 1.f;
  SharedQuadState* shared_quad_state =
      frame.render_pass_list.front()->CreateAndAppendSharedQuadState();
  auto* quad = frame.render_pass_list.front()
                   ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
  SharedQuadState* shared_quad_state2 =
      frame.render_pass_list.front()->CreateAndAppendSharedQuadState();
  auto* quad2 = frame.render_pass_list.front()
                    ->quad_list.AllocateAndConstruct<SolidColorDrawQuad>();
  {
    // rect 1 becomes (0, 0, 71x100) after rotation, so rect 2 is outside of
    // rect 1 after rotation.
    shared_quad_state->SetAll(rotate, rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);
    shared_quad_state2->SetAll(gfx::Transform(), rect2, rect2, rect2,
                               is_clipped, are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);
    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect2, rect2, SK_ColorBLACK, false);
    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
  }

  {
    // Rotational transformer is applied to both rect 1 and rect 2. So rect 2 is
    // covered by rect 1 after rotation in this case.
    shared_quad_state->SetAll(rotate, rect1, rect1, rect1, is_clipped,
                              are_contents_opaque, opacity,
                              SkBlendMode::kSrcOver, 0);
    shared_quad_state2->SetAll(rotate, rect2, rect2, rect2, is_clipped,
                               are_contents_opaque, opacity,
                               SkBlendMode::kSrcOver, 0);
    quad->SetNew(shared_quad_state, rect1, rect1, SK_ColorBLACK, false);
    quad2->SetNew(shared_quad_state2, rect2, rect2, SK_ColorBLACK, false);
    EXPECT_EQ(2u, frame.render_pass_list.front()->quad_list.size());
    display_->RemoveDrawQuad(&frame);
    EXPECT_EQ(1u, frame.render_pass_list.front()->quad_list.size());
  }
  TearDownDisplay();
}

}  // namespace
}  // namespace viz
