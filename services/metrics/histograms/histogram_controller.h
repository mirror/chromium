// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_METRICS_HISTOGRAMS_HISTOGRAM_CONTROLLER_H_
#define SERVICES_METRICS_HISTOGRAMS_HISTOGRAM_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/metrics/public/interfaces/histogram.mojom.h"

namespace metrics {

class HistogramSubscriber;

// HistogramController is used in the metrics service to collect histogram data.
class HistogramController {
 public:
  // Returns the HistogramController object for the current process, or NULL if
  // none.
  static HistogramController* GetInstance();

  // Normally instantiated when the child process is launched. Only one instance
  // should be created per process.
  HistogramController();
  virtual ~HistogramController();

  // Register the subscriber so that it will be called when for example
  // OnHistogramDataCollected is returning histogram data from a child process.
  // This is called on UI thread.
  void Register(HistogramSubscriber* subscriber);

  // Unregister the subscriber so that it will not be called when for example
  // OnHistogramDataCollected is returning histogram data from a child process.
  // Safe to call even if caller is not the current subscriber.
  void Unregister(const HistogramSubscriber* subscriber);

  // Contact all registered clients and get their histogram data.
  void GetHistogramData(int sequence_number);

  // Register |client| as a provider of histogram data from a remote process.
  // |client| will be queried on the calling thread for histogram data when
  // GetHistogramData() is called. Must be called on the UI or IO threads.
  void RegisterClient(metrics::mojom::HistogramCollectorClientPtr client);

 private:
  friend struct base::DefaultSingletonTraits<HistogramController>;

  // Send the |histogram| back to the |subscriber_|. This can be called from any
  // thread.
  void OnHistogramDataCollected(
      int sequence_number,
      const std::vector<std::string>& pickled_histograms);

  // Binding set for the remote clients.
  mojo::InterfacePtrSet<metrics::mojom::HistogramCollectorClient> clients_;

  HistogramSubscriber* subscriber_;

  DISALLOW_COPY_AND_ASSIGN(HistogramController);
};

}  // namespace metrics

#endif  // SERVICES_METRICS_HISTOGRAMS_HISTOGRAM_CONTROLLER_H_
