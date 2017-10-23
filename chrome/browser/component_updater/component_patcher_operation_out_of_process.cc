// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/component_patcher_operation_out_of_process.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string16.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/services/chrome_file_util/public/interfaces/constants.mojom.h"
#include "components/update_client/component_patcher_operation.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/l10n/l10n_util.h"

namespace component_updater {

ChromeOutOfProcessPatcher::ChromeOutOfProcessPatcher(
    std::unique_ptr<service_manager::Connector> connector)
    : connector_(std::move(connector)) {}

ChromeOutOfProcessPatcher::~ChromeOutOfProcessPatcher() = default;

void ChromeOutOfProcessPatcher::Patch(
    const std::string& operation,
    const base::FilePath& input_path,
    const base::FilePath& patch_path,
    const base::FilePath& output_path,
    const base::Callback<void(int result)>& callback) {
  DCHECK(!callback.is_null());

  callback_ = callback;

  base::File input_file(input_path,
                        base::File::FLAG_OPEN | base::File::FLAG_READ);
  base::File patch_file(patch_path,
                        base::File::FLAG_OPEN | base::File::FLAG_READ);
  base::File output_file(output_path, base::File::FLAG_CREATE |
                                          base::File::FLAG_WRITE |
                                          base::File::FLAG_EXCLUSIVE_WRITE);

  if (!input_file.IsValid() || !patch_file.IsValid() ||
      !output_file.IsValid()) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(callback_, -1));
    return;
  }

  DCHECK(!file_patcher_);

  connector_->BindInterface(chrome::mojom::kChromeFileUtilServiceName,
                            mojo::MakeRequest(&file_patcher_));
  file_patcher_.set_connection_error_handler(
      base::Bind(&ChromeOutOfProcessPatcher::PatchDone, this, -1));

  if (operation == update_client::kBsdiff) {
    file_patcher_->PatchFileBsdiff(
        std::move(input_file), std::move(patch_file), std::move(output_file),
        base::Bind(&ChromeOutOfProcessPatcher::PatchDone, this));
  } else if (operation == update_client::kCourgette) {
    file_patcher_->PatchFileCourgette(
        std::move(input_file), std::move(patch_file), std::move(output_file),
        base::Bind(&ChromeOutOfProcessPatcher::PatchDone, this));
  } else {
    NOTREACHED();
  }
}

void ChromeOutOfProcessPatcher::PatchDone(int result) {
  file_patcher_.reset();  // Remove our ref to the service so it can stop.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(callback_, result));
}

}  // namespace component_updater
