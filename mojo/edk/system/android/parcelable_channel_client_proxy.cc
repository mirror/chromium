// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/android/parcelable_channel_client_proxy.h"

#include "base/android/jni_android.h"
#include "mojo/edk/jni/ParcelableChannelClientProxy_jni.h"

using base::android::ScopedJavaLocalRef;

namespace mojo {
namespace edk {

ParcelableChannelClientProxy::ParcelableChannelClientProxy()
    : j_parcelable_channel_(Java_ParcelableChannelClientProxy_create(
          base::android::AttachCurrentThread())) {}

ParcelableChannelClientProxy::~ParcelableChannelClientProxy() {}

ScopedJavaLocalRef<jobject>
ParcelableChannelClientProxy::GetIParcelableChannelProvider() {
  return Java_ParcelableChannelClientProxy_getIParcelableChannelProvider(
      base::android::AttachCurrentThread(), j_parcelable_channel_);
}

void ParcelableChannelClientProxy::SendParcelable(
    uint32_t id,
    ScopedJavaLocalRef<jobject> parcelable) {
  Java_ParcelableChannelClientProxy_sendParcelable(
      base::android::AttachCurrentThread(), j_parcelable_channel_, id,
      parcelable);
}

}  // namespace edk
}  // namespace mojo