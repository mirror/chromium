// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/connection_params.h"

#include <utility>

#include "base/logging.h"

namespace mojo {
namespace edk {

ConnectionParams::ConnectionParams(TransportProtocol protocol,
                                   ScopedPlatformHandle channel)
    : protocol_(protocol), channel_(std::move(channel)) {
  // TODO(rockot): Support other protocols.
  // DCHECK_EQ(TransportProtocol::kLegacy, protocol);
}

ConnectionParams::ConnectionParams(ConnectionParams&& params) = default;

ConnectionParams& ConnectionParams::operator=(ConnectionParams&& params) =
    default;

ScopedPlatformHandle ConnectionParams::TakeChannelHandle() {
  return std::move(channel_);
}

#if defined(OS_ANDROID)
void ConnectionParams::SetParcelableChannel(
    ParcelableChannel parcelable_channel) {
  parcelable_channel_ = std::move(parcelable_channel);
}

ParcelableChannel ConnectionParams::TakeParcelableChannel() {
  return std::move(parcelable_channel_);
}

std::unique_ptr<ParcelableChannelClient>
ConnectionParams::TakeParcelableChannelClient() {
  return parcelable_channel_.TakeParcelableChannelClient();
}

std::unique_ptr<ParcelableChannelServer>
ConnectionParams::TakeParcelableChannelServer() {
  return parcelable_channel_.TakeParcelableChannelServer();
}
#endif

}  // namespace edk
}  // namespace mojo
