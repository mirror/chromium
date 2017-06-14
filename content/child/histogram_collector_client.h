// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/macros.h"
#include "content/common/histogram.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/buffer.h"

namespace base {
class HistogramDeltaSerialization;
}

namespace service_manager {
class Connector;
}

namespace content {

// This class acts as a client to the HistogramCollector interface hosted in
// the browser process. It registers itself at contruction with this collector,
// which then may query it periodically for histogram updates.
class HistogramCollectorClient : private mojom::HistogramCollectorClient {
 public:
  // |connector| is used to connect ot the HistogramCollector in the browser
  // process.
  explicit HistogramCollectorClient(service_manager::Connector* connector);
  ~HistogramCollectorClient() override;

 private:
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

}  // namespace content
