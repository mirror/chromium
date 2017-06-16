// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/win/chrome_select_file_dialog_factory.h"

#include <Windows.h>
#include <commdlg.h>

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "base/synchronization/waitable_event.h"
#include "base/win/win_util.h"
#include "chrome/common/shell_handler_win.mojom.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/utility_process_mojo_client.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/win/open_file_name_win.h"
#include "ui/shell_dialogs/select_file_dialog_win.h"

namespace {

constexpr base::Feature kIsolateShellOperations{
    "IsolateShellOperations", base::FEATURE_DISABLED_BY_DEFAULT};

// Helper class used by CreateWinSelectFileDialog to delegate calls to
// ::GetOpenFileName() and ::GetSaveFileName() to a utility process.
class OpenFileNameClient {
 public:
  OpenFileNameClient();

  // Calls ::GetOpenFileName() and stores the result in |directory| and
  // |filenames|. Returns false on failure.
  bool BlockingGetOpenFileName(OPENFILENAME* ofn);

  // Calls ::GetSaveFileName() and stores the result in |path| and
  // |one_based_filter_index|. Returns false on failure.
  bool BlockingGetSaveFileName(OPENFILENAME* ofn);

 private:
  // Calls ::GetOpenFileName() in the utility process: the results are
  // returned in the done callback.
  void CallGetOpenFileName(OPENFILENAME* ofn);
  void GetOpenFileNameDone(const base::FilePath& directory,
                           const std::vector<base::FilePath>& filenames);

  // Calls ::GetSaveFileName() in the utility process: the results are
  // returned in the done callback.
  void CallGetSaveFileName(OPENFILENAME* ofn);
  void GetSaveFileNameDone(const base::FilePath& path,
                           uint32_t one_based_filter_index);

  // Utility process used for the delegated calls.
  void StartUtilityProcess();
  void UtilityProcessFailed();

  // Client used for sending requests to the utility process.
  std::unique_ptr<
      content::UtilityProcessMojoClient<chrome::mojom::ShellHandler>>
      utility_process_mojo_client_;

  // Used to block until the request result is received.
  base::WaitableEvent result_received_event_;

  // CallGetOpenFileName result.
  base::FilePath directory_;
  std::vector<base::FilePath> filenames_;

  // CallGetSaveFileName result.
  base::FilePath path_;
  DWORD one_based_filter_index_ = 0;

  DISALLOW_COPY_AND_ASSIGN(OpenFileNameClient);
};

OpenFileNameClient::OpenFileNameClient()
    : result_received_event_(base::WaitableEvent::ResetPolicy::MANUAL,
                             base::WaitableEvent::InitialState::NOT_SIGNALED) {}

bool OpenFileNameClient::BlockingGetOpenFileName(OPENFILENAME* ofn) {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&OpenFileNameClient::CallGetOpenFileName,
                 base::Unretained(this), ofn));

  result_received_event_.Wait();

  if (filenames_.empty())
    return false;

  ui::win::OpenFileName::SetResult(directory_, filenames_, ofn);
  return true;
}

bool OpenFileNameClient::BlockingGetSaveFileName(OPENFILENAME* ofn) {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&OpenFileNameClient::CallGetSaveFileName,
                 base::Unretained(this), ofn));

  result_received_event_.Wait();

  if (path_.empty())
    return false;

  base::wcslcpy(ofn->lpstrFile, path_.value().c_str(), ofn->nMaxFile);
  ofn->nFilterIndex = one_based_filter_index_;
  return true;
}

void OpenFileNameClient::CallGetOpenFileName(OPENFILENAME* ofn) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  StartUtilityProcess();

  utility_process_mojo_client_->service()->CallGetOpenFileName(
      base::win::HandleToUint32(ofn->hwndOwner),
      static_cast<uint32_t>(ofn->Flags), ui::win::OpenFileName::GetFilters(ofn),
      ofn->lpstrInitialDir ? base::FilePath(ofn->lpstrInitialDir)
                           : base::FilePath(),
      base::FilePath(ofn->lpstrFile),
      base::Bind(&OpenFileNameClient::GetOpenFileNameDone,
                 base::Unretained(this)));
}

void OpenFileNameClient::GetOpenFileNameDone(
    const base::FilePath& directory,
    const std::vector<base::FilePath>& filenames) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  utility_process_mojo_client_.reset();
  directory_ = directory;
  filenames_ = filenames;

  result_received_event_.Signal();
}

void OpenFileNameClient::CallGetSaveFileName(OPENFILENAME* ofn) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  StartUtilityProcess();

  utility_process_mojo_client_->service()->CallGetSaveFileName(
      base::win::HandleToUint32(ofn->hwndOwner),
      static_cast<uint32_t>(ofn->Flags), ui::win::OpenFileName::GetFilters(ofn),
      ofn->nFilterIndex,
      ofn->lpstrInitialDir ? base::FilePath(ofn->lpstrInitialDir)
                           : base::FilePath(),
      base::FilePath(ofn->lpstrFile),
      ofn->lpstrDefExt ? base::FilePath(ofn->lpstrDefExt) : base::FilePath(),
      base::Bind(&OpenFileNameClient::GetSaveFileNameDone,
                 base::Unretained(this)));
}

void OpenFileNameClient::GetSaveFileNameDone(const base::FilePath& path,
                                             uint32_t one_based_filter_index) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  utility_process_mojo_client_.reset();
  path_ = path;
  one_based_filter_index_ = one_based_filter_index;

  result_received_event_.Signal();
}

void OpenFileNameClient::StartUtilityProcess() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(!utility_process_mojo_client_);

  utility_process_mojo_client_ = base::MakeUnique<
      content::UtilityProcessMojoClient<chrome::mojom::ShellHandler>>(
      l10n_util::GetStringUTF16(IDS_UTILITY_PROCESS_FILE_DIALOG_NAME));

  utility_process_mojo_client_->set_error_callback(base::Bind(
      &OpenFileNameClient::UtilityProcessFailed, base::Unretained(this)));

  utility_process_mojo_client_->set_disable_sandbox();

  utility_process_mojo_client_->Start();
}

void OpenFileNameClient::UtilityProcessFailed() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  utility_process_mojo_client_.reset();

  result_received_event_.Signal();
}

bool GetOpenFileNameImpl(OPENFILENAME* ofn) {
  if (base::FeatureList::IsEnabled(kIsolateShellOperations))
    return OpenFileNameClient().BlockingGetOpenFileName(ofn);
  return ::GetOpenFileName(ofn) == TRUE;
}

bool GetSaveFileNameImpl(OPENFILENAME* ofn) {
  if (base::FeatureList::IsEnabled(kIsolateShellOperations))
    return OpenFileNameClient().BlockingGetSaveFileName(ofn);
  return ::GetSaveFileName(ofn) == TRUE;
}

}  // namespace

ChromeSelectFileDialogFactory::ChromeSelectFileDialogFactory() = default;

ChromeSelectFileDialogFactory::~ChromeSelectFileDialogFactory() = default;

ui::SelectFileDialog* ChromeSelectFileDialogFactory::Create(
    ui::SelectFileDialog::Listener* listener,
    ui::SelectFilePolicy* policy) {
  return ui::CreateWinSelectFileDialog(listener, policy,
                                       base::Bind(GetOpenFileNameImpl),
                                       base::Bind(GetSaveFileNameImpl));
}
