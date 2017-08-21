// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/android/parcelable_channel_server.h"

#include "mojo/edk/jni/ParcelableChannelServer_jni.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;
using base::android::JavaRef;

namespace mojo {
namespace edk {

ParcelableChannelServer::ParcelableChannelServer()
    : j_parcelable_channel_(Java_ParcelableChannelServer_create(
          AttachCurrentThread(),
          reinterpret_cast<intptr_t>(this))) {}

ParcelableChannelServer::~ParcelableChannelServer() {
  Java_ParcelableChannelServer_invalidate(AttachCurrentThread(),
                                          j_parcelable_channel_);
}

ParcelableChannelServer::ParcelableChannelServer(
    ParcelableChannelServer&& other) = default;

ParcelableChannelServer& ParcelableChannelServer::operator=(
    ParcelableChannelServer&& other) = default;

void ParcelableChannelServer::SetListener(Listener* listener) {
  DCHECK(!listener_);
  listener_ = listener;
}

ScopedJavaGlobalRef<jobject> ParcelableChannelServer::GetIParcelableChannel() {
  // The ParcelableChannelServer java class implements IParcelableChannel.
  return j_parcelable_channel_;
}

bool ParcelableChannelServer::SetOnChannelProvider(
    const JavaRef<jobject>& channel_provider) {
  DCHECK(!channel_provider.is_null());
  return Java_ParcelableChannelServer_setChannelOnParent(
      AttachCurrentThread(), j_parcelable_channel_, channel_provider);
}

void ParcelableChannelServer::OnParcelableReceived(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jint id,
    const base::android::JavaParamRef<jobject>& parcelable) {
  DCHECK(listener_);
  if (listener_) {
    listener_->OnParcelableReceived(id, parcelable);
  }
}

}  // namespace edk
}  // namespace mojo