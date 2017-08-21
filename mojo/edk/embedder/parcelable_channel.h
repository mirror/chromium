// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_PARCELABLE_CHANNEL_H_
#define MOJO_EDK_EMBEDDER_PARCELABLE_CHANNEL_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "mojo/edk/system/system_impl_export.h"

#if defined(OS_ANDROID)
#include "base/android/scoped_java_ref.h"
#include "mojo/edk/system/android/parcelable_channel_client.h"
#include "mojo/edk/system/android/parcelable_channel_client_proxy.h"
#include "mojo/edk/system/android/parcelable_channel_server.h"
#endif

namespace mojo {
namespace edk {

class MOJO_SYSTEM_IMPL_EXPORT ParcelableChannel {
 public:
  static ParcelableChannel CreateForParentProcess();

  static ParcelableChannel CreateForChildProcess(
      const base::android::JavaRef<jobjectArray>& ibinder_list);

  ParcelableChannel();
  ~ParcelableChannel();
  ParcelableChannel(ParcelableChannel&& other);
  ParcelableChannel& operator=(ParcelableChannel&& other);

  // Returns a Java array of IBinders containing the IBinders that should be
  // sent to the child process. The child process should use
  // CreateForChildProcess with that list to create the ParcelableChannel.
  base::android::ScopedJavaLocalRef<jobjectArray> GetIBindersForClientProcess();

  std::unique_ptr<ParcelableChannelClient> TakeParcelableChannelClient();
  std::unique_ptr<ParcelableChannelServer> TakeParcelableChannelServer();

 private:
  std::unique_ptr<ParcelableChannelClientProxy> client_channel_proxy_;
  std::unique_ptr<ParcelableChannelClient> client_channel_;
  std::unique_ptr<ParcelableChannelServer> server_channel_;

  DISALLOW_COPY_AND_ASSIGN(ParcelableChannel);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_PARCELABLE_CHANNEL_H_
