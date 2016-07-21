// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/security_key/security_key_extension.h"

#include "base/memory/ptr_util.h"
#include "remoting/host/security_key/security_key_extension_session.h"

namespace {
// TODO(joedow): Update this once clients support sending a security key
//               capabililty.  Tracked via: crbug.com/587485
const char kCapability[] = "";
}

namespace remoting {

SecurityKeyExtension::SecurityKeyExtension() {}

SecurityKeyExtension::~SecurityKeyExtension() {}

std::string SecurityKeyExtension::capability() const {
  return kCapability;
}

std::unique_ptr<HostExtensionSession>
SecurityKeyExtension::CreateExtensionSession(
    ClientSessionDetails* details,
    protocol::ClientStub* client_stub) {
  // TODO(joedow): Update this mechanism to allow for multiple sessions.  The
  //               extension will only send messages through the initial
  //               |client_stub| and |details| with the current design.
  return base::WrapUnique(
      new SecurityKeyExtensionSession(details, client_stub));
}

}  // namespace remoting
