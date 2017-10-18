// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMPONENT_UPDATER_COMPONENT_PATCHER_OPERATION_OUT_OF_PROCESS_H_
#define CHROME_BROWSER_COMPONENT_UPDATER_COMPONENT_PATCHER_OPERATION_OUT_OF_PROCESS_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/services/chrome_file_util/public/interfaces/file_patcher.mojom.h"
#include "components/update_client/out_of_process_patcher.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace service_manager {
class Connector;
}

namespace component_updater {

class ChromeOutOfProcessPatcher : public update_client::OutOfProcessPatcher {
 public:
  explicit ChromeOutOfProcessPatcher(
      std::unique_ptr<service_manager::Connector> connector);

  // update_client::OutOfProcessPatcher:
  void Patch(const std::string& operation,
             const base::FilePath& input_path,
             const base::FilePath& patch_path,
             const base::FilePath& output_path,
             const base::Callback<void(int result)>& callback) override;

 private:
  ~ChromeOutOfProcessPatcher() override;

  // Patch operation result handler.
  void PatchDone(int result);

  // Used to signal the operation result back to the Patch() requester.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::Callback<void(int result)> callback_;

  std::unique_ptr<service_manager::Connector> connector_;

  // Interface ptr to the patcher running in the Chrome file util service.
  chrome::mojom::FilePatcherPtr file_patcher_;

  DISALLOW_COPY_AND_ASSIGN(ChromeOutOfProcessPatcher);
};

}  // namespace component_updater

#endif  // CHROME_BROWSER_COMPONENT_UPDATER_COMPONENT_PATCHER_OPERATION_OUT_OF_PROCESS_H_
