// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_METRICS_HISTOGRAMS_HISTOGRAM_SUBSCRIBER_H_
#define SERVICES_METRICS_HISTOGRAMS_HISTOGRAM_SUBSCRIBER_H_

#include <string>
#include <vector>

namespace metrics {

// Objects interested in receiving histograms derive from HistogramSubscriber.
class HistogramSubscriber {
 public:
  virtual ~HistogramSubscriber() {}

  // Send number of pending clients to subscriber. |end| is set to true if it
  // is the last time. This is called on the UI thread.
  virtual void OnPendingClients(int sequence_number,
                                int pending_clients,
                                bool end) = 0;

  // Send |histogram| back to subscriber.
  // This is called on the UI thread.
  virtual void OnHistogramDataCollected(
      int sequence_number,
      const std::vector<std::string>& pickled_histograms) = 0;
};

}  // namespace metrics

#endif  // SERVICES_METRICS_HISTOGRAMS_HISTOGRAM_SUBSCRIBER_H_
