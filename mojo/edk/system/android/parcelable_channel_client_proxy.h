// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_ANDROID_PARCELABLE_CHANNEL_CLIENT_PROXY_H_
#define MOJO_EDK_SYSTEM_ANDROID_PARCELABLE_CHANNEL_CLIENT_PROXY_H_

#include "base/android/scoped_java_ref.h"
#include "mojo/edk/system/android/parcelable_channel_client.h"
#include "mojo/edk/system/system_impl_export.h"

namespace mojo {
namespace edk {

// Wraps a Java ParcelableChannelClientProxy so it can be used from native code.
class MOJO_SYSTEM_IMPL_EXPORT ParcelableChannelClientProxy
    : public ParcelableChannelClient {
 public:
  ParcelableChannelClientProxy();
  ~ParcelableChannelClientProxy() override;

  base::android::ScopedJavaLocalRef<jobject> GetIParcelableChannelProvider();

  // Sends the |parcelable| over the channel.
  void SendParcelable(
      uint32_t id,
      base::android::ScopedJavaLocalRef<jobject> parcelable) override;

 private:
  base::android::ScopedJavaGlobalRef<jobject> j_parcelable_channel_;

  DISALLOW_COPY_AND_ASSIGN(ParcelableChannelClientProxy);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_ANDROID_PARCELABLE_CHANNEL_CLIENT_PROXY_H_