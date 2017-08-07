// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/highlighter/highlighter_controller.h"

#include "ash/ash_switches.h"
#include "ash/fast_ink/fast_ink_points.h"
#include "ash/highlighter/highlighter_controller_test_api.h"
#include "ash/palette_delegate.h"
#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/shell_test_api.h"
#include "ash/system/palette/palette_ids.h"
#include "ash/system/palette/palette_tray.h"
#include "ash/system/palette/palette_utils.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/status_area_widget_test_helper.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ash/test_shell_delegate.h"
#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "components/prefs/testing_pref_service.h"
#include "ui/events/test/event_generator.h"

namespace ash {
namespace {

class TestPaletteDelegate : public PaletteDelegate {
 public:
  TestPaletteDelegate() {}
  ~TestPaletteDelegate() override {}

  void set_is_metalayer_supported(bool is_metalayer_supported) {
    is_metalayer_supported_ = is_metalayer_supported;
    if (!is_metalayer_supported_ && !metalayer_closed_callback_.is_null())
      base::ResetAndReturn(&metalayer_closed_callback_).Run();
  }

  void set_controller_test_api(
      HighlighterControllerTestApi* controller_test_api) {
    controller_test_api_ = controller_test_api;
  }

 private:
  // PaletteDelegate:
  std::unique_ptr<EnableListenerSubscription> AddPaletteEnableListener(
      const EnableListener& on_state_changed) override {
    return nullptr;
  }
  void CreateNote() override {}
  bool HasNoteApp() override { return false; }
  bool ShouldAutoOpenPalette() override { return false; }
  bool ShouldShowPalette() override { return false; }
  void TakeScreenshot() override {}
  void TakePartialScreenshot(const base::Closure& done) override {}
  void CancelPartialScreenshot() override {}
  bool IsMetalayerSupported() override { return is_metalayer_supported_; }
  void ShowMetalayer(const base::Closure& callback) override {
    controller_test_api_->SetEnabled(true);
    metalayer_closed_callback_ = callback;
  }
  void HideMetalayer() override {
    controller_test_api_->SetEnabled(false);
    metalayer_closed_callback_ = base::Closure();
  }

  bool is_metalayer_supported_ = false;
  base::Closure metalayer_closed_callback_;
  HighlighterControllerTestApi* controller_test_api_;

  DISALLOW_COPY_AND_ASSIGN(TestPaletteDelegate);
};

class HighlighterControllerTest : public AshTestBase {
 public:
  HighlighterControllerTest() {}
  ~HighlighterControllerTest() override {}

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kAshForceEnableStylusTools);
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kAshEnablePaletteOnAllDisplays);

    AshTestBase::SetUp();

    // The below pref_service_ manipulation is required to make the palette tray
    // visible (see ash::PaletteTrayTest).
    Shell::RegisterLocalStatePrefs(pref_service_.registry());
    ash_test_helper()->test_shell_delegate()->set_local_state_pref_service(
        &pref_service_);

    palette_tray_ =
        StatusAreaWidgetTestHelper::GetStatusAreaWidget()->palette_tray();
    palette_test_api_ = base::MakeUnique<PaletteTray::TestApi>(palette_tray_);

    // Set the test palette delegate here, since this requires an instance of
    // shell to be available.
    ShellTestApi().SetPaletteDelegate(base::MakeUnique<TestPaletteDelegate>());
    // Initialize the palette tray again since this test requires information
    // from the palette delegate. (It was initialized without the delegate in
    // AshTestBase::SetUp()).
    palette_tray_->Initialize();

    controller_ = base::MakeUnique<HighlighterController>();
  }

  void TearDown() override {
    // This needs to be called first to remove the event handler before the
    // shell instance gets torn down.
    controller_.reset();
    AshTestBase::TearDown();
  }

 protected:
  void TraceRect(const gfx::Rect& rect) {
    GetEventGenerator().MoveTouch(gfx::Point(rect.x(), rect.y()));
    GetEventGenerator().PressTouch();
    GetEventGenerator().MoveTouch(gfx::Point(rect.right(), rect.y()));
    GetEventGenerator().MoveTouch(gfx::Point(rect.right(), rect.bottom()));
    GetEventGenerator().MoveTouch(gfx::Point(rect.x(), rect.bottom()));
    GetEventGenerator().MoveTouch(gfx::Point(rect.x(), rect.y()));
    GetEventGenerator().ReleaseTouch();
  }

  TestPaletteDelegate* test_palette_delegate() {
    return static_cast<TestPaletteDelegate*>(Shell::Get()->palette_delegate());
  }

  bool IsToolActive() {
    return palette_test_api_->GetPaletteToolManager()->IsToolActive(
        PaletteToolId::METALAYER);
  }

  void ActivateTool() {
    palette_test_api_->GetPaletteToolManager()->ActivateTool(
        PaletteToolId::METALAYER);
  }

  void DeactivateTool() {
    palette_test_api_->GetPaletteToolManager()->DeactivateTool(
        PaletteToolId::METALAYER);
  }

  gfx::Rect GetPaletteBoundsInScreen() {
    return palette_tray_->GetBoundsInScreen();
  }

  std::unique_ptr<HighlighterController> controller_;

 private:
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<PaletteTray::TestApi> palette_test_api_;
  PaletteTray* palette_tray_ = nullptr;  // not owned

  DISALLOW_COPY_AND_ASSIGN(HighlighterControllerTest);
};

}  // namespace

// Test to ensure the class responsible for drawing the highlighter pointer
// receives points from stylus movements as expected.
TEST_F(HighlighterControllerTest, HighlighterRenderer) {
  HighlighterControllerTestApi controller_test_api_(controller_.get());

  // The highlighter pointer mode only works with stylus.
  GetEventGenerator().EnterPenPointerMode();

  // When disabled the highlighter pointer should not be showing.
  GetEventGenerator().MoveTouch(gfx::Point(1, 1));
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());

  // Verify that by enabling the mode, the highlighter pointer should still not
  // be showing.
  controller_test_api_.SetEnabled(true);
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());

  // Verify moving the stylus 4 times will not display the highlighter pointer.
  GetEventGenerator().MoveTouch(gfx::Point(2, 2));
  GetEventGenerator().MoveTouch(gfx::Point(3, 3));
  GetEventGenerator().MoveTouch(gfx::Point(4, 4));
  GetEventGenerator().MoveTouch(gfx::Point(5, 5));
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());

  // Verify pressing the stylus will show the highlighter pointer and add a
  // point but will not activate fading out.
  GetEventGenerator().PressTouch();
  EXPECT_TRUE(controller_test_api_.IsShowingHighlighter());
  EXPECT_FALSE(controller_test_api_.IsFadingAway());
  EXPECT_EQ(1, controller_test_api_.points().GetNumberOfPoints());

  // Verify dragging the stylus 2 times will add 2 more points.
  GetEventGenerator().MoveTouch(gfx::Point(6, 6));
  GetEventGenerator().MoveTouch(gfx::Point(7, 7));
  EXPECT_EQ(3, controller_test_api_.points().GetNumberOfPoints());

  // Verify releasing the stylus still shows the highlighter pointer, which is
  // fading away.
  GetEventGenerator().ReleaseTouch();
  EXPECT_TRUE(controller_test_api_.IsShowingHighlighter());
  EXPECT_TRUE(controller_test_api_.IsFadingAway());

  // Verify that disabling the mode does not display the highlighter pointer.
  controller_test_api_.SetEnabled(false);
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());
  EXPECT_FALSE(controller_test_api_.IsFadingAway());

  // Verify that disabling the mode while highlighter pointer is displayed does
  // not display the highlighter pointer.
  controller_test_api_.SetEnabled(true);
  GetEventGenerator().PressTouch();
  GetEventGenerator().MoveTouch(gfx::Point(6, 6));
  EXPECT_TRUE(controller_test_api_.IsShowingHighlighter());
  controller_test_api_.SetEnabled(false);
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());

  // Verify that the highlighter pointer does not add points while disabled.
  GetEventGenerator().PressTouch();
  GetEventGenerator().MoveTouch(gfx::Point(8, 8));
  GetEventGenerator().ReleaseTouch();
  GetEventGenerator().MoveTouch(gfx::Point(9, 9));
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());

  // Verify that the highlighter pointer does not get shown if points are not
  // coming from the stylus, even when enabled.
  GetEventGenerator().ExitPenPointerMode();
  controller_test_api_.SetEnabled(true);
  GetEventGenerator().PressTouch();
  GetEventGenerator().MoveTouch(gfx::Point(10, 10));
  GetEventGenerator().MoveTouch(gfx::Point(11, 11));
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());
  GetEventGenerator().ReleaseTouch();
}

// Test to ensure the class responsible for drawing the highlighter pointer
// handles prediction as expected when it receives points from stylus movements.
TEST_F(HighlighterControllerTest, HighlighterPrediction) {
  HighlighterControllerTestApi controller_test_api_(controller_.get());

  controller_test_api_.SetEnabled(true);
  // The highlighter pointer mode only works with stylus.
  GetEventGenerator().EnterPenPointerMode();
  GetEventGenerator().PressTouch();
  EXPECT_TRUE(controller_test_api_.IsShowingHighlighter());

  EXPECT_EQ(1, controller_test_api_.points().GetNumberOfPoints());
  // Initial press event shouldn't generate any predicted points as there's no
  // history to use for prediction.
  EXPECT_EQ(0, controller_test_api_.predicted_points().GetNumberOfPoints());

  // Verify dragging the stylus 3 times will add some predicted points.
  GetEventGenerator().MoveTouch(gfx::Point(10, 10));
  GetEventGenerator().MoveTouch(gfx::Point(20, 20));
  GetEventGenerator().MoveTouch(gfx::Point(30, 30));
  EXPECT_NE(0, controller_test_api_.predicted_points().GetNumberOfPoints());
  // Verify predicted points are in the right direction.
  for (const auto& point : controller_test_api_.predicted_points().points()) {
    EXPECT_LT(30, point.location.x());
    EXPECT_LT(30, point.location.y());
  }
}

// Test that stylus gestures are correctly recognized by HighlighterController.
TEST_F(HighlighterControllerTest, HighlighterGestures) {
  HighlighterControllerTestApi controller_test_api_(controller_.get());

  controller_test_api_.SetEnabled(true);
  GetEventGenerator().EnterPenPointerMode();

  // A non-horizontal stroke is not recognized
  controller_test_api_.ResetSelection();
  GetEventGenerator().MoveTouch(gfx::Point(100, 100));
  GetEventGenerator().PressTouch();
  GetEventGenerator().MoveTouch(gfx::Point(200, 200));
  GetEventGenerator().ReleaseTouch();
  EXPECT_FALSE(controller_test_api_.handle_selection_called());

  // An almost horizontal stroke is recognized
  controller_test_api_.ResetSelection();
  GetEventGenerator().MoveTouch(gfx::Point(100, 100));
  GetEventGenerator().PressTouch();
  GetEventGenerator().MoveTouch(gfx::Point(300, 102));
  GetEventGenerator().ReleaseTouch();
  EXPECT_TRUE(controller_test_api_.handle_selection_called());

  // Horizontal stroke selection rectangle should:
  //   have the same horizontal center line as the stroke bounding box,
  //   be 4dp wider than the stroke bounding box,
  //   be exactly 14dp high.
  EXPECT_EQ("98,94 204x14", controller_test_api_.selection().ToString());

  // An insufficiently closed C-like shape is not recognized
  controller_test_api_.ResetSelection();
  GetEventGenerator().MoveTouch(gfx::Point(100, 0));
  GetEventGenerator().PressTouch();
  GetEventGenerator().MoveTouch(gfx::Point(0, 0));
  GetEventGenerator().MoveTouch(gfx::Point(0, 100));
  GetEventGenerator().MoveTouch(gfx::Point(100, 100));
  GetEventGenerator().ReleaseTouch();
  EXPECT_FALSE(controller_test_api_.handle_selection_called());

  // An almost closed G-like shape is recognized
  controller_test_api_.ResetSelection();
  GetEventGenerator().MoveTouch(gfx::Point(200, 0));
  GetEventGenerator().PressTouch();
  GetEventGenerator().MoveTouch(gfx::Point(0, 0));
  GetEventGenerator().MoveTouch(gfx::Point(0, 100));
  GetEventGenerator().MoveTouch(gfx::Point(200, 100));
  GetEventGenerator().MoveTouch(gfx::Point(200, 20));
  GetEventGenerator().ReleaseTouch();
  EXPECT_TRUE(controller_test_api_.handle_selection_called());
  EXPECT_EQ("0,0 200x100", controller_test_api_.selection().ToString());

  // A closed diamond shape is recognized
  controller_test_api_.ResetSelection();
  GetEventGenerator().MoveTouch(gfx::Point(100, 0));
  GetEventGenerator().PressTouch();
  GetEventGenerator().MoveTouch(gfx::Point(200, 150));
  GetEventGenerator().MoveTouch(gfx::Point(100, 300));
  GetEventGenerator().MoveTouch(gfx::Point(0, 150));
  GetEventGenerator().MoveTouch(gfx::Point(100, 0));
  GetEventGenerator().ReleaseTouch();
  EXPECT_TRUE(controller_test_api_.handle_selection_called());
  EXPECT_EQ("0,0 200x300", controller_test_api_.selection().ToString());
}

// Test that stylus gesture recognition correctly handles display scaling
TEST_F(HighlighterControllerTest, HighlighterGesturesScaled) {
  HighlighterControllerTestApi controller_test_api_(controller_.get());

  controller_test_api_.SetEnabled(true);
  GetEventGenerator().EnterPenPointerMode();

  const gfx::Rect original_rect(200, 100, 400, 300);

  // Allow for rounding errors.
  gfx::Rect inflated(original_rect);
  inflated.Inset(-1, -1);

  constexpr float display_scales[] = {1.f, 1.5f, 2.0f};
  constexpr float ui_scales[] = {0.5f,  0.67f, 1.0f,  1.25f,
                                 1.33f, 1.5f,  1.67f, 2.0f};

  for (size_t i = 0; i < sizeof(display_scales) / sizeof(float); ++i) {
    const float display_scale = display_scales[i];
    for (size_t j = 0; j < sizeof(ui_scales) / sizeof(float); ++j) {
      const float ui_scale = ui_scales[j];

      std::string display_spec =
          base::StringPrintf("1500x1000*%.2f@%.2f", display_scale, ui_scale);
      SCOPED_TRACE(display_spec);
      UpdateDisplay(display_spec);

      controller_test_api_.ResetSelection();
      TraceRect(original_rect);
      EXPECT_TRUE(controller_test_api_.handle_selection_called());

      const gfx::Rect selection = controller_test_api_.selection();
      EXPECT_TRUE(inflated.Contains(selection));
      EXPECT_TRUE(selection.Contains(original_rect));
    }
  }
}

// Test that stylus gesture recognition correctly handles display rotation
TEST_F(HighlighterControllerTest, HighlighterGesturesRotated) {
  HighlighterControllerTestApi controller_test_api_(controller_.get());

  controller_test_api_.SetEnabled(true);
  GetEventGenerator().EnterPenPointerMode();

  const gfx::Rect trace(200, 100, 400, 300);

  // No rotation
  UpdateDisplay("1500x1000");
  controller_test_api_.ResetSelection();
  TraceRect(trace);
  EXPECT_TRUE(controller_test_api_.handle_selection_called());
  EXPECT_EQ("200,100 400x300", controller_test_api_.selection().ToString());

  // Rotate to 90 degrees
  UpdateDisplay("1500x1000/r");
  controller_test_api_.ResetSelection();
  TraceRect(trace);
  EXPECT_TRUE(controller_test_api_.handle_selection_called());
  EXPECT_EQ("100,899 300x400", controller_test_api_.selection().ToString());

  // Rotate to 180 degrees
  UpdateDisplay("1500x1000/u");
  controller_test_api_.ResetSelection();
  TraceRect(trace);
  EXPECT_TRUE(controller_test_api_.handle_selection_called());
  EXPECT_EQ("899,599 400x300", controller_test_api_.selection().ToString());

  // Rotate to 270 degrees
  UpdateDisplay("1500x1000/l");
  controller_test_api_.ResetSelection();
  TraceRect(trace);
  EXPECT_TRUE(controller_test_api_.handle_selection_called());
  EXPECT_EQ("599,200 300x400", controller_test_api_.selection().ToString());
}

// Test the interaction between the highlighter and the metalayer palette tool.
// The two are unaware of each other and need to be connected by some kind
// of "glue" (provided in this case by TestPaletteDelegate).
TEST_F(HighlighterControllerTest, HighlighterInvokedViaPalette) {
  // TODO(crbug.com/751191): Remove the check for Mash.
  // See palette_tray_unittest.cc for similar checks.
  if (Shell::GetAshConfig() == Config::MASH)
    return;

  HighlighterControllerTestApi controller_test_api_(controller_.get());
  test_palette_delegate()->set_controller_test_api(&controller_test_api_);
  GetEventGenerator().EnterPenPointerMode();

  test_palette_delegate()->set_is_metalayer_supported(true);

  // Press/drag does not activate the highlighter unless the palette tool is
  // activated.
  GetEventGenerator().MoveTouch(gfx::Point(1, 1));
  GetEventGenerator().PressTouch();
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());
  GetEventGenerator().MoveTouch(gfx::Point(2, 2));
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());
  GetEventGenerator().ReleaseTouch();

  // Activate the palette tool, still no highlighter.
  ActivateTool();
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());

  // Press over a regular (non-palette) location. This should activate the
  // highlighter.
  EXPECT_FALSE(palette_utils::PaletteContainsPointInScreen(gfx::Point(1, 1)));
  GetEventGenerator().MoveTouch(gfx::Point(1, 1));
  GetEventGenerator().PressTouch();
  EXPECT_TRUE(controller_test_api_.IsShowingHighlighter());
  GetEventGenerator().ReleaseTouch();

  // Disable/enable the palette tool to hide the highlighter.
  DeactivateTool();
  ActivateTool();
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());

  // Press/drag over the palette button. This should not activate the
  // highlighter.
  gfx::Point palette_point = GetPaletteBoundsInScreen().CenterPoint();
  EXPECT_TRUE(palette_utils::PaletteContainsPointInScreen(palette_point));
  GetEventGenerator().MoveTouch(palette_point);
  GetEventGenerator().PressTouch();
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());
  palette_point += gfx::Vector2d(1, 1);
  EXPECT_TRUE(palette_utils::PaletteContainsPointInScreen(palette_point));
  GetEventGenerator().MoveTouch(palette_point);
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());
  GetEventGenerator().ReleaseTouch();

  // The previous gesture should have disabled the palette tool.
  EXPECT_FALSE(IsToolActive());
  ActivateTool();

  // Disabling metalayer support in the delegate should disable the palette
  // tool.
  test_palette_delegate()->set_is_metalayer_supported(false);
  EXPECT_FALSE(IsToolActive());

  // With the metalayer disabled again, press/drag does not activate the
  // highlighter.
  GetEventGenerator().MoveTouch(gfx::Point(1, 1));
  GetEventGenerator().PressTouch();
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());
  GetEventGenerator().MoveTouch(gfx::Point(2, 2));
  EXPECT_FALSE(controller_test_api_.IsShowingHighlighter());
  GetEventGenerator().ReleaseTouch();
}

}  // namespace ash
