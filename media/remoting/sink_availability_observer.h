// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_REMOTING_SINK_AVAILABILITY_OBSERVER_H_
#define MEDIA_REMOTING_SINK_AVAILABILITY_OBSERVER_H_

#include "media/mojo/interfaces/remoting.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {
namespace remoting {

// Implements the RemotingSource interface for the sole purpose of monitoring
// sink availability status.
class SinkAvailabilityObserver final : public mojom::RemotingSource {
 public:
  SinkAvailabilityObserver(mojom::RemotingSourceRequest source_request,
                           mojom::RemoterPtr remoter);
  ~SinkAvailabilityObserver() override;

  mojom::RemotingSinkMetadataPtr sink_metadata() const {
    return sink_metadata_.Clone();
  }

  bool is_remote_decryption_available() const {
    return std::find(std::begin(sink_metadata_.features),
                     std::end(sink_metadata_.features),
                     mojom::RemotingSinkFeatures::CONTENT_DECRYPTION) !=
           std::end(sink_metadata_.features);
  }

  // RemotingSource implementations.
  void OnSinkAvailable(mojom::RemotingSinkMetadataPtr metadata) override;
  void OnSinkGone() override;
  void OnStarted() override {}
  void OnStartFailed(mojom::RemotingStartFailReason reason) override {}
  void OnMessageFromSink(const std::vector<uint8_t>& message) override {}
  void OnStopped(mojom::RemotingStopReason reason) override {}

 private:
  const mojo::Binding<mojom::RemotingSource> binding_;
  const mojom::RemoterPtr remoter_;

  // When the sink is available for remoting, this describes its metadata. When
  // not available, this is empty. Updated by OnSinkAvailable/Gone().
  mojom::RemotingSinkMetadata sink_metadata_;

  DISALLOW_COPY_AND_ASSIGN(SinkAvailabilityObserver);
};

}  // namespace remoting
}  // namespace media

#endif  // MEDIA_REMOTING_SINK_AVAILABILITY_OBSERVER_H_
