// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/keyboard_shortcut_viewer_controller_client.h"

#include "ash/public/interfaces/constants.mojom.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

KeyboardShortcutViewerControllerClient* g_instance = nullptr;

}  // namespace

KeyboardShortcutViewerControllerClient::KeyboardShortcutViewerControllerClient()
    : binding_(this) {
  // Connect to the controller in ash.
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName, &ksv_controller_);
  // Register this object as the client interface implementation.
  keyboard_shortcut_viewer::KSVClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  ksv_controller_->SetClient(std::move(client));

  DCHECK(!g_instance);
  g_instance = this;
}

KeyboardShortcutViewerControllerClient::
    ~KeyboardShortcutViewerControllerClient() {
  DCHECK_EQ(this, g_instance);
  g_instance = nullptr;
}

// static
KeyboardShortcutViewerControllerClient*
KeyboardShortcutViewerControllerClient::Get() {
  return g_instance;
}

void KeyboardShortcutViewerControllerClient::GetKeyboardShortcutsInfo(
    GetKeyboardShortcutsInfoCallback callback) {
  std::move(callback).Run(GetChromeShortcutsMetadata());
}

void KeyboardShortcutViewerControllerClient::OpenKeyboardShortcutViewer() {
  // The controller will call GetKeyboardShortcutsInfo() to get and show the
  // client side shortcut metadata.
  ksv_controller_->OpenKeyboardShortcutViewer();
}

keyboard_shortcut_viewer::KSVItemInfoPtrList
KeyboardShortcutViewerControllerClient::GetChromeShortcutsMetadata() {
  keyboard_shortcut_viewer::KSVItemInfoPtrList shortcuts_info;

  if (chrome_shortcuts_metadata_.empty()) {
    // TODO(wutao): load chrome shortcuts metadata.
    NOTIMPLEMENTED();
  }

  for (auto info : chrome_shortcuts_metadata_) {
    shortcuts_info.emplace_back(
        keyboard_shortcut_viewer::KSVItemInfo::New(info));
  }
  return shortcuts_info;
}
