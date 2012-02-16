// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/plugin/daemon_controller.h"

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/logging.h"

namespace remoting {

namespace {

class DaemonControllerWin : public remoting::DaemonController {
 public:
  DaemonControllerWin();

  virtual State GetState() OVERRIDE;
  virtual bool SetPin(const std::string& pin) OVERRIDE;
  virtual bool Start() OVERRIDE;
  virtual bool Stop() OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(DaemonControllerWin);
};

DaemonControllerWin::DaemonControllerWin() {
}

remoting::DaemonController::State DaemonControllerWin::GetState() {
  return remoting::DaemonController::STATE_NOT_IMPLEMENTED;
}

bool DaemonControllerWin::SetPin(const std::string& pin) {
  NOTIMPLEMENTED();
  return false;
}

bool DaemonControllerWin::Start() {
  NOTIMPLEMENTED();
  return false;
}

bool DaemonControllerWin::Stop() {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace

DaemonController* remoting::DaemonController::Create() {
  return new DaemonControllerWin();
}

}  // namespace remoting
