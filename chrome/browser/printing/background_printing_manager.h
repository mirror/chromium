// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_BACKGROUND_PRINTING_MANAGER_H_
#define CHROME_BROWSER_PRINTING_BACKGROUND_PRINTING_MANAGER_H_

#include <map>
#include <memory>
#include <set>

#include "base/macros.h"
#include "base/sequence_checker.h"
#include "printing/features/features.h"

#if !BUILDFLAG(ENABLE_PRINT_PREVIEW)
#error "Print Preview must be enabled"
#endif

namespace content {
class BrowserContext;
class WebContents;
}

namespace printing {

// Manages hidden WebContents that prints documents in the background.
// The hidden WebContents are no longer part of any Browser / TabStripModel.
// The WebContents started life as a ConstrainedWebDialog.
// They get deleted when the printing finishes.
class BackgroundPrintingManager {
 public:
  class Observer;

  BackgroundPrintingManager();
  ~BackgroundPrintingManager();

  // Takes ownership of |preview_dialog| and deletes it when |preview_dialog|
  // finishes printing. This removes |preview_dialog| from its ConstrainedDialog
  // and hides it from the user.
  void OwnPrintPreviewDialog(content::WebContents* preview_dialog);

  // Deletes |preview_dialog| in response to failure in PrintViewManager.
  void PrintJobReleased(content::WebContents* preview_dialog);

  // Returns true if |printing_contents_map_| contains |preview_dialog|.
  bool HasPrintPreviewDialog(content::WebContents* preview_dialog);

  // Delete all preview contents that are associated with |browser_context|.
  void DeletePreviewContentsForBrowserContext(
      content::BrowserContext* browser_context);

  void OnPrintRequestCancelled(content::WebContents* preview_dialog);

 private:
  // Schedule deletion of |preview_contents|.
  void DeletePreviewContents(content::WebContents* preview_contents);

  // A map from print preview WebContentses (managed by
  // BackgroundPrintingManager) to the Observers that observe them.
  std::map<content::WebContents*, std::unique_ptr<Observer>>
      printing_contents_map_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(BackgroundPrintingManager);
};

}  // namespace printing

#endif  // CHROME_BROWSER_PRINTING_BACKGROUND_PRINTING_MANAGER_H_
