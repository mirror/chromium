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

// This class acts as a client to the HistogramController interface hosted in
// the browser process. It registers itself at contruction with this controller,
// which then may query it periodically for histogram updates.
class HistogramControllerClient : private mojom::HistogramControllerClient {
 public:
  // |connector| is used to connect ot the HistogramController in the browser
  // process.
  explicit HistogramControllerClient(service_manager::Connector* connector);
  ~HistogramControllerClient() override;

 private:
  // mojom::HistogramControllerClient implementation:
  void RequestNonPersistentHistogramData(
      mojom::HistogramControllerClient::
          RequestNonPersistentHistogramDataCallback callback) override;

  // This will be called when the HistogramController has registered this
  // object as a histogram client.
  void OnClientRegistered(mojo::ScopedSharedBufferHandle handle);

  std::unique_ptr<base::HistogramDeltaSerialization>
      histogram_delta_serialization_;
  mojo::Binding<mojom::HistogramControllerClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(HistogramControllerClient);
};

}  // namespace content
