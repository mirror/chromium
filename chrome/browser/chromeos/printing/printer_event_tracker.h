// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_PRINTER_EVENT_TRACKER_H_
#define CHROME_BROWSER_CHROMEOS_PRINTING_PRINTER_EVENT_TRACKER_H_

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/metrics/proto/printer_event_proto.pb.h"

namespace chromeos {

// Aggregates printer events for logging.
class PrinterEventTracker : public KeyedService {
 public:
  PrinterEventTracker();

  // Store a succesful printer installation.
  void RecordPrinterInstalled(const Printer& printer);

  // Store a printer removal.
  void RecordPrinterRemoved(const Printer& printer);

  // Writes stored events to |events|.
  void FlushPrinterEvents(std::vector<metrics::PrinterEventProto>* events);

 private:
  std::vector<std::unique_ptr<metrics::PrinterEventProto>> events_;

  DISALLOW_COPY_AND_ASSIGN(PrinterEventTracker);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_PRINTER_EVENT_TRACKER_H_
