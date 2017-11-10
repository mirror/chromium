// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_QUIC_CORE_FRAMES_QUIC_CONNECTIVITY_PROBING_FRAME_H_
#define NET_QUIC_CORE_FRAMES_QUIC_CONNECTIVITY_PROBING_FRAME_H_

#include "net/quic/platform/api/quic_export.h"

namespace net {

// A path connectivity probing frame contains no payload and is serialized as a
// ping frame.
struct QUIC_EXPORT_PRIVATE QuicConnectivityProbingFrame {};

}  // namespace net

#endif  // NET_QUIC_CORE_FRAMES_QUIC_CONNECTIVITY_PROBING_FRAME_H_
