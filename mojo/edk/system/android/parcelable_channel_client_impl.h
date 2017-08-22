// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_ANDROID_PARCELABLE_CHANNEL_CLIENT_IMPL_H_
#define MOJO_EDK_SYSTEM_ANDROID_PARCELABLE_CHANNEL_CLIENT_IMPL_H_

#include "base/android/scoped_java_ref.h"
#include "mojo/edk/system/android/parcelable_channel_client.h"

namespace mojo {
namespace edk {

// Wraps a Java ParcelableChannelClient so it can be used from native code.
class ParcelableChannelClientImpl : public ParcelableChannelClient {
 public:
  explicit ParcelableChannelClientImpl(
      const base::android::JavaRef<jobject>& iparcelable_channel);

  ParcelableChannelClientImpl(ParcelableChannelClientImpl&& other);
  ~ParcelableChannelClientImpl() override;
  ParcelableChannelClientImpl& operator=(ParcelableChannelClientImpl&& other);

  bool is_valid() const { return !j_parcelable_channel_.is_null(); }

  // Sends the |parcelable| over the channel.
  void SendParcelable(
      uint32_t id,
      base::android::ScopedJavaLocalRef<jobject> parcelable) override;

 private:
  base::android::ScopedJavaGlobalRef<jobject> j_parcelable_channel_;

  DISALLOW_COPY_AND_ASSIGN(ParcelableChannelClientImpl);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_ANDROID_PARCELABLE_CHANNEL_CLIENT_IMPL_H_