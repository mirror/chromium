// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_PDF_PRINTER_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_PDF_PRINTER_HANDLER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "chrome/browser/ui/webui/print_preview/printer_handler.h"
#include "chrome/common/features.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace base {
class RefCountedBytes;
}

namespace content {
class WebContents;
}

class Profile;

namespace gfx {
class Size;
}

namespace printing {
class StickySettings;
}

class PdfPrinterHandler : public PrinterHandler,
                          public ui::SelectFileDialog::Listener {
 public:
  PdfPrinterHandler(Profile* profile,
                    content::WebContents* preview_web_contents,
                    printing::StickySettings* sticky_settings);

  ~PdfPrinterHandler() override;

  // Cancels all pending requests.
  virtual void Reset() override;

  // Starts getting available printers.
  // |callback| should be called in the response to the request.
  void StartGetPrinters(const GetPrintersCallback& callback) override;

  // Starts getting printing capability of the printer with the provided
  // destination ID.
  // |callback| should be called in the response to the request.
  void StartGetCapability(const std::string& destination_id,
                          const GetCapabilityCallback& callback) override;

  // Starts granting access to the given provisional printer. The print handler
  // will respond with more information about the printer including its non-
  // provisional printer id.
  // |callback| should be called in response to the request.
  void StartGrantPrinterAccess(const std::string& printer_id,
                               const GetPrinterInfoCallback& callback);

  // Starts a print request.
  // |destination_id|: The printer to which print job should be sent.
  // |capability|: Capability reported by the printer.
  // |job_title|: The  title used for print job.
  // |ticket_json|: The print job ticket as JSON string.
  // |page_size|: The document page size.
  // |print_data|: The document bytes to print.
  // |callback| should be called in the response to the request.
  void StartPrint(const std::string& destination_id,
                  const std::string& capability,
                  const base::string16& job_title,
                  const std::string& ticket_json,
                  const gfx::Size& page_size,
                  const scoped_refptr<base::RefCountedBytes>& print_data,
                  const PrintCallback& callback) override;

  // SelectFileDialog::Listener implementation.
  virtual void FileSelected(const base::FilePath& path,
                            int index,
                            void* params) override;
  virtual void FileSelectionCanceled(void* params) override;

  // Sets |pdf_file_saved_closure_| to |closure|.
  void SetPdfSavedClosureForTesting(const base::Closure& closure);

 protected:
  virtual void SelectFile(const base::FilePath& default_filename,
                          bool prompt_user);

  // The underlying dialog object. Protected so unit tests can access it.
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;

  // The preview web contents. Protected for access by unit tests.
  content::WebContents* preview_web_contents_;

 private:
  void PostPrintToPdfTask();
  void OnGotUniqueFileName(const base::FilePath& path);
  void OnDirectoryCreated(const base::FilePath& path);

  Profile* profile_;
  printing::StickySettings* sticky_settings_;

  // Holds the path to the print to pdf request. It is empty if no such request
  // exists.
  base::FilePath print_to_pdf_path_;

  // Notifies tests that want to know if the PDF has been saved. This doesn't
  // notify the test if it was a successful save, only that it was attempted.
  base::Closure pdf_file_saved_closure_;

  // The data to print
  scoped_refptr<base::RefCountedBytes> print_data_;

  // The callback to call when complete.
  PrintCallback print_callback_;

  base::WeakPtrFactory<PdfPrinterHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PdfPrinterHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_PRINT_PREVIEW_PRINTER_HANDLER_H_
