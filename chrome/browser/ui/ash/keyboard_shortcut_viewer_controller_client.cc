// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/keyboard_shortcut_viewer_controller_client.h"

#include "ash/public/interfaces/constants.mojom.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

KeyboardShortcutViewerControllerClient* g_instance = nullptr;

KeyboardShortcutViewerControllerClient::KSVItemInfoPtrList
MakeChromeKeyboardShortcutViewerMetadata() {
  KeyboardShortcutViewerControllerClient::KSVItemInfoPtrList shortcuts_info;
  NOTIMPLEMENTED();
  return shortcuts_info;
}

}  // namespace

KeyboardShortcutViewerControllerClient::KeyboardShortcutViewerControllerClient()
    : binding_(this) {
  // Connect to the controller in ash.
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName,
                      &keyboard_shortcut_viewer_controller_);
  // Register this object as the client interface implementation.
  KSVClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  keyboard_shortcut_viewer_controller_->SetClient(std::move(client));

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
  std::move(callback).Run(MakeChromeKeyboardShortcutViewerMetadata());
}

void KeyboardShortcutViewerControllerClient::OpenKeyboardShortcutViewer() {
  // The controller will call GetKeyboardShortcutsInfo() to get and show the
  // client side shortcut metadata.
  keyboard_shortcut_viewer_controller_->OpenKeyboardShortcutViewer();
}
