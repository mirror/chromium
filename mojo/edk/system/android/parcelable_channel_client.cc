// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/android/parcelable_channel_client.h"

#include "mojo/edk/system/android/jni/ParcelableChannelClient_jni.h"

namespace mojo {
namespace edk {

// static
ParcelableChannelClient ParcelableChannelClient::CreateProxyChannelClient() {}

// static
ParcelableChannelClient ParcelableChannelClientCreateChannelClient(
    const base::android::JavaRef<jobject>& iparcelable_channel) {}

ParcelableChannelClient::ParcelableChannelClient() {}

ParcelableChannelClient::~ParcelableChannelClient() {}

ParcelableChannelClient::ParcelableChannelClient(
    const base::android::JavaRef<jobject>& j_parcelable_channel)
    : j_parcelable_channel_(j_parcelable_channel) {
  DCHECK(!j_parcelable_channel_.is_null());
}

ParcelableChannelClient::ParcelableChannelClient(
    ParcelableChannelClient&& other) = default;

ParcelableChannelClient& ParcelableChannelClient::operator=(
    ParcelableChannelClient&& other) = default;

void ParcelableChannelClient::SendParcelable(
    uint32_t id,
    base::android::ScopedJavaLocalRef<jobject> parcelable) {
  DCHECK(!j_parcelable_channel_.is_null());

  Java_ParcelableChannelClient_sendParcelable(
      base::android::AttachCurrentThread(), j_parcelable_channel_, id,
      parcelable);
}

}  // namespace edk
}  // namespace mojo