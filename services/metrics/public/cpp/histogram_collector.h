// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_METRICS_PUBLIC_CPP_HISTOGRAM_COLLECTOR_H_
#define SERVICES_METRICS_PUBLIC_CPP_HISTOGRAM_COLLECTOR_H_

#include <string>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/metrics/histograms/metrics_service_histogram_provider.h"
#include "services/metrics/public/interfaces/histogram.mojom.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace metrics {

// This class collects histograms from clients and updates those histograms when
// requested. This class is not thread-safe.
class HistogramCollector : public mojom::HistogramCollector {
 public:
  HistogramCollector();
  ~HistogramCollector() override;

  // Bind this interface to a request.
  void Bind(const service_manager::BindSourceInfo& source_info,
            mojom::HistogramCollectorRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  // mojom::HistogramCollector implementation.
  void RegisterClient(
      mojom::HistogramCollectorClientPtr client,
      metrics::CallStackProfileParams::Process process_type,
      mojom::HistogramCollector::RegisterClientCallback cb) override;
  void UpdateHistograms(base::TimeDelta wait_time,
                        UpdateHistogramsCallback callback) override;

 private:
  // Delcare an id that will be unique to every client which can be used to
  // identify the client for error handlers and callbacks.
  using ClientId = mojom::HistogramCollectorClient*;

  // The connection error handler for registered clients. This will be called
  // when the remote end of the pipe is torn down.
  void OnClientConnectionDestroyed(ClientId client_id);

  // Request histogram updates from all active clients. |sequence_number| is a
  // unique integer assigned to each batch of requests.
  void RequestHistogramUpdatesFromAllClients(int sequence_number);

  // Clients will call this after updating their histograms in shared memory.
  // The |client_id| and |sequence_number| are bound by the service.
  void OnHistogramDataCollected(
      ClientId client_id,
      int sequence_number,
      const std::vector<std::string>& pickled_histograms);

  // Called if the |wait_time| specified in UpdateHistograms() expires before
  // all clients respond to the request.
  void OnWaitTimeExpired(int sequence_number);

  // Post |pending_callback_| to this thread, resetting it. |complete| will be
  // true if all clients have succesfully responded. It will be false if the
  // request times out or if another request supersedes it.
  void RunPendingCallback(bool complete);

  // Used to post timeout tasks for UpdateHistograms().
  base::OneShotTimer timer_;

  // Provides histogram information from all of the clients of this interface
  // to base::StatisticsRecorder.
  MetricsServiceHistorgramProvider histogram_provider_;

  // The sequence number of the current pending request. This number will roll
  // over if necessary - it must only be unique among pending remote interface
  // calls.
  int last_used_sequence_number_ = 0;

  // The set of active client connection ids. An id is added to this set when a
  // client is registered. It is removed when the connection error handler is
  // called.
  std::unordered_set<ClientId> active_client_ids_;

  // The set of clients which have outstanding requests. This set is initialized
  // when a request is issued to all clients, and it is reduced as responses to
  // these requests come in, or when the connection error handler is called.
  std::unordered_set<ClientId> pending_client_ids_;
  UpdateHistogramsCallback pending_callback_;

  // Binding set for the remote clients.
  mojo::InterfacePtrSet<metrics::mojom::HistogramCollectorClient> clients_;
  mojo::BindingSet<mojom::HistogramCollector> bindings_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(HistogramCollector);
};

}  // namespace metrics

#endif  // SERVICES_METRICS_PUBLIC_CPP_HISTOGRAM_COLLECTOR_H_
