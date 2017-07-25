// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/remoting/rpc_broker_stub_android.h"

namespace media {

namespace remoting {

RpcBroker::RpcBroker(const SendMessageCallback& send_message_cb)
    : weak_factory_(this) {}

RpcBroker::~RpcBroker() {}

}  // namespace remoting

}  // namespace media