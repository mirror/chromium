// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/android/parcelable_channel_client_impl.h"

#include "base/android/jni_android.h"
#include "mojo/edk/jni/ParcelableChannelClient_jni.h"

namespace mojo {
namespace edk {

ParcelableChannelClientImpl::ParcelableChannelClientImpl(
    const base::android::JavaRef<jobject>& iparcelable_channel) {
  DCHECK(!iparcelable_channel.is_null());
  j_parcelable_channel_.Reset(Java_ParcelableChannelClient_create(
      base::android::AttachCurrentThread(), iparcelable_channel));
}

ParcelableChannelClientImpl::~ParcelableChannelClientImpl() {}

ParcelableChannelClientImpl::ParcelableChannelClientImpl(
    ParcelableChannelClientImpl&& other) = default;

ParcelableChannelClientImpl& ParcelableChannelClientImpl::operator=(
    ParcelableChannelClientImpl&& other) = default;

// Sends the |parcelable| over the channel.
void ParcelableChannelClientImpl::SendParcelable(
    uint32_t id,
    base::android::ScopedJavaLocalRef<jobject> parcelable) {
  Java_ParcelableChannelClient_sendParcelable(
      base::android::AttachCurrentThread(), j_parcelable_channel_, id,
      parcelable);
}

}  // namespace edk
}  // namespace mojo
