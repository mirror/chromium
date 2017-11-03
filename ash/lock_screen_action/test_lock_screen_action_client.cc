// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/lock_screen_action/test_lock_screen_action_client.h"

#include "mojo/public/cpp/bindings/interface_request.h"

namespace ash {

TestLockScreenActionClient::TestLockScreenActionClient() : binding_(this) {}

TestLockScreenActionClient::~TestLockScreenActionClient() = default;

void TestLockScreenActionClient::ClearRecordedRequests() {
  note_origins_.clear();
  close_note_reasons_.clear();
}

void TestLockScreenActionClient::RequestNewNote(
    mojom::LockScreenNoteOrigin origin) {
  note_origins_.push_back(origin);
}

void TestLockScreenActionClient::CloseNote(
    mojom::CloseLockScreenNoteReason reason) {
  close_note_reasons_.push_back(reason);
}

mojom::LockScreenActionClientPtr
TestLockScreenActionClient::CreateInterfacePtrAndBind() {
  mojom::LockScreenActionClientPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

}  // namespace ash
