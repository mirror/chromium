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

// Form factor of the down event. This enum is used to back an UMA histogram
// and new values should be inserted immediately above FORM_FACTOR_COUNT.
enum class DownEventFormFactor {
  CLAMSHELL = 0,
  TABLET_MODE_LANDSCAPE,
  TABLET_MODE_PORTRAIT,
  FORM_FACTOR_COUNT,
};

// Input type of the down event. This enum is used to back an UMA
// histogram and new values should be inserted immediately above SOURCE_COUNT.
enum class DownEventSource {
  UNKNOWN = 0,
  MOUSE,
  STYLUS,
  TOUCH,
  SOURCE_COUNT,
};

// Input, FormFactor, and Destination Combination of the down event.
// This enum is used to back an UMA histogram and new values should
// be inserted immediately above COMBINATION_COUNT.
enum class DownEventInputFormFactorDestinationCombination {
  UNKOWN_CLAMSHELL_OTHERS = 0,
  UNKOWN_CLAMSHELL_BROWSER,
  UNKOWN_CLAMSHELL_CHROME_APP,
  UNKOWN_CLAMSHELL_ARC_APP,
  UNKOWN_TABLET_LANDSCAPE_OTHERS,
  UNKOWN_TABLET_LANDSCAPE_BROWSER,
  UNKOWN_TABLET_LANDSCAPE_CHROME_APP,
  UNKOWN_TABLET_LANDSCAPE_ARC_APP,
  UNKOWN_TABLET_PORTRAIT_OTHERS,
  UNKOWN_TABLET_PORTRAIT_BROWSER,
  UNKOWN_TABLET_PORTRAIT_CHROME_APP,
  UNKOWN_TABLET_PORTRAIT_ARC_APP,
  MOUSE_CLAMSHELL_OTHERS,
  MOUSE_CLAMSHELL_BROWSER,
  MOUSE_CLAMSHELL_CHROME_APP,
  MOUSE_CLAMSHELL_ARC_APP,
  MOUSE_TABLET_LANDSCAPE_OTHERS,
  MOUSE_TABLET_LANDSCAPE_BROWSER,
  MOUSE_TABLET_LANDSCAPE_CHROME_APP,
  MOUSE_TABLET_LANDSCAPE_ARC_APP,
  MOUSE_TABLET_PORTRAIT_OTHERS,
  MOUSE_TABLET_PORTRAIT_BROWSER,
  MOUSE_TABLET_PORTRAIT_CHROME_APP,
  MOUSE_TABLET_PORTRAIT_ARC_APP,
  STYLUS_CLAMSHELL_OTHERS,
  STYLUS_CLAMSHELL_BROWSER,
  STYLUS_CLAMSHELL_CHROME_APP,
  STYLUS_CLAMSHELL_ARC_APP,
  STYLUS_TABLET_LANDSCAPE_OTHERS,
  STYLUS_TABLET_LANDSCAPE_BROWSER,
  STYLUS_TABLET_LANDSCAPE_CHROME_APP,
  STYLUS_TABLET_LANDSCAPE_ARC_APP,
  STYLUS_TABLET_PORTRAIT_OTHERS,
  STYLUS_TABLET_PORTRAIT_BROWSER,
  STYLUS_TABLET_PORTRAIT_CHROME_APP,
  STYLUS_TABLET_PORTRAIT_ARC_APP,
  TOUCH_CLAMSHELL_OTHERS,
  TOUCH_CLAMSHELL_BROWSER,
  TOUCH_CLAMSHELL_CHROME_APP,
  TOUCH_CLAMSHELL_ARC_APP,
  TOUCH_TABLET_LANDSCAPE_OTHERS,
  TOUCH_TABLET_LANDSCAPE_BROWSER,
  TOUCH_TABLET_LANDSCAPE_CHROME_APP,
  TOUCH_TABLET_LANDSCAPE_ARC_APP,
  TOUCH_TABLET_PORTRAIT_OTHERS,
  TOUCH_TABLET_PORTRAIT_BROWSER,
  TOUCH_TABLET_PORTRAIT_CHROME_APP,
  TOUCH_TABLET_PORTRAIT_ARC_APP,
  COMBINATION_COUNT,
};

int GetDestination(views::Widget* target) {
  if (!target)
    return static_cast<int>(AppType::OTHERS);

  aura::Window* window = target->GetNativeWindow();
  DCHECK(window);
  return window->GetProperty(aura::client::kAppType);
}

int FindCombination(int input_type, int form_factor, int destination) {
  int num_combination_per_input =
      kAppCount * static_cast<int>(DownEventFormFactor::FORM_FACTOR_COUNT);
  return input_type * num_combination_per_input + form_factor * kAppCount +
         destination;
}

void RecordUMA(ui::EventPointerType type, views::Widget* target) {
  DownEventFormFactor form_factor = DownEventFormFactor::CLAMSHELL;
  if (Shell::Get()
          ->tablet_mode_controller()
          ->IsTabletModeWindowManagerEnabled()) {
    blink::WebScreenOrientationLockType screen_orientation =
        Shell::Get()->screen_orientation_controller()->GetCurrentOrientation();
    if (screen_orientation ==
            blink::kWebScreenOrientationLockLandscapePrimary ||
        screen_orientation ==
            blink::kWebScreenOrientationLockLandscapeSecondary) {
      form_factor = DownEventFormFactor::TABLET_MODE_LANDSCAPE;
    } else {
      form_factor = DownEventFormFactor::TABLET_MODE_PORTRAIT;
    }
  }

  DownEventSource input_type = DownEventSource::UNKNOWN;
  switch (type) {
    case ui::EventPointerType::POINTER_TYPE_UNKNOWN:
      input_type = DownEventSource::UNKNOWN;
      break;
    case ui::EventPointerType::POINTER_TYPE_MOUSE:
      input_type = DownEventSource::MOUSE;
      break;
    case ui::EventPointerType::POINTER_TYPE_PEN:
      input_type = DownEventSource::STYLUS;
      break;
    case ui::EventPointerType::POINTER_TYPE_TOUCH:
      input_type = DownEventSource::TOUCH;
      break;
    case ui::EventPointerType::POINTER_TYPE_ERASER:
      input_type = DownEventSource::STYLUS;
      break;
  }

  UMA_HISTOGRAM_ENUMERATION(
      "Event.DownEventCount.PerInputFormFactorDestinationCombination",
      FindCombination(static_cast<int>(input_type),
                      static_cast<int>(form_factor), GetDestination(target)),
      static_cast<base::HistogramBase::Sample>(
          DownEventInputFormFactorDestinationCombination::COMBINATION_COUNT));
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
