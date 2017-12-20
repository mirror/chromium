// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/background_printing_manager.h"

#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/printing/print_job.h"
#include "chrome/browser/printing/print_preview_dialog_controller.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"

using content::BrowserContext;
using content::BrowserThread;
using content::WebContents;

namespace printing {

class BackgroundPrintingManager::Observer
    : public content::WebContentsObserver {
 public:
  Observer(BackgroundPrintingManager* manager, WebContents* web_contents);

 private:
  void RenderProcessGone(base::TerminationStatus status) override;
  void WebContentsDestroyed() override;

  BackgroundPrintingManager* manager_;
};

BackgroundPrintingManager::Observer::Observer(
    BackgroundPrintingManager* manager, WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      manager_(manager) {
}

void BackgroundPrintingManager::Observer::RenderProcessGone(
    base::TerminationStatus status) {
  manager_->DeletePreviewContents(web_contents());
}
void BackgroundPrintingManager::Observer::WebContentsDestroyed() {
  manager_->DeletePreviewContents(web_contents());
}

BackgroundPrintingManager::BackgroundPrintingManager() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

BackgroundPrintingManager::~BackgroundPrintingManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // The might be some WebContentses still in |printing_contents_map_| at this
  // point (e.g. when the last remaining tab closes and there is still a print
  // preview WebContents trying to print). In such a case it will fail to print,
  // but we should at least clean up the observers.
  // TODO(thestig): Handle this case better.
}

void BackgroundPrintingManager::OwnPrintPreviewDialog(
    WebContents* preview_dialog) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(PrintPreviewDialogController::IsPrintPreviewURL(
      preview_dialog->GetURL()));
  CHECK(!HasPrintPreviewDialog(preview_dialog));

  printing_contents_map_[preview_dialog] =
      base::MakeUnique<Observer>(this, preview_dialog);

  // Activate the initiator.
  PrintPreviewDialogController* dialog_controller =
      PrintPreviewDialogController::GetInstance();
  if (!dialog_controller)
    return;
  WebContents* initiator = dialog_controller->GetInitiator(preview_dialog);
  if (!initiator)
    return;
  initiator->GetDelegate()->ActivateContents(initiator);
}

void BackgroundPrintingManager::PrintJobReleased(
    content::WebContents* preview_dialog) {
  DeletePreviewContents(preview_dialog);
}

void BackgroundPrintingManager::DeletePreviewContentsForBrowserContext(
    BrowserContext* browser_context) {
  std::vector<WebContents*> preview_contents_to_delete;
  for (const auto& iter : printing_contents_map_) {
    WebContents* preview_contents = iter.first;
    if (preview_contents->GetBrowserContext() == browser_context) {
      preview_contents_to_delete.push_back(preview_contents);
    }
  }

  for (size_t i = 0; i < preview_contents_to_delete.size(); i++) {
    DeletePreviewContents(preview_contents_to_delete[i]);
  }
}

void BackgroundPrintingManager::OnPrintRequestCancelled(
    WebContents* preview_contents) {
  DeletePreviewContents(preview_contents);
}

void BackgroundPrintingManager::DeletePreviewContents(
    WebContents* preview_contents) {
  size_t erased_count = printing_contents_map_.erase(preview_contents);
  if (erased_count == 0) {
    // Everyone is racing to be the first to delete the |preview_contents|. If
    // this case is hit, someone else won the race, so there is no need to
    // continue. <http://crbug.com/100806>
    return;
  }

  // Mortally wound the contents. Deletion immediately is not a good idea in
  // case this was triggered by |preview_contents| far up the callstack.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, preview_contents);
}

bool BackgroundPrintingManager::HasPrintPreviewDialog(
    WebContents* preview_dialog) {
  return base::ContainsKey(printing_contents_map_, preview_dialog);
}

}  // namespace printing
