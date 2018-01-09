// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/metrics/pointer_metrics_recorder.h"

#include "ash/display/screen_orientation_controller_chromeos.h"
#include "ash/public/cpp/app_types.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/metrics/histogram_macros.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/events/event_constants.h"
#include "ui/views/widget/widget.h"

namespace ash {

namespace {

// Form factor of the down event.
// This enum is used to control a UMA histogram buckets and new values
// should be inserted immediately above kFormFactorCount.
enum class DownEventFormFactor {
  kClamshell = 0,
  kTabletModeLandscape,
  kTabletModePortrait,
  kFormFactorCount,
};

// Input type of the down event.
// This enum is used to control a UMA histogram buckets and new values
// should be inserted immediately above kSourceCount.
enum class DownEventSource {
  kUnknown = 0,
  kMouse,
  kStylus,
  kTouch,
  kSourceCount,
};

// Input, FormFactor, and Destination Combination of the down event.
// This enum is used to back an UMA histogram and new values should
// be inserted immediately above kCombinationCount.
enum class DownEventInputFormFactorDestinationCombination {
  kUnknownClamshellOthers = 0,
  kUnknownClamshellBrowser,
  kUnknownClamshellChromeApp,
  kUnknownClamshellArcApp,
  kUnknownTabletLandscapeOthers,
  kUnknownTabletLandscapeBrowser,
  kUnknownTabletLandscapeChromeApp,
  kUnknownTabletLandscapeArcApp,
  kUnknownTabletPortraitOthers,
  kUnknownTabletPortraitBrowser,
  kUnknownTabletPortraitChromeApp,
  kUnknownTabletPortraitArcApp,
  kMouseClamshellOthers,
  kMouseClamshellBrowser,
  kMouseClamshellChromeApp,
  kMouseClamshellArcApp,
  kMouseTabletLandscapeOthers,
  kMouseTabletLandscapeBrowser,
  kMouseTabletLandscapeChromeApp,
  kMouseTabletLandscapeArcApp,
  kMouseTabletPortraitOthers,
  kMouseTabletPortraitBrowser,
  kMouseTabletPortraitChromeApp,
  kMouseTabletPortraitArcApp,
  kStylusClamshellOthers,
  kStylusClamshellBrowser,
  kStylusClamshellChromeApp,
  kStylusClamshellArcApp,
  kStylusTabletLandscapeOthers,
  kStylusTabletLandscapeBrowser,
  kStylusTabletLandscapeChromeApp,
  kStylusTabletLandscapeArcApp,
  kStylusTabletPortraitOthers,
  kStylusTabletPortraitBrowser,
  kStylusTabletPortraitChromeApp,
  kStylusTabletPortraitArcApp,
  kTouchClamshellOthers,
  kTouchClamshellBrowser,
  kTouchClamshellChromeApp,
  kTouchClamshellArcApp,
  kTouchTabletLandscapeOthers,
  kTouchTabletLandscapeBrowser,
  kTouchTabletLandscapeChromeApp,
  kTouchTabletLandscapeArcApp,
  kTouchTabletPortraitOthers,
  kTouchTabletPortraitBrowser,
  kTouchTabletPortraitChromeApp,
  kTouchTabletPortraitArcApp,
  kCombinationCount,
};

int GetDestination(views::Widget* target) {
  if (!target)
    return static_cast<int>(AppType::OTHERS);

  aura::Window* window = target->GetNativeWindow();
  DCHECK(window);
  return window->GetProperty(aura::client::kAppType);
}

// Find the input type, form factor and destination combination of the down
// event. Used to get the UMA histogram bucket.
DownEventInputFormFactorDestinationCombination
FindCombination(int input_type, int form_factor, int destination) {
  int num_combination_per_input =
      kAppCount * static_cast<int>(DownEventFormFactor::kFormFactorCount);
  int result = input_type * num_combination_per_input +
               form_factor * kAppCount + destination;
  DCHECK(result >= 0 &&
         result <
             static_cast<int>(DownEventInputFormFactorDestinationCombination::
                                  kCombinationCount));
  return static_cast<DownEventInputFormFactorDestinationCombination>(result);
}

void RecordUMA(ui::EventPointerType type, views::Widget* target) {
  DownEventFormFactor form_factor = DownEventFormFactor::kClamshell;
  if (Shell::Get()
          ->tablet_mode_controller()
          ->IsTabletModeWindowManagerEnabled()) {
    blink::WebScreenOrientationLockType screen_orientation =
        Shell::Get()->screen_orientation_controller()->GetCurrentOrientation();
    if (screen_orientation ==
            blink::kWebScreenOrientationLockLandscapePrimary ||
        screen_orientation ==
            blink::kWebScreenOrientationLockLandscapeSecondary) {
      form_factor = DownEventFormFactor::kTabletModeLandscape;
    } else {
      form_factor = DownEventFormFactor::kTabletModePortrait;
    }
  }

  DownEventSource input_type = DownEventSource::kUnknown;
  switch (type) {
    case ui::EventPointerType::POINTER_TYPE_UNKNOWN:
      input_type = DownEventSource::kUnknown;
      break;
    case ui::EventPointerType::POINTER_TYPE_MOUSE:
      input_type = DownEventSource::kMouse;
      break;
    case ui::EventPointerType::POINTER_TYPE_PEN:
      input_type = DownEventSource::kStylus;
      break;
    case ui::EventPointerType::POINTER_TYPE_TOUCH:
      input_type = DownEventSource::kTouch;
      break;
    case ui::EventPointerType::POINTER_TYPE_ERASER:
      input_type = DownEventSource::kStylus;
      break;
  }

  UMA_HISTOGRAM_ENUMERATION(
      "Event.DownEventCount.PerInputFormFactorDestinationCombination",
      FindCombination(static_cast<int>(input_type),
                      static_cast<int>(form_factor), GetDestination(target)),
      DownEventInputFormFactorDestinationCombination::kCombinationCount);
}

}  // namespace

PointerMetricsRecorder::PointerMetricsRecorder() {
  ShellPort::Get()->AddPointerWatcher(this,
                                      views::PointerWatcherEventTypes::BASIC);
}

PointerMetricsRecorder::~PointerMetricsRecorder() {
  ShellPort::Get()->RemovePointerWatcher(this);
}

void PointerMetricsRecorder::OnPointerEventObserved(
    const ui::PointerEvent& event,
    const gfx::Point& location_in_screen,
    gfx::NativeView target) {
  if (event.type() == ui::ET_POINTER_DOWN)
    RecordUMA(event.pointer_details().pointer_type,
              views::Widget::GetTopLevelWidgetForNativeView(target));
}

}  // namespace ash
