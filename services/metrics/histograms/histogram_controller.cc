// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/histograms/histogram_controller.h"

#include "base/bind.h"
#include "services/metrics/histograms/histogram_subscriber.h"

namespace metrics {

HistogramController* HistogramController::GetInstance() {
  return base::Singleton<HistogramController>::get();
}

HistogramController::HistogramController() : subscriber_(nullptr) {}

HistogramController::~HistogramController() {}

void HistogramController::Register(HistogramSubscriber* subscriber) {
  DCHECK(!subscriber_);
  subscriber_ = subscriber;
}

void HistogramController::Unregister(const HistogramSubscriber* subscriber) {
  DCHECK_EQ(subscriber_, subscriber);
  subscriber_ = NULL;
}

void HistogramController::GetHistogramData(int sequence_number) {
  int pending_clients = 0;
  clients_.ForAllPtrs([this, sequence_number, &pending_clients](
                          metrics::mojom::HistogramCollectorClient* client) {
    client->RequestNonPersistentHistogramData(
        base::BindOnce(&HistogramController::OnHistogramDataCollected,
                       base::Unretained(this), sequence_number));
    pending_clients += 1;
  });

  if (subscriber_)
    subscriber_->OnPendingClients(sequence_number, pending_clients,
                                  true /* end */);
}

void HistogramController::RegisterClient(
    metrics::mojom::HistogramCollectorClientPtr client) {
  clients_.AddPtr(std::move(client));
}

void HistogramController::OnHistogramDataCollected(
    int sequence_number,
    const std::vector<std::string>& pickled_histograms) {
  if (subscriber_) {
    subscriber_->OnHistogramDataCollected(sequence_number, pickled_histograms);
  }
}

}  // namespace metrics
