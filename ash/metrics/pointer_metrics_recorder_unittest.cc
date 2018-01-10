// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/metrics/pointer_metrics_recorder.h"

#include "ash/display/screen_orientation_controller_test_api.h"
#include "ash/public/cpp/app_types.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/test/histogram_tester.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/display/test/display_manager_test_api.h"
#include "ui/events/event.h"
#include "ui/views/pointer_watcher.h"
#include "ui/views/widget/widget.h"

using views::PointerWatcher;

namespace ash {
namespace {

const char kCombinationHistogramName[] =
    "Event.DownEventCount.PerInputFormFactorDestinationCombination";

// Test fixture for the PointerMetricsRecorder class.
class PointerMetricsRecorderTest : public AshTestBase {
 public:
  PointerMetricsRecorderTest();
  ~PointerMetricsRecorderTest() override;

  // AshTestBase:
  void SetUp() override;
  void TearDown() override;

  void CreateDownEventWithGivenCombination(ui::EventPointerType pointer_type,
                                           DownEventFormFactor form_factor,
                                           AppType destination);

 protected:
  // The test target.
  std::unique_ptr<PointerMetricsRecorder> pointer_metrics_recorder_;

  // Used to verify recorded data.
  std::unique_ptr<base::HistogramTester> histogram_tester_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PointerMetricsRecorderTest);
};

PointerMetricsRecorderTest::PointerMetricsRecorderTest() = default;

PointerMetricsRecorderTest::~PointerMetricsRecorderTest() = default;

void PointerMetricsRecorderTest::SetUp() {
  AshTestBase::SetUp();
  pointer_metrics_recorder_.reset(new PointerMetricsRecorder());
  histogram_tester_.reset(new base::HistogramTester());
}

void PointerMetricsRecorderTest::TearDown() {
  pointer_metrics_recorder_.reset();
  AshTestBase::TearDown();
}

void PointerMetricsRecorderTest::CreateDownEventWithGivenCombination(
    ui::EventPointerType pointer_type,
    DownEventFormFactor form_factor,
    AppType destination) {
  const ui::PointerEvent pointer_event(
      ui::ET_POINTER_DOWN, gfx::Point(), gfx::Point(), 0, 0,
      ui::PointerDetails(pointer_type, 0), base::TimeTicks());
  std::unique_ptr<views::Widget> target =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());

  aura::Window* window = target->GetNativeWindow();
  CHECK(window);
  window->SetProperty(aura::client::kAppType, static_cast<int>(destination));

  if (form_factor == DownEventFormFactor::kClamshell) {
    Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(
        false);
  } else {
    Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);

    int64_t display_id = display::Screen::GetScreen()->GetPrimaryDisplay().id();
    display::DisplayManager* display_manager = Shell::Get()->display_manager();
    display::test::ScopedSetInternalDisplayId set_internal(display_manager,
                                                           display_id);
    ScreenOrientationControllerTestApi test_api(
        Shell::Get()->screen_orientation_controller());
    display::Display::Rotation rotation =
        (form_factor == DownEventFormFactor::kTabletModeLandscape)
            ? display::Display::ROTATE_0
            : display::Display::ROTATE_90;

    // Set the screen orientation.
    test_api.SetDisplayRotation(rotation,
                                display::Display::ROTATION_SOURCE_ACTIVE);
  }
  pointer_metrics_recorder_->OnPointerEventObserved(pointer_event, gfx::Point(),
                                                    window);
}

}  // namespace

// Verifies that histogram is not recorded when receiving events that are not
// down events.
TEST_F(PointerMetricsRecorderTest, NonDownEventsInAllPointerHistogram) {
  std::unique_ptr<views::Widget> target =
      CreateTestWidget(nullptr, kShellWindowId_DefaultContainer, gfx::Rect());
  const ui::PointerEvent pointer_event(
      ui::ET_POINTER_UP, gfx::Point(), gfx::Point(), 0, 0,
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_MOUSE, 0),
      base::TimeTicks());
  pointer_metrics_recorder_->OnPointerEventObserved(pointer_event, gfx::Point(),
                                                    target->GetNativeView());

  histogram_tester_->ExpectTotalCount(kCombinationHistogramName, 0);
}

// Verifies that down events from different combination of input type, form
// factor and destination are recorded.
TEST_F(PointerMetricsRecorderTest, DownEventPerCombination) {
  CreateDownEventWithGivenCombination(
      ui::EventPointerType::POINTER_TYPE_UNKNOWN,
      DownEventFormFactor::kClamshell, AppType::OTHERS);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kUnknownClamshellOthers),
      1);

  CreateDownEventWithGivenCombination(
      ui::EventPointerType::POINTER_TYPE_UNKNOWN,
      DownEventFormFactor::kClamshell, AppType::BROWSER);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kUnknownClamshellBrowser),
      1);

  CreateDownEventWithGivenCombination(
      ui::EventPointerType::POINTER_TYPE_UNKNOWN,
      DownEventFormFactor::kClamshell, AppType::CHROME_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kUnknownClamshellChromeApp),
      1);

  CreateDownEventWithGivenCombination(
      ui::EventPointerType::POINTER_TYPE_UNKNOWN,
      DownEventFormFactor::kClamshell, AppType::ARC_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kUnknownClamshellArcApp),
      1);

  CreateDownEventWithGivenCombination(
      ui::EventPointerType::POINTER_TYPE_UNKNOWN,
      DownEventFormFactor::kTabletModeLandscape, AppType::OTHERS);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kUnknownTabletLandscapeOthers),
      1);

  CreateDownEventWithGivenCombination(
      ui::EventPointerType::POINTER_TYPE_UNKNOWN,
      DownEventFormFactor::kTabletModeLandscape, AppType::BROWSER);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kUnknownTabletLandscapeBrowser),
      1);

  CreateDownEventWithGivenCombination(
      ui::EventPointerType::POINTER_TYPE_UNKNOWN,
      DownEventFormFactor::kTabletModeLandscape, AppType::CHROME_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kUnknownTabletLandscapeChromeApp),
      1);

  CreateDownEventWithGivenCombination(
      ui::EventPointerType::POINTER_TYPE_UNKNOWN,
      DownEventFormFactor::kTabletModeLandscape, AppType::ARC_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kUnknownTabletLandscapeArcApp),
      1);

  CreateDownEventWithGivenCombination(
      ui::EventPointerType::POINTER_TYPE_UNKNOWN,
      DownEventFormFactor::kTabletModePortrait, AppType::OTHERS);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kUnknownTabletPortraitOthers),
      1);

  CreateDownEventWithGivenCombination(
      ui::EventPointerType::POINTER_TYPE_UNKNOWN,
      DownEventFormFactor::kTabletModePortrait, AppType::BROWSER);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kUnknownTabletPortraitBrowser),
      1);

  CreateDownEventWithGivenCombination(
      ui::EventPointerType::POINTER_TYPE_UNKNOWN,
      DownEventFormFactor::kTabletModePortrait, AppType::CHROME_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kUnknownTabletPortraitChromeApp),
      1);

  CreateDownEventWithGivenCombination(
      ui::EventPointerType::POINTER_TYPE_UNKNOWN,
      DownEventFormFactor::kTabletModePortrait, AppType::ARC_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kUnknownTabletPortraitArcApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_MOUSE,
                                      DownEventFormFactor::kClamshell,
                                      AppType::OTHERS);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kMouseClamshellOthers),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_MOUSE,
                                      DownEventFormFactor::kClamshell,
                                      AppType::BROWSER);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kMouseClamshellBrowser),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_MOUSE,
                                      DownEventFormFactor::kClamshell,
                                      AppType::CHROME_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kMouseClamshellChromeApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_MOUSE,
                                      DownEventFormFactor::kClamshell,
                                      AppType::ARC_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kMouseClamshellArcApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_MOUSE,
                                      DownEventFormFactor::kTabletModeLandscape,
                                      AppType::OTHERS);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kMouseTabletLandscapeOthers),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_MOUSE,
                                      DownEventFormFactor::kTabletModeLandscape,
                                      AppType::BROWSER);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kMouseTabletLandscapeBrowser),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_MOUSE,
                                      DownEventFormFactor::kTabletModeLandscape,
                                      AppType::CHROME_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kMouseTabletLandscapeChromeApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_MOUSE,
                                      DownEventFormFactor::kTabletModeLandscape,
                                      AppType::ARC_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kMouseTabletLandscapeArcApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_MOUSE,
                                      DownEventFormFactor::kTabletModePortrait,
                                      AppType::OTHERS);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kMouseTabletPortraitOthers),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_MOUSE,
                                      DownEventFormFactor::kTabletModePortrait,
                                      AppType::BROWSER);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kMouseTabletPortraitBrowser),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_MOUSE,
                                      DownEventFormFactor::kTabletModePortrait,
                                      AppType::CHROME_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kMouseTabletPortraitChromeApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_MOUSE,
                                      DownEventFormFactor::kTabletModePortrait,
                                      AppType::ARC_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kMouseTabletPortraitArcApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_PEN,
                                      DownEventFormFactor::kClamshell,
                                      AppType::OTHERS);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusClamshellOthers),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_PEN,
                                      DownEventFormFactor::kClamshell,
                                      AppType::BROWSER);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusClamshellBrowser),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_PEN,
                                      DownEventFormFactor::kClamshell,
                                      AppType::CHROME_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusClamshellChromeApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_PEN,
                                      DownEventFormFactor::kClamshell,
                                      AppType::ARC_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusClamshellArcApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_PEN,
                                      DownEventFormFactor::kTabletModeLandscape,
                                      AppType::OTHERS);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletLandscapeOthers),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_PEN,
                                      DownEventFormFactor::kTabletModeLandscape,
                                      AppType::BROWSER);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletLandscapeBrowser),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_PEN,
                                      DownEventFormFactor::kTabletModeLandscape,
                                      AppType::CHROME_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletLandscapeChromeApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_PEN,
                                      DownEventFormFactor::kTabletModeLandscape,
                                      AppType::ARC_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletLandscapeArcApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_PEN,
                                      DownEventFormFactor::kTabletModePortrait,
                                      AppType::OTHERS);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletPortraitOthers),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_PEN,
                                      DownEventFormFactor::kTabletModePortrait,
                                      AppType::BROWSER);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletPortraitBrowser),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_PEN,
                                      DownEventFormFactor::kTabletModePortrait,
                                      AppType::CHROME_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletPortraitChromeApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_PEN,
                                      DownEventFormFactor::kTabletModePortrait,
                                      AppType::ARC_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletPortraitArcApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_TOUCH,
                                      DownEventFormFactor::kClamshell,
                                      AppType::OTHERS);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusClamshellOthers),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_TOUCH,
                                      DownEventFormFactor::kClamshell,
                                      AppType::BROWSER);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusClamshellBrowser),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_TOUCH,
                                      DownEventFormFactor::kClamshell,
                                      AppType::CHROME_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusClamshellChromeApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_TOUCH,
                                      DownEventFormFactor::kClamshell,
                                      AppType::ARC_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusClamshellArcApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_TOUCH,
                                      DownEventFormFactor::kTabletModeLandscape,
                                      AppType::OTHERS);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletLandscapeOthers),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_TOUCH,
                                      DownEventFormFactor::kTabletModeLandscape,
                                      AppType::BROWSER);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletLandscapeBrowser),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_TOUCH,
                                      DownEventFormFactor::kTabletModeLandscape,
                                      AppType::CHROME_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletLandscapeChromeApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_TOUCH,
                                      DownEventFormFactor::kTabletModeLandscape,
                                      AppType::ARC_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletLandscapeArcApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_TOUCH,
                                      DownEventFormFactor::kTabletModePortrait,
                                      AppType::OTHERS);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletPortraitOthers),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_TOUCH,
                                      DownEventFormFactor::kTabletModePortrait,
                                      AppType::BROWSER);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletPortraitBrowser),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_TOUCH,
                                      DownEventFormFactor::kTabletModePortrait,
                                      AppType::CHROME_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletPortraitChromeApp),
      1);

  CreateDownEventWithGivenCombination(ui::EventPointerType::POINTER_TYPE_TOUCH,
                                      DownEventFormFactor::kTabletModePortrait,
                                      AppType::ARC_APP);
  histogram_tester_->ExpectBucketCount(
      kCombinationHistogramName,
      static_cast<int>(DownEventInputFormFactorDestinationCombination::
                           kStylusTabletPortraitArcApp),
      1);

  histogram_tester_->ExpectTotalCount(
      kCombinationHistogramName,
      static_cast<int>(
          DownEventInputFormFactorDestinationCombination::kCombinationCount));
}

}  // namespace ash
