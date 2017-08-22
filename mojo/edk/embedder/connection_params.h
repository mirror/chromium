// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_CONNECTION_PARAMS_H_
#define MOJO_EDK_EMBEDDER_CONNECTION_PARAMS_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/embedder/transport_protocol.h"
#include "mojo/edk/system/system_impl_export.h"

#if defined(OS_ANDROID)
#include "base/android/scoped_java_ref.h"
#include "mojo/edk/embedder/parcelable_channel.h"
#include "mojo/edk/system/android/parcelable_channel_client.h"
#include "mojo/edk/system/android/parcelable_channel_server.h"
#endif

namespace mojo {
namespace edk {

// A set of parameters used when establishing a connection to another process.
class MOJO_SYSTEM_IMPL_EXPORT ConnectionParams {
 public:
  // Configures an OS pipe-based connection of type |type| to the remote process
  // using the given transport |protocol|.
  ConnectionParams(TransportProtocol protocol, ScopedPlatformHandle channel);

  ConnectionParams(ConnectionParams&& params);
  ConnectionParams& operator=(ConnectionParams&& params);

  TransportProtocol protocol() const { return protocol_; }

  ScopedPlatformHandle TakeChannelHandle();

#if defined(OS_ANDROID)
  void SetParcelableChannel(ParcelableChannel channel);

  ParcelableChannel TakeParcelableChannel();

  std::unique_ptr<ParcelableChannelClient> TakeParcelableChannelClient();
  std::unique_ptr<ParcelableChannelServer> TakeParcelableChannelServer();
#endif

 private:
  TransportProtocol protocol_;
  ScopedPlatformHandle channel_;

#if defined(OS_ANDROID)
  ParcelableChannel parcelable_channel_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ConnectionParams);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_CONNECTION_PARAMS_H_
