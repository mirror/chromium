// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_METRICS_PUBLIC_CPP_HISTOGRAM_COLLECTOR_CLIENT_H_
#define SERVICES_METRICS_PUBLIC_CPP_HISTOGRAM_COLLECTOR_CLIENT_H_

#include <memory>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/buffer.h"
#include "services/metrics/public/interfaces/histogram.mojom.h"

namespace base {
class HistogramDeltaSerialization;
}

namespace service_manager {
class Connector;
}

namespace metrics {

// This class acts as a client to the HistogramCollector interface hosted in
// the metrics service. It registers itself at contruction with this collector,
// which then may query it periodically for histogram updates.
class HistogramCollectorClient : private mojom::HistogramCollectorClient {
 public:
  // |connector| is used to connect to the HistogramCollector.
  explicit HistogramCollectorClient(service_manager::Connector* connector);

  // |connector| is used to connect ot the HistogramCollector. |process_name|
  // is the name of the content subprocess in which this client exists. This
  // will be used by the HistogramCollector to allocate shared memory.
  HistogramCollectorClient(
      service_manager::Connector* connector,
      metrics::CallStackProfileParams::Process process_type);

  ~HistogramCollectorClient() override;

 private:
  // Connect to the HistogramCollector. Called from the constructor.
  void ConnectInternal(service_manager::Connector* connector,
                       metrics::CallStackProfileParams::Process process_type);

  // mojom::HistogramCollectorClient implementation:
  void RequestNonPersistentHistogramData(
      mojom::HistogramCollectorClient::RequestNonPersistentHistogramDataCallback
          callback) override;

  // This will be called when the HistogramCollector has registered this
  // object as a histogram client.
  void OnClientRegistered(mojo::ScopedSharedBufferHandle handle);

  std::unique_ptr<base::HistogramDeltaSerialization>
      histogram_delta_serialization_;
  mojo::Binding<mojom::HistogramCollectorClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(HistogramCollectorClient);
};

}  // namespace metrics

#endif  // SERVICES_METRICS_PUBLIC_CPP_HISTOGRAM_COLLECTOR_CLIENT_H_
