// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_ANDROID_PARCELABLE_CHANNEL_CLIENT_H_
#define MOJO_EDK_SYSTEM_ANDROID_PARCELABLE_CHANNEL_CLIENT_H_

#include "base/android/scoped_java_ref.h"
#include "mojo/edk/system/system_impl_export.h"

namespace mojo {
namespace edk {

// Interface that provides a way of sending a Parcelable object.
class MOJO_SYSTEM_IMPL_EXPORT ParcelableChannelClient {
 public:
  ParcelableChannelClient() {}
  virtual ~ParcelableChannelClient() {}

  // Sends the |parcelable| over the channel.
  virtual void SendParcelable(
      uint32_t id,
      base::android::ScopedJavaLocalRef<jobject> parcelable) = 0;
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_ANDROID_PARCELABLE_CHANNEL_CLIENT_H_