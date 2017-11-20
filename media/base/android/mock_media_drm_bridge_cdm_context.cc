// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/android/mock_media_drm_bridge_cdm_context.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/threading/thread_task_runner_handle.h"

using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Not;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

namespace media {

MockMediaDrmBridgeCdmContext::MockMediaDrmBridgeCdmContext(int cdm_id,
                                                           bool secure_codec)
    : require_secure_video_codec_(secure_codec) {
  // Provide some sane defaults.
  ON_CALL(*this, GetCdmId()).WillByDefault(Return(cdm_id));
  ON_CALL(*this, RegisterPlayer(_, _))
      .WillByDefault(DoAll(SaveArg<0>(&new_key_cb), SaveArg<1>(&cdm_unset_cb),
                           Return(kRegistrationId)));
  ON_CALL(*this, SetMediaCryptoReadyCB(_))
      .WillByDefault(SaveArg<0>(&media_crypto_ready_cb));

  // Allow any number of calls to UnregisterPlayer, but get mad if
  // they provide the wrong registration id.
  EXPECT_CALL(*this, UnregisterPlayer(Not(kRegistrationId))).Times(0);
}

MockMediaDrmBridgeCdmContext::~MockMediaDrmBridgeCdmContext() {}

/*
void MockMediaDrmBridgeCdmContext::SetMediaCryptoReadyCB(const
MediaCryptoReadyCB& cb) { media_crypto_ready_cb = cb;
    // TODO(liberato):maybe support an option not to do this.
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
base::Bind(media_crypto_ready_cb, nullptr, require_secure_video_codec_));
}
*/

}  // namespace media
