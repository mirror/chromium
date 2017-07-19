// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/printer_event_provider.h"

#include "chrome/browser/chromeos/printing/printer_event_tracker.h"
#include "chrome/browser/chromeos/printing/printer_event_tracker_factory.h"
#include "components/metrics/proto/chrome_user_metrics_extension.pb.h"
#include "components/metrics/proto/printer_event.pb.h"

namespace chromeos {

PrinterEventProvider::PrinterEventProvider() = default;
PrinterEventProvider::~PrinterEventProvider() = default;

void PrinterEventProvider::OnRecordingEnabled() {
  PrinterEventTrackerFactory* factory =
      PrinterEventTrackerFactory::GetInstance();
  std::vector<Profile*> profiles =
      g_browser_procecss->profile_manager()->GetLoadedProfiles();
  for (Profile* profile : profiles) {
    factory->GetForBrowserContext(profile)->EnableRecording();
  }
}

void PrinterEventProvider::OnRecordingDisabled() {
  PrinterEventTrackerFactory* factory =
      PrinterEventTrackerFactory::GetInstance();
  std::vector<Profile*> profiles =
      g_browser_procecss->profile_manager()->GetLoadedProfiles();
  for (Profile* profile : profiles) {
    factory->GetForBrowserContext(profile)->DisableRecording();
  }
}

PrinterEventProvider::ProvideGeneralMetrics(
    metrics::ChromeUserMetricsExtension* uma_proto) {
  PrinterEventTrackerFactory* factory =
      PrinterEventTrackerFactory::GetInstance();
  std::vector<Profile*> profiles =
      g_browser_procecss->profile_manager()->GetLoadedProfiles();
  for (Profile* profile : profiles) {
    std::vector<metrics::PrinterEventProto> events =
        factory->GetForBrowserContext(profile)->FlushPrinterEvents();

    for (metrics::PrinterEvent& event : events) {
      uma_proto->add_printer_event()->Swap(&event);
    }
  }
}

}  // namespace chromeos
