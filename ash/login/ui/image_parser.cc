// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell.h"
#include "ash/system/system_notifier.h"
#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/i18n/time_formatting.h"
#include "base/macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
// #include "chrome/app/vector_icons/vector_icons.h"
// #include "chrome/browser/browser_process.h"
// #include "chrome/browser/chromeos/drive/file_system_util.h"
// #include "chrome/browser/chromeos/file_manager/open_util.h"
// #include "chrome/browser/chromeos/note_taking_helper.h"
// #include "chrome/browser/download/download_prefs.h"
// #include "chrome/browser/notifications/notification_ui_manager.h"
// #include "chrome/browser/notifications/notifier_state_tracker.h"
// #include "chrome/browser/notifications/notifier_state_tracker_factory.h"
// #include "chrome/browser/platform_util.h"
// #include "chrome/browser/profiles/profile.h"
// #include "chrome/browser/profiles/profile_manager.h"
// #include "chrome/common/pref_names.h"
// #include "chrome/grit/generated_resources.h"
// #include "chrome/grit/theme_resources.h"
// #include "chromeos/login/login_state.h"
// #include "components/drive/chromeos/file_system_interface.h"
// #include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "services/data_decoder/public/cpp/decode_image.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/public/cpp/message_center_switches.h"

namespace ash {

// void OnDecoded(const SkBitmap& bitmap) {
// LOG(ERROR) << "!! Decoded bitmap";
// }

using OnDecoded =
    base::OnceCallback<void(const std::vector<SkBitmap>& bitmaps)>;

void DecodeAPNG(scoped_refptr<base::RefCountedMemory> image_data,
                OnDecoded on_decoded) {
  // auto& rb = ResourceBundle::GetSharedInstance();
  // base::StringPiece image_data =
  //       rb.GetRawDataResource(GetEasyUnlockResource(state));

  service_manager::mojom::ConnectorRequest connector_request;
  std::unique_ptr<service_manager::Connector> connector =
      service_manager::Connector::Create(&connector_request);
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindConnectorRequest(std::move(connector_request));

  // Decode the image in sandboxed process because decode image_data comes from
  // external storage.
  data_decoder::DecodeImages(
      connector.get(),
      std::vector<uint8_t>(image_data->front(),
                           image_data->front() + image_data->size()),
      gfx::Size(), std::move(on_decoded));
}

}  // namespace ash